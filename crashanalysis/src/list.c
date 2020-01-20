// File: list.c


#include <stdlib.h>
#include <stdio.h>
#include "list.h"


void *
list_init () {
	struct list_element *le;
	le = (struct list_element *) malloc(sizeof(struct list_element));
	le->element=NULL;
	le->next=NULL;
	return le;
}

int
check_if_list_empty(struct list_element *list) {
	if (list->next == NULL)
		return 1;
	else
		return 0;
}

void
list_free (struct list_element *list) {
	struct list_element *temp;
 
	temp = list;
	temp=temp->next;
	while (temp != NULL)
	{
		list->next=temp->next;
		free(temp->element);
		free(temp);
		temp=list->next;
	}
	free(list->element);
	free(list);
}


void *
list_add_element (struct list_element *list, void *element) {
	struct list_element *le;
	le = (struct list_element *) malloc(sizeof(struct list_element));
	le->element=element;
	le->next=NULL;

	struct list_element *temp;
	temp = list;
	while (temp->next != NULL)
		temp = temp->next;

	temp->next=le;
	return le;
}

int
list_remove_element (struct list_element *list, struct list_element *element) {
	struct list_element *temp;
	temp = list;
	while (temp->next != NULL)
	{
		if (temp->next == element)
		{		
			temp->next = element->next;
			free(element);
			return 1;
		}
		temp=temp->next;
	}
	return 0;
} 
