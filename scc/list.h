// SPDX-License-Identifier: MIT

#ifndef _LIST_H_
#define _LIST_H_

#define list_iterate(var, head, link)		\
	for (var = head; var; var = var->link)

#define list_count(head, link) ({		\
	typeof(&(head)[0]) ptr;			\
	int n = 0;				\
	list_iterate(var, head, link)		\
		n++;				\
	n; })

// add 'new' at the beginning of the list and
// return the new head
#define list_add_head(head, new, link) ({	\
	new->link = head->link;			\
	head->link = new;			\
	new; })

// add 'new' to the end of the list and
// return the index of the element in the list
#define list_append(head, new, link) ({		\
	typeof(&(head)[0]) *ptr = &head;	\
	int n = 0;				\
	for (; *ptr; ptr = &(*ptr)->link)	\
		n++;				\
	new->link = NULL;			\
	*ptr = new;				\
	n; })

#endif
