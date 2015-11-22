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
    /* Team name */
    "ateam",
    /* First member's full name */
    "LuoLei Zhao",
    /* First member's email address */
    "luoleizhao2018@u.northwestern.edu",
    /* Second member's full name (leave blank if none) */
    "Yue Hu",
    /* Second member's email address (leave blank if none) */
    "yuehu2018@u.northwestern.edu"
};

/* single word (4) or double word (8) alignment */
#define SSIZE 4
#define DSIZE 8
#define TSIZE 12
#define QSIZE 16
#define OVERHEAD 8 #define ALIGNMENT 8 #define BLKSIZE 24
#define CHUNKSIZE (1<<12) 
#define INISIZE 1016




/*Macros*/
/*Max and min value of 2 values*/
#define MAX(x, y) ( (x)>(y)? (x): (y) ) #define MIN(x, y) ( (x)<(y)? (x): (y) )
/*Read and write a word at address p*/
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p)=(val))
/*Read the size and allocated fields from address p*/
#define SIZE(p) (GET(p)&~0x7) #define PACK(size, alloc) ((size)|(alloc)) #define ALLOC(p) (GET(p)&0x1)
/*Given pointer p at the second word of the data structure, compute addresses of its HEAD,LEFT,RIGHT,PRNT,BROS and FOOT pointer*/
#define HEAD(p) ((void *)(p)-SSIZE)
#define LEFT(p) ((void *)(p))
#define RIGHT(p) ((void *)(p)+SSIZE)
#define PRNT(p) ((void *)(p)+DSIZE)
#define BROS(p) ((void *)(p)+TSIZE)
#define FOOT(p) ((void *)(p)+SIZE(HEAD(p))-DSIZE)
/*Make the block to meet with the standard alignment requirements*/
#define ALIGN_SIZE(size) (((size) + (ALIGNMENT-1)) & ~0x7)
/*Given block pointer bp, get the POINTER of its directions*/
#define GET_SIZE(bp) ((GET(HEAD(bp)))&~0x7)
#define GET_PREV(bp) ((void *)(bp)-SIZE(((void *)(bp)-DSIZE))) 
#define GET_NEXT(bp) ((void *)(bp)+SIZE(HEAD(bp)))
#define GET_ALLOC(bp) (GET(HEAD(bp))&0x1)
/*Get the LEFT,RIGHT,PRNT,BROS and FOOT pointer of the block to which bp points*/
#define GET_LEFT(bp) (GET(LEFT(bp)))
#define GET_RIGHT(bp) (GET(RIGHT(bp)))
#define GET_PRNT(bp) (GET(PRNT(bp))) #define GET_BROS(bp) (GET(BROS(bp))) 
#define GET_FOOT(bp) (GET(FOOT(bp)))
/*Define value to each character in the block bp points to.*/
#define PUT_HEAD(bp, val) (PUT(HEAD(bp), (int)val)) 
#define PUT_FOOT(bp, val) (PUT(FOOT(bp), (int)val)) 
#define PUT_LEFT(bp, val) (PUT(LEFT(bp), (int)val)) 
#define PUT_RIGHT(bp, val) (PUT(RIGHT(bp), (int)val)) 
#define PUT_PRNT(bp, val) (PUT(PRNT(bp), (int)val)) 
#define PUT_BROS(bp, val) (PUT(BROS(bp), (int)val))



/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	memlib::mem_init();
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	size_t asize; /* adjusted block size */
	size_t extendsize; /* amount to extend heap if no fit */
	char *bp;

	/* Ignore spurious requests */
	if (size <= 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = DSIZE + OVERHEAD;
	else
		asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);

	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}

	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize,CHUNKSIZE);
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	place(bp, asize);
	return bp;
}

/*
 * mm_free - Free function and coalesce helper function copied from textbook.
 */
void mm_free(void *bp)
{
	size_t size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	coalesce(bp);
}

static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	if (prev_alloc && next_alloc) { /* Case 1 */
		return bp;
	}
	else if (prev_alloc && !next_alloc) { /* Case 2 */
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
		return(bp);
	}
	else if (!prev_alloc && next_alloc) { /* Case 3 */
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		return(PREV_BLKP(bp));
	}
	else { /* Case 4 */
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
		GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		return(PREV_BLKP(bp));
	}
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

void mm_check() 
{
	/* Is every block in the free list marked as free? 
	• Are there any contiguous free blocks that somehow escaped coalescing? 
	• Is every free block actually in the free list? 
	• Do the pointers in the free list point to valid free blocks? 
	• Do any allocated blocks overlap? 
	• Do the pointers in a heap block point to valid heap addresses?
	*/
}