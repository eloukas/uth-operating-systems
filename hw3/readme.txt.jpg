"slob.c" is the file that implements the SLOB First-Fit and Next Fit implementation.

pathfile of slob.c : /usr/src/linux-3.14.62-dev/mm/slob.c

Slob Heap = 
	1:set of linked list of pages from alloc_pages()
	2:Within each page, there is a singly-linked list of free blocks(slob_t)

To reduce fragmentation, heap pages are segregated into three(3) lists,
	1: with objects less than 256 bytes, 	( <256 bytes)
   	2: objects less than 1024 bytes,	( <1024 bytes)
	3: and all other objects.		( >1024 bytes)

How does allocation from heap work?
	1:First, search for a page with sufficient free blocks(next-fit approach)
	2:Then, scan by First-Fit algorithm the page.

How does de-allocation from heap work?
	Deallocation inserts objects back
 * into the free list in address order, so this is effectively an
 * address-ordered first fit.

How does SLOB work when it starts?
	Based upon the size, it finds the corresponding list (of all 3 lists).	
	Then, it traverses the chosen list to find a page that has free size so it can serve the request of the memory.As said previously, this is done with the next-fit algorithm(which equals to first-fit modified).

Unlike "First-Fit", which starts from the *beginning of the list*, "Next Fit" starts searching the list from the last position the algorithm finished previously.
Then, the algorithm returns the first page that can handle the request.
If there is no page that can handle the request, then one *new* page gets allocated.

When a page is found, it traverses the free blocks so a block with available memory for the request is to be found.The blocks are organised in a singly-linked list.
This gets done with the use of First-Fit algorithm, which means that the first block with available memory, is our chosen one.
REMINDER:The list of the blocks inside a page are address-ordered
Whenever we find our chosen block, we remove it from the list of the free blocks of this page and we return it to the process that created the request for the memory.

SLOB Cons:
	Using First-Fit(for block searching) and Next-Fit(for page searching) gives us a lot of external fragmentation.

TO-DO:
	Replace *both* algorithms used with Best-Fit.
Best-Fit should choose the block with the less available space that can serve the request of course.
So, as said, Best-Fit should be used for choosing the best page, as long as the best block inside this page.

Why?
	Best-Fit tends to decrease the external fragmentation, even though the success of the algorithms depend heavily on the requests.

