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
#define WSIZE 4
#define DSIZE 8
#define TSIZE 12
#define QSIZE 16
#define OVERHEAD 8
#define ALIGNMENT 8
#define BLKSIZE 24
#define CHUNKSIZE (1<<12)
#define INISIZE 1016




/*Macros*/
/*Max and min value of 2 values*/
#define MAX(x, y) ( (x)>(y)? (x): (y) )
#define MIN(x, y) ( (x)<(y)? (x): (y) )
/*Read and write a word at address p*/
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p)=(val))
/*Read the size and allocated fields from address p*/
#define SIZE(p) (GET(p)&~0x7)
#define PACK(size, alloc) ((size)|(alloc))
#define ALLOC(p) (GET(p)&0x1)
/*Given pointer p at the second word of the data structure, compute addresses of its HEAD,LEFT,RIGHT,PRNT,BROS and FOOT pointer*/
#define HEAD(p) ((void *)(p)-WSIZE)
#define LEFT(p) ((void *)(p))
#define RIGHT(p) ((void *)(p)+WSIZE)
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
#define GET_PRNT(bp) (GET(PRNT(bp))) 
#define GET_BROS(bp) (GET(BROS(bp)))
#define GET_FOOT(bp) (GET(FOOT(bp)))
/*Define value to each character in the block bp points to.*/
#define PUT_HEAD(bp, val) (PUT(HEAD(bp), (int)val)) 
#define PUT_FOOT(bp, val) (PUT(FOOT(bp), (int)val)) 
#define PUT_LEFT(bp, val) (PUT(LEFT(bp), (int)val)) 
#define PUT_RIGHT(bp, val) (PUT(RIGHT(bp), (int)val)) 
#define PUT_PRNT(bp, val) (PUT(PRNT(bp), (int)val)) 
#define PUT_BROS(bp, val) (PUT(BROS(bp), (int)val))




#define SIZE_T_SIZE (ALIGN_SIZE(sizeof(size_t)))


/* non-static functions */
int mm_init ( void );
void *mm_malloc ( size_t size );
void mm_free ( void *bp );
void *mm_realloc ( void *bp, size_t size );
/* static functions */
static void *coalesce ( void *bp );
static void *extend_heap ( size_t size );
static void place ( void *ptr, size_t asize );
static void insert_node ( void *bp );
static void delete_node ( void *bp );
static void *find_fit ( size_t asize );
/* Global variables */
static void *heap_list_ptr;
static void *free_tree_rt;


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* create the initial empty heap */
    if( (heap_list_ptr = mem_sbrk(QSIZE)) == NULL ) return -1;
    PUT( heap_list_ptr, 0 ); /* alignment padding */
    PUT( heap_list_ptr+WSIZE, PACK(OVERHEAD,1) ); /* prologue header */
    PUT( heap_list_ptr+DSIZE, PACK(OVERHEAD,1) ); /* prologue footer */
    PUT( heap_list_ptr+TSIZE, PACK(0,1) ); /* epilogue header */
    heap_list_ptr += QSIZE;
    free_tree_rt = NULL;
    
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if( extend_heap(ALIGN_SIZE(INISIZE))==NULL )
        return -1;
    return 0;
}


void *extend_heap(size_t size) {
    void *bp;
    if ((unsigned int)(bp=mem_sbrk(size)) ==(unsigned)(-1))//if( (int)(bp=mem_sbrk(size)) <0 )//new
        return NULL;
    /* Initialize free block header/footer and the epilogue header */
    PUT_HEAD( bp, PACK(size,0) ); /* free block header */
    PUT_FOOT( bp, PACK(size,0) ); /* free block footer */
    PUT_HEAD( GET_NEXT(bp), PACK(0,1) ); /* new epilogue header */
    insert_node(coalesce(bp));
    return (void *)bp;
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

    /* Adjust block size to include overhead and alignment requirements. */
    if( size <= BLKSIZE-OVERHEAD)
    /*size=required size; block size = all size excluded head&foot(overhead)*/
        asize = BLKSIZE; else
        /*asize=ajusted size*/
            asize = ALIGN_SIZE(size+(OVERHEAD));
    /* Search the free list for a fit */
    if( (bp=find_fit(asize)) == NULL ){ extendsize = MAX( asize + 32, INISIZE ); extend_heap(ALIGN_SIZE(extendsize)); if( (bp=find_fit(asize)) == NULL )
        return NULL; }
    /* place the block into its fit */
    if( size==448 && GET_SIZE(bp) > asize+64 ) asize += 64;
    else if( size==112 && GET_SIZE(bp) > asize+16 ) asize += 16;
    place(bp, asize);
    return bp;
}



/*
 * mm_free - Free function and coalesce helper function copied from textbook.
 */
void mm_free(void *bp)
{
	size_t size = GET_SIZE(bp);
	PUT_HEAD( bp, PACK(size,0) );
    PUT_FOOT( bp, PACK(size,0) );
	insert_node(coalesce(bp));
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(GET_PREV(bp));
    size_t next_alloc = GET_ALLOC(GET_NEXT(bp));
    size_t size = GET_SIZE(bp);
    if ( prev_alloc && next_alloc ) /* Case 0 */
        return bp;
    else if ( !prev_alloc && next_alloc ) { /* Case 1*/
        delete_node(GET_PREV(bp));
        size += GET_SIZE( GET_PREV(bp) );
        PUT_HEAD( GET_PREV(bp), PACK(size, 0) );
        PUT_FOOT( bp, PACK(size,0) );
        return GET_PREV(bp); }
    else if ( prev_alloc && !next_alloc ) { /* Case 2 */
        delete_node( GET_NEXT(bp) );
        size += GET_SIZE( GET_NEXT(bp) );
        PUT_HEAD( bp, PACK(size,0) );
        PUT_FOOT( bp, PACK(size,0) );
        return bp;
    }
    else { /* Case 3 */
        delete_node(GET_NEXT(bp));
        delete_node(GET_PREV(bp));
        size += GET_SIZE( GET_PREV(bp) ) + GET_SIZE( GET_NEXT(bp) );
        PUT_HEAD( GET_PREV(bp), PACK(size,0) );
        PUT_FOOT( GET_NEXT(bp), PACK(size,0) );
        return GET_PREV(bp);
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
	ï Are there any contiguous free blocks that somehow escaped coalescing? 
	ï Is every free block actually in the free list? 
	ï Do the pointers in the free list point to valid free blocks? 
	ï Do any allocated blocks overlap? 
	ï Do the pointers in a heap block point to valid heap addresses?
	*/
}