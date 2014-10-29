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
#define MAPSIZE (PAGESIZE/MIN_BLK_SIZE)/(sizeof(int)*8)
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
  struct page_node   *node;    // point back to page node to access bit map.
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
  void* node_list_page[15];
  struct list_header freelist[HEADERSIZE];
  struct page_node page_list;
};

struct free_block *make_free_block(void *ptr, struct page_node *pnode) {
  struct free_block *blk;

  blk = (struct free_block*)ptr;
  blk->next = NULL;
  blk->node = pnode;

  return blk;
}

void *current_page_begin_addr(void *addr) {
  return (void*)((unsigned long)addr & ~((unsigned long)(PAGESIZE-1)));
}

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

void reset_bitmap(int A[]) {
  int i;

  for(i=0; i<MAPSIZE; i++) {
    A[i] = 0;
  }
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

void node_list_append(struct page_node *newNode, struct page_node *prevNode) {
  newNode->prev = prevNode;
  newNode->next = NULL;
  prevNode->next = newNode;
  
}

void blk_remove(struct free_block *block, struct free_block *prevBlk) {
  prevBlk->next = block->next;
  block->next = NULL;
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
  struct page_node *currNode, *prevNode;
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
    control->freelist[i].blk->node = NULL;
  }


  for(i=0; i<15;i++) {
    control->node_list_page[i] = NULL;
  }

  
  for(i=0; i<(PAGESIZE/MIN_BLK_SIZE)/(sizeof(int)*8); i++) {
    control->page_list.bitmap[i] = 0;
  }
    
  control->page_list.size = 0;
  control->page_list.id = temp;
  control->page_list.addr = NULL;    
  control->page_list.ptr = NULL;
  control->page_list.prev = &(control->page_list);
  control->page_list.next = &(control->page_list);

  currNode = (struct page_node*)((char*)temp + sizeof(struct free_block));

  prevNode = &(control->page_list);
  while(currNode + 1 <= page_end_addr) {
    currNode->size = FREE_PAGE;
    node_list_append(currNode, prevNode);
    prevNode = currNode;
    currNode++;
  }
  prevNode->next =  NULL;

}

/***
 * set a new node page when every node in page_list is used.
 **/
void set_new_node_page() {
  struct bud_controller *control;
  struct page_node *currNode, *prevNode, *page_end_addr;
  struct page_header *header;
  kma_page_t *page;

  control = bud_info();
  page = get_page();

  header = (struct page_header*) page->ptr;
  header->page = page;

  currNode = (struct page_node*)((char*)header + sizeof(struct page_header));
  page_end_addr = (struct page_node*)((char*)header + PAGESIZE);

  prevNode = control->page_list.prev;
  while(currNode + 1< page_end_addr) {
    currNode->size = FREE_PAGE;
    node_list_append(currNode, prevNode);
    prevNode = currNode;
    currNode++;
  }
  prevNode->next = NULL;

  int i = 0;
  for(i=0;i<15;i++) {
    if(control->node_list_page[i] == NULL) {
      control->node_list_page[i] = (void*)page;
    }
  }
}

/**
 * when there are no free block in the block list
 * call this function to allocate a new page in the 
 * page node. and use it to fit the request memory.
 **/
void new_free_page() {
  struct bud_controller *control;
  struct page_node *currNode, *prevNode;
  struct free_block *blk;
  kma_page_t *page;

  control = bud_info();
  
  page = get_page();

  prevNode = control->page_list.prev;

  if(prevNode->next == NULL) {
    set_new_node_page();
  }
  currNode = prevNode->next;
  currNode->addr = page;
  currNode->ptr = page->ptr;
  currNode->size = page->size;
  currNode->id = page->id;
  reset_bitmap(currNode->bitmap);

  blk = make_free_block(currNode->ptr, currNode);

  /* point page_list.prev to the end of new page_list */
  control->page_list.prev = currNode;
}


/**
 * reduce free block size to fit request memory size.
 * Add extra space to free list array.
 **/
void resize_block(kma_size_t reqSize, void *ptr, int blkSize) {
  struct bud_controller *control;
  struct free_block *temp, *curr;
  void *temp_ptr;
  int offset;
  int i=0;

  control = bud_info();

  curr = (struct free_block*)ptr;

  offset = ((char*)ptr - (char*)curr->node->ptr)/MIN_BLK_SIZE;

  while(blkSize/reqSize >= 2) {
    blkSize = blkSize/2;
    temp_ptr = (void*)((char*)ptr + blkSize);
    for(i=HEADERSIZE - 1; i > -1; i--) {
      if(control->freelist[i].size == blkSize) {
        temp = make_free_block(temp_ptr, curr->node);
        list_blk_insert(temp, control->freelist[i].blk);
        break;
      }
    }
  }

  for(i=0; i<=(reqSize-1)/MIN_BLK_SIZE; i++) {
    set_bit(curr->node->bitmap, i+offset);
  }
}


void *allocate_mem(kma_size_t size) {
  struct page_node *currNode;
  struct bud_controller *control;
  void *ptr;
  kma_page_t *page;
  int i=0;

  control = bud_info();

  control->used++;

  for(i=0; i<HEADERSIZE; i++) {
    if(size <= control->freelist[i].size) {
      if(control->freelist[i].blk->next == NULL) {
        continue;
      }
      else {
        ptr = (void*)control->freelist[i].blk->next;
        control->freelist[i].blk->next = control->freelist[i].blk->next->next;
        resize_block(size, ptr, control->freelist[i].size);
        return ptr;
      }
    }
  }
  new_free_page();
  ptr = control->page_list.prev->ptr;
  resize_block(size, ptr, (int)PAGESIZE);

  return ptr;
}

void coalescing(void *ptr, int blkSize, struct page_node *currNode) {
  struct bud_controller *control;
  struct free_block *blk, *prevBlk;

  int offset;
  int blkOffset;
  int remainder;
  void* prime_ptr;
  int free = 1;
  int i=0;
  int p=0;


  control = bud_info();
  
  offset = ((char*)ptr - (char*)currNode->ptr)/MIN_BLK_SIZE;
  blkOffset = blkSize / MIN_BLK_SIZE;

  remainder = offset % (2 * blkOffset);

  for(p=0;p<HEADERSIZE; p++) {
    if(control->freelist[p].size == blkSize) {
      break;
    }
  }

  if(blkSize == PAGESIZE) {
    make_free_block(ptr, currNode);
    list_blk_insert(ptr, control->freelist[p].blk);
    reset_bitmap(currNode->bitmap);
    return ;
  }
  
  if(remainder == 0) {
    for(i=0;i<blkOffset;i++) {
      if(get_bit(currNode->bitmap, i+blkOffset) == 1) {
        free = 0;
        break;
      }
    }
    if(free == 1) {
      prime_ptr = (void*)((char*)ptr + blkSize);
      blk = control->freelist[p].blk->next;
      prevBlk = control->freelist[p].blk;
      while(blk  != NULL) {
        if(control->freelist[p].blk == prime_ptr) {
          blk_remove(blk, prevBlk);
          break;
        }
        prevBlk = blk;
        blk = blk->next;
      }

      blkSize = 2 * blkSize;
      return coalescing(ptr, blkSize, currNode);
        
    }
    else if(free == 0) {
      blk = make_free_block(ptr, currNode);
      list_blk_insert(blk, control->freelist[p].blk);
    } 
  }
  else if(remainder == blkOffset) {
    for(i=0; i<blkOffset;i++) {
      if(get_bit(currNode->bitmap, i-blkOffset) == 1) {
        free = 0;
        break;
      }
    }
    if(free == 1) {
      prime_ptr = (void*)((char*)ptr - blkSize);
      blk = control->freelist[p].blk->next;
      prevBlk = control->freelist[p].blk;
      while(blk != NULL) {
        if(control->freelist[p].blk == prime_ptr) {
          blk_remove(blk,prevBlk);
          break;
        }
        prevBlk = blk;
        blk = blk->next;
      }
      blkSize = 2 * blkSize;
      return coalescing(prime_ptr, blkSize, currNode);

    }
    else if(free == 0) {
      blk = make_free_block(ptr, currNode);
      list_blk_insert(blk, control->freelist[p].blk);
    }
  }
 
  else {
    printf("Error in coalescing\n");
  }

}

  

void*
kma_malloc(kma_size_t size)
{
  if(size > PAGESIZE)
    return NULL;
  if(page_entry == NULL)
    init_page_entry();

  return allocate_mem(size);
}

void 
kma_free(void* ptr, kma_size_t size)
{
  struct bud_controller *control;
  struct free_block *curr;
  struct page_node *currNode ;
  void *page_begin_addr;
  int offset;
  kma_page_t *tempPage;
  int i=0;
  int j=0;

  control = bud_info();

  

  for(i=0; i<HEADERSIZE; i++) {
    if(size <= control->freelist[i].size) {
      curr = ptr;
      curr->next = NULL;
      page_begin_addr = current_page_begin_addr(ptr);
      currNode = control->page_list.next;
      while(currNode->ptr != page_begin_addr){
        currNode = currNode->next;
      }
      curr->node = currNode;
      offset = ((char*)ptr - (char*)currNode->ptr) / MIN_BLK_SIZE;
      for(j=0; j<= (size-1)/MIN_BLK_SIZE; j++) {
        clear_bit(currNode->bitmap, j+offset);
      }
      coalescing(ptr, control->freelist[i].size, currNode);
      break;
    }
  }

  control->free++;

  if(control->free == control->used) {
    
    while(currNode != control->page_list.prev) {
      free_page(currNode->ptr);
      currNode = currNode->next;
    }
    free_page(currNode->ptr);
  }



  
  
}

#endif // KMA_BUD
