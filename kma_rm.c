/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
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
#ifdef KMA_RM
#define __KMA_IMPL__

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
struct page_header {
  kma_page_t *page;
};

struct node {
  void *addr;
  int size;
  struct node *prev;
  struct node *next;
};

struct rm_controller {
  int   used;
  int   free;
  struct node free_list;
  struct node used_list;
};

void *current_page_begin_addr(void *addr) {
  return (void*)((unsigned long)addr & ~((unsigned long)(PAGESIZE-1)));
}

void *current_page_end_addr(void *addr) {
  return (void*)((char*)current_page_begin_addr(addr) + PAGESIZE);
}

void list_append(struct node *newNode, struct node *list) {
  newNode->prev = list->prev;
  newNode->next = list;
  list->prev = newNode;
  newNode->prev->next = newNode;
}

void list_insert(struct node *newNode, struct node *list, int pos) {
  if(pos < 1)
    fprintf(stderr, "insert position is less than 1");
  else {
    struct node *curr;
    
    curr = list;
    int i = 1;

    while(i != pos) {
      curr = curr->next;
      i++;
      if(curr == list){
        fprintf(stderr, "the position is larger than list size");
        break;
      }
    }
    newNode->prev = curr;
    newNode->next = curr->next;
    curr->next = newNode;
    newNode->next->prev = newNode;
  }
}

void list_remove(struct node *node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
}
//void list_append(struct node *newNode, struct node *list) {
//  list->next = newNode;
//}

void init_page_entry() {
  struct page_header *header;
  struct rm_controller *rm;
  struct node *curr, *end;
  page_entry = get_page();

  /* allocate header and controller to entry page. */
  header = (struct page_header*)page_entry->ptr;
  rm = (struct rm_controller*)((char*)page_entry->ptr + sizeof(struct page_header));

  /* initial elements in header and controller */
  header->page = page_entry;
  rm->used = 0;
  rm->free = 0;
  rm->free_list.prev = &(rm->free_list);
  rm->free_list.next = &(rm->free_list);
  rm->used_list.prev = &(rm->used_list);
  rm->used_list.next = &(rm->used_list);
  
  curr = (struct node*)((char*)rm + sizeof(struct rm_controller));
  end = (struct node*)((char*)current_page_end_addr(curr));

  /* initial rest of  nodes to free_list. */
  while(curr + 1 < end) {
    list_append(curr, &(rm->free_list));
    curr++;
  }
}
  




void*
kma_malloc(kma_size_t size)
{
  if(page_entry == NULL)
    init_page_entry(); 
  


  return NULL;
}

void
kma_free(void* ptr, kma_size_t size)
{
  ;
}

#endif // KMA_RM
