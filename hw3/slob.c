/*
 * SLOB Allocator: Simple List Of Blocks
 *
 * Matt Mackall <mpm@selenic.com> 12/30/03
 *
 * NUMA support by Paul Mundt, 2007.
 *
 * How SLOB works:
 *
 * The core of SLOB is a traditional K&R style heap allocator, with
 * support for returning aligned objects. The granularity of this
 * allocator is as little as 2 bytes, however typically most architectures
 * will require 4 bytes on 32-bit and 8 bytes on 64-bit.
 *
 * The slob heap is a set of linked list of pages from alloc_pages(),
 * and within each page, there is a singly-linked list of free blocks
 * (slob_t). The heap is grown on demand. To reduce fragmentation,
 * heap pages are segregated into three lists, with objects less than
 * 256 bytes, objects less than 1024 bytes, and all other objects.
 *
 * Allocation from heap involves first searching for a page with
 * sufficient free blocks (using a next-fit-like approach) followed by
 * a first-fit scan of the page. Deallocation inserts objects back
 * into the free list in address order, so this is effectively an
 * address-ordered first fit.
 *
 * Above this is an implementation of kmalloc/kfree. Blocks returned
 * from kmalloc are prepended with a 4-byte header with the kmalloc size.
 * If kmalloc is asked for objects of PAGE_SIZE or larger, it calls
 * alloc_pages() directly, allocating compound pages so the page order
 * does not have to be separately tracked.
 * These objects are detected in kfree() because PageSlab()
 * is false for them.
 *
 * SLAB is emulated on top of SLOB by simply calling constructors and
 * destructors for every SLAB allocation. Objects are returned with the
 * 4-byte alignment unless the SLAB_HWCACHE_ALIGN flag is set, in which
 * case the low-level allocator will fragment blocks to create the proper
 * alignment. Again, objects of page-size or greater are allocated by
 * calling alloc_pages(). As SLAB objects know their size, no separate
 * size bookkeeping is necessary and there is essentially no allocation
 * space overhead, and compound pages aren't needed for multi-page
 * allocations.
 *
 * NUMA support in SLOB is fairly simplistic, pushing most of the real
 * logic down to the page allocator, and simply doing the node accounting
 * on the upper levels. In the event that a node id is explicitly
 * provided, alloc_pages_exact_node() with the specified node id is used
 * instead. The common case (or when the node id isn't explicitly provided)
 * will default to the current node, as per numa_node_id().
 *
 * Node aware pages are still inserted in to the global freelist, and
 * these are scanned for by matching against the node id encoded in the
 * page flags. As a result, block allocations that can be satisfied from
 * the freelist will only be done so on pages residing on the same node,
 * in order to prevent random node placement.
 */

#define BESTFIT
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/mm.h>
#include <linux/swap.h> /* struct reclaim_state */
#include <linux/cache.h>
#include <linux/init.h>
#include <linux/export.h>
#include <linux/rcupdate.h>
#include <linux/list.h>
#include <linux/kmemleak.h>

#include <trace/events/kmem.h>

#include <linux/atomic.h>

#include "slab.h"
/*
 * slob_block has a field 'units', which indicates size of block if +ve,
 * or offset of next block if -ve (in SLOB_UNITs).
 *
 * Free blocks of size 1 unit simply contain the offset of the next block.
 * Those with larger size contain their size in the first SLOB_UNIT of
 * memory, and the offset of the next free block in the second SLOB_UNIT.
 */
#if PAGE_SIZE <= (32767 * 2)
typedef s16 slobidx_t;
#else
typedef s32 slobidx_t;
#endif

static unsigned long print_counter = 0;
static unsigned long total_alloc_mem = 0;
static unsigned long total_free_mem = 0;

struct slob_block {
	slobidx_t units;
};
typedef struct slob_block slob_t;

/*
 * All partially free slob pages go on these lists.
 */
#define SLOB_BREAK1 256
#define SLOB_BREAK2 1024
static LIST_HEAD(free_slob_small);
static LIST_HEAD(free_slob_medium);
static LIST_HEAD(free_slob_large);

/*
 * slob_page_free: true for pages on free_slob_pages list.
 */
static inline int slob_page_free(struct page *sp)
{
	return PageSlobFree(sp);
}

static void set_slob_page_free(struct page *sp, struct list_head *list)
{
	list_add(&sp->list, list);
	__SetPageSlobFree(sp);
}

static inline void clear_slob_page_free(struct page *sp)
{
	list_del(&sp->list);
	__ClearPageSlobFree(sp);
}

#define SLOB_UNIT sizeof(slob_t)
#define SLOB_UNITS(size) DIV_ROUND_UP(size, SLOB_UNIT)

/*
 * struct slob_rcu is inserted at the tail of allocated slob blocks, which
 * were created with a SLAB_DESTROY_BY_RCU slab. slob_rcu is used to free
 * the block using call_rcu.
 */
struct slob_rcu {
	struct rcu_head head;
	int size;
};

/*
 * slob_lock protects all slob allocator structures.
 */
static DEFINE_SPINLOCK(slob_lock);

/*
 * Encode the given size and next info into a free slob block s.
 */
static void set_slob(slob_t *s, slobidx_t size, slob_t *next)
{
	slob_t *base = (slob_t *)((unsigned long)s & PAGE_MASK);
	slobidx_t offset = next - base;

	if (size > 1) {
		s[0].units = size;
		s[1].units = offset;
	} else
		s[0].units = -offset;
}

/*
 * Return the size of a slob block.
 */
static slobidx_t slob_units(slob_t *s)
{
	if (s->units > 0)
		return s->units;
	return 1;
}

/*
 * Return the next free slob block pointer after this one.
 */
static slob_t *slob_next(slob_t *s)
{
	slob_t *base = (slob_t *)((unsigned long)s & PAGE_MASK);
	slobidx_t next;

	if (s[0].units < 0)
		next = -s[0].units;
	else
		next = s[1].units;
	return base+next;
}

/*
 * Returns true if s is the last free block in its page.
 */
static int slob_last(slob_t *s)
{
	return !((unsigned long)slob_next(s) & ~PAGE_MASK);
}

static void *slob_new_pages(gfp_t gfp, int order, int node)
{
	void *page;

#ifdef CONFIG_NUMA
	if (node != NUMA_NO_NODE)
		page = alloc_pages_exact_node(node, gfp, order);
	else
#endif
		page = alloc_pages(gfp, order);

	if (!page)
		return NULL;

	// update total alloc mem
	total_alloc_mem += sizeof(page);
	return page_address(page);
}

static void slob_free_pages(void *b, int order)
{
	if (current->reclaim_state)
			current->reclaim_state->reclaimed_slab += 1 << order;

	// update total alloc mem
	total_alloc_mem -= sizeof(b);
	free_pages((unsigned long)b, order);
}

/*
 * "slob_page_alloc":
 * Allocate a slob block within a given slob_page sp.
 */

/*
 * For the Best Fit algorithm:
 * Almost all variables are duplicated like this : variableX => best_variableX
 *
 * We let the loop run with the initial variables, then update the values to the "best" ones
 * so it works on both Algorithms if you want to make "ifdef" statements inside the function.
 *
 */
#ifdef BESTFIT //slob_page_alloc function for BESTFIT
static void *slob_page_alloc(struct page *sp, size_t size, int align)
{

  /*
   * Arguments:
   * sp is a pointer to the current page
   * size is the size requested for the block
   * align = used for delta and fragmentation..os things
   */

	//Variables
	slob_t *prev, *cur, *aligned = NULL;
	int delta = 0, units = SLOB_UNITS(size);

	//why do we have these variables?read function explaining!
	slob_t *best_prev = NULL;
	slob_t *best_cur = NULL;
	slob_t *best_aligned = NULL;
	int best_delta = 0;
	slobidx_t best_fit = 0; //variable that keeps the minimum space

    if (print_counter > 6000){
        printk("slob_Request: %u\n", units);
        printk("slob_alloc: Candidate blocks size:");
    }

	//traverse each block in the list
	for (prev = NULL, cur = sp->freelist; ; prev = cur, cur = slob_next(cur)) {
		slobidx_t avail = slob_units(cur); //available space in the block

		if (align) { // OS things..do not change.
			aligned = (slob_t *)ALIGN((unsigned long)cur, align);
			delta = aligned - cur;
		}

        // print each block size
        if (print_counter > 6000){
            printk(" %u", avail - delta);
        }

		/*
		 * if there is enough room, then get the block.
		 * we need to get the BEST block for BESTFIT algorithm
		 */

		if ( (avail >= units + delta) && //if it can handle the request
			( (avail - (units + delta ) < best_fit ) || best_cur == NULL ) ) {
			//if it's better than the previous one found (OR if it's the first one being scanned)


			//Update all "best" variables with the right values
			best_prev = prev;
			best_cur = cur;
			best_aligned = aligned;
			best_delta = delta;
			best_fit = avail - (units + delta);	 //current one

			/*we need to return best_fit node block when we are done checking the list
					reminder: we are inside a for loop
				*/
		}

		if (slob_last(best_cur)){
			if (best_cur != NULL) {

			slob_t *next;

			//
			slob_t best_next = NULL;

			slobidx_t best_avail = slob_units(best_cur);
			//slob_units = macro that returns the size of a slob block


			//Change *every* variable to work for both BESTFIT and FIRSTFIT variables.

			if (best_delta) { /* need to fragment head to align? */
				best_next = slob_next(best_cur);
				set_slob(best_aligned, best_avail - best_delta, best_next);
				set_slob(best_cur, best_delta, best_aligned);
				best_prev = best_cur;
				best_cur = best_aligned;
				best_avail = slob_units(best_cur);
			}

			best_next = slob_next(best_cur);
			if (best_avail == units) { /* exact fit? unlink. */
				if (best_prev)
					set_slob(best_prev, slob_units(best_prev), best_next);
				else
					sp->freelist = best_next;
			} else { /* fragment */
				if (best_prev)
					set_slob(best_prev, slob_units(best_prev), best_cur + units);
				else
					sp->freelist = best_cur + units;
				set_slob(best_cur + units, best_avail - units, best_next);
			}

			sp->units -= units;
			if (!sp->units){
				clear_slob_page_free(sp);
			}


			if (print_counter > 6000){
				printk("\nslob_alloc: Best Fit:  %u\n", best_avail);
			}

			return best_cur; //we use the "best" variable now, not the old one.


			}

			if (print_counter > 6000){
				printk("\nslob_alloc: Best Fit: None\n");
			}
			return NULL;
		}

	}
}

#else //slob_page_alloc function for FIRSTFIT
/*
 * Allocate a slob block within a given slob_page sp.
 */
static void *slob_page_alloc(struct page *sp, size_t size, int align)
{
	slob_t *prev, *cur, *aligned = NULL;
	int delta = 0, units = SLOB_UNITS(size);

	for (prev = NULL, cur = sp->freelist; ; prev = cur, cur = slob_next(cur)) {
		slobidx_t avail = slob_units(cur);

		if (align) {
			aligned = (slob_t *)ALIGN((unsigned long)cur, align);
			delta = aligned - cur;
		}
		if (avail >= units + delta) { /* room enough? */
			slob_t *next;

			if (delta) { /* need to fragment head to align? */
				next = slob_next(cur);
				set_slob(aligned, avail - delta, next);
				set_slob(cur, delta, aligned);
				prev = cur;
				cur = aligned;
				avail = slob_units(cur);
			}

			next = slob_next(cur);
			if (avail == units) { /* exact fit? unlink. */
				if (prev)
					set_slob(prev, slob_units(prev), next);
				else
					sp->freelist = next;
			} else { /* fragment */
				if (prev)
					set_slob(prev, slob_units(prev), cur + units);
				else
					sp->freelist = cur + units;
				set_slob(cur + units, avail - units, next);
			}

			sp->units -= units;
			if (!sp->units)
				clear_slob_page_free(sp);
			return cur;
		}
		if (slob_last(cur))
			return NULL;
	}
}

#endif



/*
 * Returns the best fitting block size for the given page.
 * Possible values: 0 (perfect fit), -1 (no block) or positive integer
 */
static int best_fit_page(struct slob_page *sp, size_t size, int align){

    slob_t *prev, *cur, *aligned = NULL;
    int delta = 0, units = SLOB_UNITs(size);

    slobidx_t best = -1;
    slob_t *best_cur = NULL;


    // traversal of all blocks to find the best fit
    for (prev = NULL, cur = sp->free; ; prev = cur, cur = slob_next(cur)){

        //available free slob units
        slobidx avail = slob_units(cur);


        if (align){
            aligned = (slob_t *)ALIGN((unsigned long)cur, align);
            delta = aligned - cur;
        }

        // main check for best block size
        if (avail > units + delta && (best_cur == NULL || avail - (units + delta < best))){
            best_cur = cur;
            best = avail - (units + delta);

            if (best==0){ //exact fit
                return 0;
            }
        }

        // End of slob block list, return best fit block for this page
        if (slob_last(cur)){
            if (best_cur != NULL){
                return best //positive integer
            }
            else{
                return -1; //no block available, search another page..
            }
        }
    } // loop end
}

// Calculation of free memory!
unsigned long calc_free_mem(struct list_head *slob_list){

	unsigned long sum = 0;
	struct page *sp;

	list_for_each_entry(sp, slob_list, list) {
		sum += sp->units;
	}
	return sum;
}

/*
 * slob_alloc: entry point into the slob allocator.
 */
static void *slob_alloc(size_t size, gfp_t gfp, int align, int node)
{

    if (print_counter > 6000){
        print_counter = 0;
    }
    print_counter++;

	struct page *sp;
	struct list_head *prev;
	struct list_head *slob_list;
	slob_t *b = NULL;
	unsigned long flags;

    int best_fit = -1;
    struct page *best_sp = NULL;

	if (size < SLOB_BREAK1)
		slob_list = &free_slob_small;
	else if (size < SLOB_BREAK2)
		slob_list = &free_slob_medium;
	else
		slob_list = &free_slob_large;

	spin_lock_irqsave(&slob_lock, flags);
	/* Iterate through each partially free page, try to find room */
	list_for_each_entry(sp, slob_list, list) {

        int tmp_fit = -1;
#ifdef CONFIG_NUMA
		/*
		 * If there's a node specification, search for a partial
		 * page with a matching node id in the freelist.
		 */
		if (node != NUMA_NO_NODE && page_to_nid(sp) != node)
			continue;
#endif
		/* Enough room on this page? */
		if (sp->units < SLOB_UNITS(size))
			continue;

        // calc fit size of current page
        tmp_fit = best_fit_page(sp, size, align);

        if (tmp_fit == 0){ //exact fit!
            best_sp = sp;
            break;
        }
        else if (tmp_fit > 0 && (best_fit == -1 || tmp_fit < best_fit)){
            best_sp = sp;
            best_fit = tmp_fit;
        }
        continue;
    } // end loop

    if (best_fit >= 0){ // found a page!

        if (best_sp != NULL){ // try to allocate
            b = slob_page_alloc(best_sp, size, align);
        }
    }

	// calculation of total free memory as need it
	total_free_mem = calc_free_mem(free_slob_small) + calc_free_mem(free_slob_medium) + calc_free_mem(free_slob_large);

	spin_unlock_irqrestore(&slob_lock, flags);

	/* Not enough space: must allocate a new page */
	if (!b) {
		b = slob_new_pages(gfp & ~__GFP_ZERO, 0, node);
		if (!b)
			return NULL;
		sp = virt_to_page(b);
		__SetPageSlab(sp);

		spin_lock_irqsave(&slob_lock, flags);
		sp->units = SLOB_UNITS(PAGE_SIZE);
		sp->freelist = b;
		INIT_LIST_HEAD(&sp->list);
		set_slob(b, SLOB_UNITS(PAGE_SIZE), b + SLOB_UNITS(PAGE_SIZE));
		set_slob_page_free(sp, slob_list);
		b = slob_page_alloc(sp, size, align);
		BUG_ON(!b);
		spin_unlock_irqrestore(&slob_lock, flags);
	}
	if (unlikely((gfp & __GFP_ZERO) && b))
		memset(b, 0, size);

	return b;
}

/*
 * slob_free: entry point into the slob allocator.
 */
static void slob_free(void *block, int size)
{
	struct page *sp;
	slob_t *prev, *next, *b = (slob_t *)block;
	slobidx_t units;
	unsigned long flags;
	struct list_head *slob_list;

	if (unlikely(ZERO_OR_NULL_PTR(block)))
		return;
	BUG_ON(!size);

	sp = virt_to_page(block);
	units = SLOB_UNITS(size);

	spin_lock_irqsave(&slob_lock, flags);

	if (sp->units + units == SLOB_UNITS(PAGE_SIZE)) {
		/* Go directly to page allocator. Do not pass slob allocator */
		if (slob_page_free(sp))
			clear_slob_page_free(sp);
		spin_unlock_irqrestore(&slob_lock, flags);
		__ClearPageSlab(sp);
		page_mapcount_reset(sp);
		slob_free_pages(b, 0);
		return;
	}

	if (!slob_page_free(sp)) {
		/* This slob page is about to become partially free. Easy! */
		sp->units = units;
		sp->freelist = b;
		set_slob(b, units,
			(void *)((unsigned long)(b +
					SLOB_UNITS(PAGE_SIZE)) & PAGE_MASK));
		if (size < SLOB_BREAK1)
			slob_list = &free_slob_small;
		else if (size < SLOB_BREAK2)
			slob_list = &free_slob_medium;
		else
			slob_list = &free_slob_large;
		set_slob_page_free(sp, slob_list);
		goto out;
	}

	/*
	 * Otherwise the page is already partially free, so find reinsertion
	 * point.
	 */
	sp->units += units;

	if (b < (slob_t *)sp->freelist) {
		if (b + units == sp->freelist) {
			units += slob_units(sp->freelist);
			sp->freelist = slob_next(sp->freelist);
		}
		set_slob(b, units, sp->freelist);
		sp->freelist = b;
	} else {
		prev = sp->freelist;
		next = slob_next(prev);
		while (b > next) {
			prev = next;
			next = slob_next(prev);
		}

		if (!slob_last(prev) && b + units == next) {
			units += slob_units(next);
			set_slob(b, units, slob_next(next));
		} else
			set_slob(b, units, next);

		if (prev + slob_units(prev) == b) {
			units = slob_units(b) + slob_units(prev);
			set_slob(prev, units, slob_next(b));
		} else
			set_slob(prev, slob_units(prev), b);
	}
out:
	spin_unlock_irqrestore(&slob_lock, flags);
}

/*
 * End of slob allocator proper. Begin kmem_cache_alloc and kmalloc frontend.
 */

static __always_inline void *
__do_kmalloc_node(size_t size, gfp_t gfp, int node, unsigned long caller)
{
	unsigned int *m;
	int align = max_t(size_t, ARCH_KMALLOC_MINALIGN, ARCH_SLAB_MINALIGN);
	void *ret;

	gfp &= gfp_allowed_mask;

	lockdep_trace_alloc(gfp);

	if (size < PAGE_SIZE - align) {
		if (!size)
			return ZERO_SIZE_PTR;

		m = slob_alloc(size + align, gfp, align, node);

		if (!m)
			return NULL;
		*m = size;
		ret = (void *)m + align;

		trace_kmalloc_node(caller, ret,
				   size, size + align, gfp, node);
	} else {
		unsigned int order = get_order(size);

		if (likely(order))
			gfp |= __GFP_COMP;
		ret = slob_new_pages(gfp, order, node);

		trace_kmalloc_node(caller, ret,
				   size, PAGE_SIZE << order, gfp, node);
	}

	kmemleak_alloc(ret, size, 1, gfp);
	return ret;
}

void *__kmalloc(size_t size, gfp_t gfp)
{
	return __do_kmalloc_node(size, gfp, NUMA_NO_NODE, _RET_IP_);
}
EXPORT_SYMBOL(__kmalloc);

#ifdef CONFIG_TRACING
void *__kmalloc_track_caller(size_t size, gfp_t gfp, unsigned long caller)
{
	return __do_kmalloc_node(size, gfp, NUMA_NO_NODE, caller);
}

#ifdef CONFIG_NUMA
void *__kmalloc_node_track_caller(size_t size, gfp_t gfp,
					int node, unsigned long caller)
{
	return __do_kmalloc_node(size, gfp, node, caller);
}
#endif
#endif

void kfree(const void *block)
{
	struct page *sp;

	trace_kfree(_RET_IP_, block);

	if (unlikely(ZERO_OR_NULL_PTR(block)))
		return;
	kmemleak_free(block);

	sp = virt_to_page(block);
	if (PageSlab(sp)) {
		int align = max_t(size_t, ARCH_KMALLOC_MINALIGN, ARCH_SLAB_MINALIGN);
		unsigned int *m = (unsigned int *)(block - align);
		slob_free(m, *m + align);
	} else
		__free_pages(sp, compound_order(sp));
}
EXPORT_SYMBOL(kfree);

/* can't use ksize for kmem_cache_alloc memory, only kmalloc */
size_t ksize(const void *block)
{
	struct page *sp;
	int align;
	unsigned int *m;

	BUG_ON(!block);
	if (unlikely(block == ZERO_SIZE_PTR))
		return 0;

	sp = virt_to_page(block);
	if (unlikely(!PageSlab(sp)))
		return PAGE_SIZE << compound_order(sp);

	align = max_t(size_t, ARCH_KMALLOC_MINALIGN, ARCH_SLAB_MINALIGN);
	m = (unsigned int *)(block - align);
	return SLOB_UNITS(*m) * SLOB_UNIT;
}
EXPORT_SYMBOL(ksize);

int __kmem_cache_create(struct kmem_cache *c, unsigned long flags)
{
	if (flags & SLAB_DESTROY_BY_RCU) {
		/* leave room for rcu footer at the end of object */
		c->size += sizeof(struct slob_rcu);
	}
	c->flags = flags;
	return 0;
}

void *slob_alloc_node(struct kmem_cache *c, gfp_t flags, int node)
{
	void *b;

	flags &= gfp_allowed_mask;

	lockdep_trace_alloc(flags);

	if (c->size < PAGE_SIZE) {
		b = slob_alloc(c->size, flags, c->align, node);
		trace_kmem_cache_alloc_node(_RET_IP_, b, c->object_size,
					    SLOB_UNITS(c->size) * SLOB_UNIT,
					    flags, node);
	} else {
		b = slob_new_pages(flags, get_order(c->size), node);
		trace_kmem_cache_alloc_node(_RET_IP_, b, c->object_size,
					    PAGE_SIZE << get_order(c->size),
					    flags, node);
	}

	if (b && c->ctor)
		c->ctor(b);

	kmemleak_alloc_recursive(b, c->size, 1, c->flags, flags);
	return b;
}
EXPORT_SYMBOL(slob_alloc_node);

void *kmem_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
{
	return slob_alloc_node(cachep, flags, NUMA_NO_NODE);
}
EXPORT_SYMBOL(kmem_cache_alloc);

#ifdef CONFIG_NUMA
void *__kmalloc_node(size_t size, gfp_t gfp, int node)
{
	return __do_kmalloc_node(size, gfp, node, _RET_IP_);
}
EXPORT_SYMBOL(__kmalloc_node);

void *kmem_cache_alloc_node(struct kmem_cache *cachep, gfp_t gfp, int node)
{
	return slob_alloc_node(cachep, gfp, node);
}
EXPORT_SYMBOL(kmem_cache_alloc_node);
#endif

static void __kmem_cache_free(void *b, int size)
{
	if (size < PAGE_SIZE)
		slob_free(b, size);
	else
		slob_free_pages(b, get_order(size));
}

static void kmem_rcu_free(struct rcu_head *head)
{
	struct slob_rcu *slob_rcu = (struct slob_rcu *)head;
	void *b = (void *)slob_rcu - (slob_rcu->size - sizeof(struct slob_rcu));

	__kmem_cache_free(b, slob_rcu->size);
}

void kmem_cache_free(struct kmem_cache *c, void *b)
{
	kmemleak_free_recursive(b, c->flags);
	if (unlikely(c->flags & SLAB_DESTROY_BY_RCU)) {
		struct slob_rcu *slob_rcu;
		slob_rcu = b + (c->size - sizeof(struct slob_rcu));
		slob_rcu->size = c->size;
		call_rcu(&slob_rcu->head, kmem_rcu_free);
	} else {
		__kmem_cache_free(b, c->size);
	}

	trace_kmem_cache_free(_RET_IP_, b);
}
EXPORT_SYMBOL(kmem_cache_free);

int __kmem_cache_shutdown(struct kmem_cache *c)
{
	/* No way to check for remaining objects */
	return 0;
}

int kmem_cache_shrink(struct kmem_cache *d)
{
	return 0;
}
EXPORT_SYMBOL(kmem_cache_shrink);

struct kmem_cache kmem_cache_boot = {
	.name = "kmem_cache",
	.size = sizeof(struct kmem_cache),
	.flags = SLAB_PANIC,
	.align = ARCH_KMALLOC_MINALIGN,
};

void __init kmem_cache_init(void)
{
	kmem_cache = &kmem_cache_boot;
	slab_state = UP;
}

void __init kmem_cache_init_late(void)
{
	slab_state = FULL;
}
