#include <stdlib.h>
#include <string.h>

#include "list.h"

void list_init(list_t *l, int elementSize, equalsFunction equalsFn, freeFunction freeFn) {
	
	l->length = 0;
	l->elementSize = elementSize;
	l->head = NULL;
	l->tail = NULL;
	l->equalsFn = equalsFn;
	l->freeFn = freeFn;
}

void list_destroy(list_t *l) {
	listNode_t *current;
	while(l->head != NULL) {
		current = l->head;
		l->head = current->next;
		if(l->freeFn) {
			l->freeFn(current->data);
		}

		l->length--;
		free(current->data);
		free(current);
	}
	l->tail = NULL;
}

void list_add(list_t *l, void *element) {
	listNode_t *node = malloc(sizeof(listNode_t));
  	node->data = malloc(l->elementSize);
	node->next = NULL;

	memcpy(node->data, element, l->elementSize);

	if(l->length == 0) {
		l->head = l->tail = node;
	} else {
		l->tail->next = node;
		l->tail = node;
  	}

	l->length++;
}

void list_remove(list_t *l, void *element) {
	listNode_t *currNode;
	listNode_t *prevNode;
	listNode_t *nextNode;

	prevNode = NULL;
	currNode = l->head;

	while(currNode) {

		nextNode = currNode->next;

		if(l->equalsFn(currNode->data, element)) {
			
			if(l->freeFn) {
      			l->freeFn(currNode->data);
    		}
    		free(currNode->data);
    		free(currNode);

    		if(prevNode) {
    			prevNode->next = nextNode;
    		} else {
    			l->head = nextNode;
    		}

    		l->length--;
    		return;
		}
		
		prevNode = currNode;
		currNode = nextNode;
	}
}

int list_contains(list_t *l, void* elem) {
	listNode_t *currNode;

	currNode = l->head;

	while(currNode) {

		if(l->equalsFn(currNode->data, elem)) {
			return 1;
		}

		currNode = currNode->next;
	}

	return 0;
}