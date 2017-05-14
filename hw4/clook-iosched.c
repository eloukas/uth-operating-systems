/*
 * elevator CLOOK
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

#define DEBUG

struct clook_data {
	struct list_head queue;
};

#ifdef DEBUG
// PRINT ENTIRE LIST. FOR DEBUG PURPOSE!
void my_print(struct request_queue *q, struct request *rq){
    struct clook_data *nd = q->elevator->elevator_data;
    struct list_head *cur_rq = NULL;
    int i=1;

    //print the list!
    list_for_each(cur_rq, &nd->queue) { 
        printk("[CLOOK] request %d: %lu\n", i, blk_rq_pos(list_entry(cur_rq, struct request, queuelist)));
        i++;
    }
}
#endif

static void clook_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int clook_dispatch(struct request_queue *q, int force)
{
	struct clook_data *nd = q->elevator->elevator_data;
    char read_or_write;

	if (!list_empty(&nd->queue)) {
		struct request *rq;
		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);

        // check for read or write request!
        read_or_write = (rq_data_dir(rq) & REQ_WRITE) 'W' : 'R';
        printk("[CLOOK] add %c %lu\n", read_or_write, blk_rq_pos(rq));
 
		return 1;
	}
	return 0;
}

static void clook_add_request(struct request_queue *q, struct request *rq)
{
	struct clook_data *nd = q->elevator->elevator_data;
    struct list_head *cur_rq = NULL;
    char read_or_write;

    #ifdef DEBUG
    printk("[CLOOK] Before add a new request!\n");
    my_print(q,rq);
    #endif

    // Find the right place for the new request
    list_for_each(cur_rq, &nd->queue) {
        if(rq_end_sector(list_entry(cur_rq, struct request, queuelist)) > rq_end_sector(rq)) {
            break;
        }
    }

    // check for read or write request!
    read_or_write = (rq_data_dir(rq) & REQ_WRITE) 'W' : 'R';
    printk("[CLOOK] add %c %lu\n", read_or_write, blk_rq_pos(rq));

    #ifdef DEBUG
    printk("[CLOOK] After add a new request!\n");
    my_print(q,rq);
    #endif


    list_add_tail(&rq->queuelist, cur_rq);
}

static struct request *
clook_former_request(struct request_queue *q, struct request *rq)
{
	struct clook_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
clook_latter_request(struct request_queue *q, struct request *rq)
{
	struct clook_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int clook_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct clook_data *nd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void clook_exit_queue(struct elevator_queue *e)
{
	struct clook_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_clook = {
	.ops = {
		.elevator_merge_req_fn		= clook_merged_requests,
		.elevator_dispatch_fn		= clook_dispatch,
		.elevator_add_req_fn		= clook_add_request,
		.elevator_former_req_fn		= clook_former_request,
		.elevator_latter_req_fn		= clook_latter_request,
		.elevator_init_fn		= clook_init_queue,
		.elevator_exit_fn		= clook_exit_queue,
	},
	.elevator_name = "clook",
	.elevator_owner = THIS_MODULE,
};

static int __init clook_init(void)
{
	return elv_register(&elevator_clook);
}

static void __exit clook_exit(void)
{
	elv_unregister(&elevator_clook);
}

module_init(clook_init);
module_exit(clook_exit);


MODULE_AUTHOR("GROUP 3");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CLOOK IO scheduler");
