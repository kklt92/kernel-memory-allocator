/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the power-of-two free list
 *             algorithm
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
#ifdef KMA_P2FL
#define __KMA_IMPL__


#define HEADERSIZE 10
#define MINBLKSIZE 16
#define MINBLK 4

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

/************External Declaration*****************************************/

/**************Implementation***********************************************/

  
struct free_block {
  struct free_block  *next;
};

struct page_header {
  kma_page_t *page;
};

struct list_header {
  int size;
  int avai_size;
  struct free_block *blk;
};

struct p2fl_controller {
  int used;
  int free;
  struct list_header lh[HEADERSIZE];
};

void list_insert(struct free_block *block, struct free_block  *l) {
  if(l->next == NULL) {
    l->next = block;
  }
  else {
    block->next = l->next;
    l->next = block;
  }
}




void init_page_entry() {
  struct page_header *header;
  struct p2fl_controller *control;
  struct list_header *curr;
  struct free_block *temp;
  page_entry = get_page();

  header = (struct page_header*)page_entry->ptr;
  control = (struct p2fl_controller*)((char*)page_entry->ptr + sizeof(struct page_header));

  header->page = page_entry;
  control->used = 0;
  control->free = 0;
  
  int i=0;
  for(i=0; i<HEADERSIZE; i++) {
    control->lh[i].size = (int)(pow((double)2,(double) (i+4)));
    control->lh[i].avai_size = control->lh[i].size - sizeof(struct free_block);
    temp = (struct free_block*)((char*)page_entry->ptr + sizeof(struct page_header) 
        + sizeof(struct p2fl_controller) + i * (sizeof(struct free_block)));
    control->lh[i].blk = temp;
    control->lh[i].blk->next = NULL;
  }
}

void *plfl_info() {
  void *ptr;
  ptr = (struct p2fl_controller*)((char*)page_entry->ptr + sizeof(struct page_header));

  return ptr;
}

void new_free_block(struct list_header *l) {
  struct free_block *curr;
  struct page_header *header;
  kma_page_t *page;
  void *page_end_addr;

  page = get_page();
  header = (struct page_header*)page->ptr;
  header->page = page;
  page_end_addr = (char*)header + PAGESIZE;
  curr = (char*)header + sizeof(struct page_header);
  curr->next = NULL;
  
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

void *mem_allocate(kma_size_t size) {
  struct p2fl_controller *control;
  void *ptr;
  int  i=0;
  
  control = plfl_info();

  for(i = 0; i<HEADERSIZE; i++) {
    if(size <= control->lh[i].avai_size) {
      if(control->lh[i].blk->next == NULL) {
        new_free_block(&control->lh[i]);
      }
      ptr = (void*)control->lh[i].blk->next;

    
      control->lh[i].blk->next = control->lh[i].blk->next->next;
      
      control->used++;
      return ptr;

    }
  }
}


void*
kma_malloc(kma_size_t size)
{
  if(page_entry == NULL)
    init_page_entry();

  return mem_allocate(size);
}

void
kma_free(void* ptr, kma_size_t size)
{
  struct p2fl_controller *control;
  struct page_header *header;
  struct free_block *curr;
  int i=0;

  control = plfl_info();

  for(i=0; i<HEADERSIZE; i++) {
    if(size <= control->lh[i].avai_size ) {
      curr = ptr;
      curr->next = NULL;
      list_insert(curr, (control->lh[i].blk));
      break;
    }
  }

  control->free++;

  if(control->used == control->free) {
  } 


}

#endif // KMA_P2FL
