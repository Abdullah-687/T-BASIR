// File: list.h

#ifndef TRACER_LIST_H
#define TRACER_LIST_H

#include "defs.h"


void *
list_init ();

int
check_if_list_empty(struct list_element *list);


void
list_free (struct list_element *list);


void *
list_add_element (struct list_element *list, void *element);


int
list_remove_element (struct list_element *list, struct list_element *element);


#endif 
