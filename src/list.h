#ifndef LIST_H
#define LIST_H
 
/* function used to test if two nodes are equals
	must return 1 if elements are equals, 0 if not */
typedef int (*equalsFunction)(void *, void *);
/* function used to free a node */
typedef void (*freeFunction)(void *);
 
typedef struct listNode_s 
{
	void *data;
	struct listNode_s *next;
} listNode_t;
 
typedef struct list_s 
{
	int length;
	int elementSize;
	listNode_t *head;
	listNode_t *tail;
	equalsFunction equalsFn;
	freeFunction freeFn;
} list_t;
 
void list_init(list_t *l, int elementSize, equalsFunction equalsFn, freeFunction freeFn);
void list_destroy(list_t *l);

// adds at the end of the list
void list_add(list_t *l, void *element);
void list_remove(list_t *l, void *element);
int list_contains(list_t *l, void* elem);
 
#endif