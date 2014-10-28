/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
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
#ifdef KMA_BUD
#define __KMA_IMPL__

/* minimal free block size is 32. */
#define MIN_BLK_SIZE 32
#define HEADERSIZE 9
#define FREE_PAGE -1
/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>

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
  struct free_block *blk;
};

struct page_node {
  int bitmap[(PAGESIZE/MIN_BLK_SIZE)/(sizeof(int)*8)];
  int id;
  void* ptr;
  int size;
  void* addr;
  struct page_node *prev;
  struct page_node *next;
};
  

struct bud_controller {
  int used;
  int free;
  struct page_node node_page_list;
  struct list_header freelist[HEADERSIZE];
  struct page_node page_list;
};

/* function for bit map */
void set_bit(int A[], int k) {
  int i = k/(sizeof(int)*8);
  int pos = k%(sizeof(int)*8);

  unsigned int flag = 1;

  flag = flag << pos;

  A[i] = A[i] | flag;
}

void clear_bit(int A[], int k) {
  int i = k/(sizeof(int)*8);
  int pos = k%(sizeof(int)*8);

  unsigned int flag = 1;
  flag = flag << pos;
  flag = -flag;

  A[i] = A[i] & flag;
}

int get_bit(int A[], int k) {
  int i = k/(sizeof(int)*8);
  int pos = k%(sizeof(int)*8);

  unsigned int flag = 1;

  flag = flag << pos;

  if(A[i] & flag)
    return 1;
  else
    return 0;
}


void list_blk_insert(struct free_block *block, struct free_block  *l) {
  if(l->next == NULL) {
    l->next = block;
  }
  else {
    block->next = l->next;
    l->next = block;
  }
}

void node_list_append(struct page_node *newNode, struct page_node *list) {
  newNode->prev = list->prev;
  newNode->next = list;
  list_prev = newNode;
  newNode->prev->next = newNode;
  
}


/* get controller infomation */
void *bud_info() {
  void *ptr;
  ptr = (struct bud_controller*)((char*)page_entry->ptr + sizeof(struct page_header));

  return ptr;
}


void init_page_entry() {
  struct page_header *header;
  struct bud_controller *control;
  struct free_block *temp;
  void *page_end_addr;
  struct page_node *currNode;
  page_entry = get_page();

  header = (struct page_header*)page_entry->ptr;
  control = (struct bud_controller*)((char*)page_entry->ptr + sizeof(struct page_header));
  page_end_addr = (void*)((char*)page_entry->ptr + PAGESIZE);

  header->page = page_entry;
  control->used = 0;
  control->free = 0;
  
  /* initial all the struct in the list */
  int i=0;
  for(i=0; i<HEADERSIZE; i++) {
    control->freelist[i].size = (int)(pow((double)2,(double) (i+5)));
    temp = (struct free_block*)((char*)page_entry->ptr + sizeof(struct page_header) 
        + sizeof(struct bud_controller) + i * (sizeof(struct free_block)));
    control->freelist[i].blk = temp;
    control->freelist[i].blk->next = NULL;
  }

  
  for(i=0; i<(PAGESIZE/MIN_BLK_SIZE)/(sizeof(int)*8); i++) {
    control->page_list.bitmap[i] = 0
  }
    
  control->page_list.size = 0;
  control->page_list.id = temp;
  control->page_list.addr = NULL;    
  control->page_list.ptr = NULL;
  control->page_list.prev = &(control->page_list);
  control->page_list.next = &(control->page_list);

  currNode = (struct page_node)((char*)temp + sizeof(struct free_block));
  while(currNode + 1 <= page_end_addr) {
    currNode->size = FREE_PAGE;
    node_list_append(currNode, &(control->page_list));
    currNode++;
  }

}


/* get a new page to store free block */
void new_free_block(struct list_header *l) {
  struct bud_controller *control;
  struct free_block *curr;
  struct page_header *header;
  kma_page_t *page;
  void *page_end_addr;

  control = plfl_info();

  /* initial a new page. */
  page = get_page();
  header = (struct page_header*)page->ptr;
  header->page = page;
  page_end_addr = (char*)page->ptr + PAGESIZE;
  curr = (struct free_block *)(char*)header + sizeof(struct page_header);
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
  
  /* add this page into page_list */
  curr = (void*)page;
  curr->next = NULL;
  list_insert(curr, control->page_list.blk);

  

}

void new_free_page() {
  struct bud_controller *control;
  struct page_node *currNode;
  kma_page_t *page;

  control = bud_info();
  
  page = get_page();

  controlr->page_list.next;
}



void*
kma_malloc(kma_size_t size)
{
  return NULL;
}

void 
kma_free(void* ptr, kma_size_t size)
{
  ;
}

#endif // KMA_BUD
