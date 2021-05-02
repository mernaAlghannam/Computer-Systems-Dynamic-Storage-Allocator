/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* bu username : eg. jappavoo */
    "maalghan",
    /* full name : eg. jonathan appavoo */
    "Merna Alghannam",
    /* email address : jappavoo@bu.edu */
    "maalghan@bu.edu",
    "",
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */ 
#define	WSIZE	4 /* Word and header/footer size (bytes) */ 
#define DSIZE	8 /* Double word size (bytes) */ 
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */ 
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */ 
#define GET(p)		(*(unsigned int *) (p)) 
#define PUT(p, val)	(*(unsigned int *) (p) = (val))

/* Read the size and allocated fields from address p */ 
#define GET_SIZE(p)	(GET (p) & ~0x7) 
#define GET_ALLOC(p)	(GET (p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */ 
#define HDRP(bp)	((char *) (bp) - WSIZE) 
#define FTRP(bp)	((char *) (bp) + GET_SIZE (HDRP (bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */ 
#define NEXT_BLKP(bp)	((char *) (bp) + GET_SIZE(((char *) (bp) - WSIZE))) 
#define PREV_BLKP(bp)	((char *) (bp) - GET_SIZE(((char *) (bp) - DSIZE)))

void *heap_listp;

int mm_init(void);
static void *extend_heap(size_t words);
void mm_free(void *bp);
static void *coalesce(void *bp);
void *mm_malloc(size_t size);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
void *mm_realloc(void *ptr, size_t size);

/*
	Paramenter: void
	It returns an integer..

	The purpose of this function is to initialize the heap
	with the prologue header and footer.
*/
int mm_init(void)
{
	/* Create the initial empty heap */ 
	if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
		return -1; 
	PUT(heap_listp, 0);				/* Alignment padding */ 
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));	/* Prologue header */ 
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));	/* Prologue footer */ 
	PUT(heap_listp + (3*WSIZE), PACK(0, 1));	/* Epilogue header */ 
	heap_listp += (2*WSIZE);	/*make the heap point after the prologue h and f*/

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */ 
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1; 
	return 0;
}

/*
	Paramenter: *ptr, size
	It returns a void pointer.

	The purpose of this function is to extend the heap
*/
static void *extend_heap(size_t words)
{
	char *bp; 
	size_t size;

	/* Allocate an even number of words to maintain alignment */ 
	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 
	if ((long) (bp = mem_sbrk(size)) == -1)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */ 
	PUT(HDRP (bp), PACK(size, 0));			/* Free block header */ 
	PUT(FTRP (bp), PACK(size, 0));			/* Free block footer */ 
	PUT(HDRP (NEXT_BLKP(bp)), PACK(0, 1));		/* New epilogue header */

	/* Coalesce if the previous block was free */ 
	return coalesce(bp);
}

/*
	Paramenter: *ptr, size
	It returns a void pointer.

	The purpose of this function is to free the block
	the pointer is pointing at and coalesce
*/
void mm_free(void *bp)
{
	// gets the size in header of block
	size_t size = GET_SIZE(HDRP(bp));

	// frees the block
	PUT(HDRP(bp), PACK(size, 0)); 
	PUT(FTRP(bp), PACK(size, 0)); 
	//combine with free blocks next to it
	coalesce(bp);
}

/*
	Paramenter: *ptr, size
	It returns a void pointer.

	The purpose of this function is to merge free block
	if they are next to eachother.
*/
static void *coalesce(void *bp)
{
	//initialize variable for allocation of prev and next blocks
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); 
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); 
	//gets block size from header
	size_t size = GET_SIZE(HDRP(bp));

	//if the prev and next block are both allocated
	if(prev_alloc && next_alloc) {			/* Case 1 */
		return bp;
	}	// the prev is allocated and next is not
	else if (prev_alloc && ! next_alloc) {		/* Case 2 */
		// increase the size in block to include the size of next block
		size += GET_SIZE(HDRP(NEXT_BLKP(bp))); 
		//combines them together
		PUT(HDRP(bp), PACK(size, 0)); 
		PUT(FTRP(bp), PACK(size, 0));
	}	// the prev is not allocated and next is
	else if (!prev_alloc && next_alloc) {		/* Case 3 */
		// incraese the size to include the prev block
		size += GET_SIZE(HDRP(PREV_BLKP(bp))); 
		//combined them together
		PUT(FTRP(bp), PACK(size, 0)); 
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
		//point bp to prev block as it is its new beginning
		bp = PREV_BLKP(bp);
	}	//if both are free
	else {						/* Case 4 */ 
		//incraese size to include prev and next
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); 
		//comine them
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		//point to new beginning from block
		bp = PREV_BLKP(bp); 
	} 
	//return the pointer
	return bp;
}

/*
	Paramenter: size
	It returns a void pointer.

	The purpose of this function is to allocate a block with the 
	given size. You must modify few paramenter and if no space expand 
	heap.
*/
void *mm_malloc(size_t size)
{
	size_t asize;		/* Adjusted block size */ 
	size_t extendsize;	/* Amount to extend heap if no fit */ 
	char *bp;

	/* Ignore spurious requests */ 
	if(size == 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */ 
	if(size <= DSIZE)
		asize = 2*DSIZE; 
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

	/* Search the free list for a fit */ 
	if((bp = find_fit(asize)) != NULL) {
		place (bp, asize); 
		return bp;
	}

	/* No fit found. Get more memory and place the block */ 
	extendsize = MAX(asize, CHUNKSIZE); 
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	//place the block after extending 
	place (bp, asize);
	return bp;
}

/*
	Paramenter: asize
	It returns nothing.

	The purpose of this function is to find the first block in the 
	heapthat fits the size you want.
*/
static void *find_fit(size_t asize)
{
	/* First-fit search */
	void *bp;
	//loops through the heap until it find a fit
	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
		if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
			return bp;
		}
	}
	return NULL; /* Otherwise, No fit */
}

/*
	Paramenter: *bp, asize
	It returns nothing.

	The purpose of this function is to place the block in it place
	with the size it needs to be. If the size of the block is larger 
	than the size needs to be place. Then, they split the block. 
	Otherwise, just place
*/
static void place(void *bp, size_t asize)
{
	//get the size in header
	size_t csize = GET_SIZE(HDRP(bp));

	// diff in size is larger than dsize,
	//then seperate the block
 	if ((csize - asize) >= (2*DSIZE)) {
 		PUT(HDRP(bp), PACK(asize, 1));
 		PUT(FTRP(bp), PACK(asize, 1));
 		bp = NEXT_BLKP(bp);
		//serate the block by freeing rest
 		PUT(HDRP(bp), PACK(csize-asize, 0));
 		PUT(FTRP(bp), PACK(csize-asize, 0));
 	}
 	else {
		 //just allocate with size as it it
		 //without splitting
		PUT(HDRP(bp), PACK(csize, 1));
 		PUT(FTRP(bp), PACK(csize, 1));
 	}
 }

/*
	Paramenter: *ptr, size
	It returns a void pointer.

	The purpose of this function is to change the size of a block
	that is chosen to a size that is smaller or larger than original size.
*/
void *mm_realloc(void *ptr, size_t size)
{
	//initialize the old ptr and new
    void *oldptr = ptr;
    void *newptr;
	//add size that is in block
    size_t copySize = GET_SIZE(HDRP(oldptr));
	//initiaze new size 
	size_t newSize = size;

	/* Adjust block size to include overhead and alignment reqs. */ 
	if(size <= DSIZE)
		newSize = 2*DSIZE; 
	else
		newSize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
	
	//if they are equal, just return
	if(size == copySize)
		return oldptr;

	//if the new size is smaller
	if(newSize < copySize){
		//just place the block and free 
		PUT(HDRP(oldptr), PACK(newSize, 1));
 		PUT(FTRP(oldptr), PACK(newSize, 1));
 		//frees the rest
 		PUT(HDRP(NEXT_BLKP(oldptr)), PACK(copySize-newSize, 0));
 		PUT(FTRP(NEXT_BLKP(oldptr)), PACK(copySize-newSize, 0));
		// then combined with other free blocks
		coalesce(NEXT_BLKP(oldptr));

		return oldptr;
	}else{
		//get new block with size you want
		newptr = mm_malloc(size);
		
		//if there is no new ptr
		if (newptr == NULL)
			return NULL;

		//unnessary, but it does not work without it
		copySize = GET_SIZE(HDRP(oldptr));
		if (size < copySize)
			copySize = size;
			
		//copy memory to the new pointer with the info of the
		//smaller block
		memcpy(newptr, oldptr, copySize);
		//free the old one
		mm_free(oldptr);

		return newptr;
	}
}

/*
	Paramenter: n, size, and copysize
	It does  not return anything.

	The purpose of this function is to print the heap for testing with a lot of the content
	that would benefit me in debugging.
*/
void mm_checker(int n, int size, int copySize){
	//initialize the pointer for the loop
	void *bp;

	//loops through the heap unil the end
	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
		//prints its content that would be helpful for debugging
		printf("# %d block: %p allocation: %d size in footer: %d size in header: %d "
			"size: %d copySize: %d \n", n, bp, GET_ALLOC(HDRP(bp)), GET_SIZE(HDRP(bp)), 
				GET_SIZE(FTRP(bp)), size, copySize);
		printf("next_size: %d alloc next: %d \n", GET_SIZE(HDRP(NEXT_BLKP(bp))), 
				GET_ALLOC(HDRP(NEXT_BLKP(bp))));
	}
	//to seperate the heap prints for better visualizeation
	printf("\n");
}