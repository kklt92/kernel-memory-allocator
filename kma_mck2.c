/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the McKusick-Karels
 *              algorithm
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_MCK2
#define __KMA_IMPL__



#define HEADERSIZE 10
#define MINBLKSIZE 16
#define MINBLK 4
#define KMPAGESIZE 2500
/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <math.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

static kma_page_t *page_entry = NULL;

/************Function Prototypes******************************************/

void list_insert(struct free_block*, struct free_block*);
/************External Declaration*****************************************/

/**************Implementation***********************************************/

/* smallest unit. used in the begin of each free block. 
 * to indicate the next same type free block.
 **/
struct free_block {
  struct free_block  *next;
};

struct page_header {
  kma_page_t *page;
};

struct kmem_page_header {
  kma_page_t *page;
  int size;
};

struct list_header {
  int size;
  struct free_block *blk;
};

struct advance_page {
  int id;
  void* ptr;
  int size;
  void* addr;
};

struct mck2_controller {
  int used;
  int free;
  struct advance_page add_kmem_page[10];
  struct list_header freelistarr[HEADERSIZE];
  struct kmem_page_header kmemsizes[KMPAGESIZE];
};


void init_page_entry() {
  struct page_header *header;
  struct mck2_controller *control;
  struct free_block *temp;
  kma_page_t *tempPage;
  int i=0;
  page_entry = get_page();

  header = (struct page_header*)page_entry->ptr;
  control = (struct mck2_controller*)((char*)page_entry->ptr + sizeof(struct page_header));

  header->page = page_entry;
  control->used = 0;
  control->free = 0;
  
  for(i=0; i<KMPAGESIZE/(PAGESIZE/sizeof(struct kmem_page_header)); i++) {
      tempPage = get_page();
      control->add_kmem_page[i].id = tempPage->id;
      control->add_kmem_page[i].ptr = tempPage->ptr;
      control->add_kmem_page[i].size = tempPage->size;
      control->add_kmem_page[i].addr = (void*)tempPage;
  }
  /* initial all the struct in the list */
  for(i=0; i<HEADERSIZE; i++) {
    control->freelistarr[i].size = (int)(pow((double)2,(double) (i+4)));
    temp = (struct free_block*)((char*)page_entry->ptr + sizeof(struct page_header) 
        + sizeof(struct mck2_controller) + i * (sizeof(struct free_block)));
    control->freelistarr[i].blk = temp;
    control->freelistarr[i].blk->next = NULL;
  }
  
  /* initial kmemsize[]. size = 0 means unallocated. */
  for(i=0; i<KMPAGESIZE; i++) {
    control->kmemsizes[i].size = 0;
    control->kmemsizes[i].page = NULL;
  }
  
}


/* get controller infomation */
void *mck2_info() {
  void *ptr;
  ptr = (struct mck2_controller*)((char*)page_entry->ptr + sizeof(struct page_header));

  return ptr;
}



/* get a new page to store free block */
void new_free_block(struct list_header *l) {
  struct mck2_controller *control;
  struct free_block *curr;
  kma_page_t *page;
  void *page_end_addr;

  control = mck2_info();

  /* initial a new page. */
  page = get_page();
  int i=0;
  for(i=0; i<KMPAGESIZE; i++) {
    if(control->kmemsizes[i].size == 0) {
      control->kmemsizes[i].page = page;
      control->kmemsizes[i].size = l->size;
      break;
    }
  }
  page_end_addr = (char*)(control->kmemsizes[i].page->ptr) + PAGESIZE;
  curr = (struct free_block *)(char*)(control->kmemsizes[i].page->ptr);
  curr->next = (struct free_block *)NULL;
  
  /* divide this page into the same size of free block as request. */
  if(l->size == 8192) {
    list_insert(curr, l->blk);

  }
  else {
    while((char*)curr + l->size < (char*)page_end_addr) {
      curr->next = NULL;
      list_insert(curr, l->blk);
      curr = (struct free_block*)((char*)curr + l->size);
    }
  }
  
}

void list_insert(struct free_block *block, struct free_block  *l) {
  if(l->next == NULL) {
    l->next = block;
  }
  else {
    block->next = l->next;
    l->next = block;
  }
}

void *mem_allocate(kma_size_t size) {
  struct mck2_controller *control;
  void *ptr;
  int  i=0;
  
  control = mck2_info();

  for(i = 0; i<HEADERSIZE; i++) {
    if(size <= control->freelistarr[i].size) {
      if(control->freelistarr[i].blk->next == NULL) {
        new_free_block(&control->freelistarr[i]);
      }
      ptr = (void*)control->freelistarr[i].blk->next;
    
      control->freelistarr[i].blk->next = control->freelistarr[i].blk->next->next;
      
      control->used++;
      return ptr;

    }
  }
  return NULL;
}


void*
kma_malloc(kma_size_t size)
{
  if(size >= PAGESIZE) 
    return NULL;
  if(page_entry == NULL)
    init_page_entry();


  return mem_allocate(size);
}

void
kma_free(void* ptr, kma_size_t size)
{
  
  struct mck2_controller *control;
  struct free_block *curr ;
  kma_page_t *tempPage;
  int i=0;

  control = mck2_info();

  /* free specific memory and re-add it to original list */
  for(i=0; i<HEADERSIZE; i++) {
    if(size <= control->freelistarr[i].size ) {
      curr = ptr;
      curr->next = NULL;
      list_insert(curr, (control->freelistarr[i].blk));
      break;
    }
  }

  control->free++;

  /* free all the page when request memory number = free memory number. */
  if(control->used == control->free) {
    for(i=0; i<KMPAGESIZE; i++) {
      if(control->kmemsizes[i].size != 0) {
        free_page(control->kmemsizes[i].page);
      }
    }
    i = 0;
    for(i=0; i<KMPAGESIZE/(PAGESIZE/sizeof(struct kmem_page_header)); i++) {
      tempPage = (kma_page_t*)control->add_kmem_page[i].addr;
      free_page(tempPage);
    } 
    free_page(page_entry);
  
    page_entry = NULL;
  } 

}

#endif // KMA_MCK2
