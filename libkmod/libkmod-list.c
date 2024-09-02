// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <stdlib.h>

#include "libkmod.h"
#include "libkmod-internal.h"

static inline struct list_node *list_node_init(struct list_node *node)
{
	node->next = node;
	node->prev = node;

	return node;
}

static inline void list_node_append(struct list_node *list,
							struct list_node *node)
{
	if (list == NULL) {
		list_node_init(node);
		return;
	}

	node->prev = list->prev;
	list->prev->next = node;
	list->prev = node;
	node->next = list;
}

static inline struct list_node *list_node_remove(struct list_node *node)
{
	if (node->prev == node || node->next == node)
		return NULL;

	node->prev->next = node->next;
	node->next->prev = node->prev;

	return node->next;
}

static inline void list_node_insert_after(struct list_node *list,
							struct list_node *node)
{
	if (list == NULL) {
		list_node_init(node);
		return;
	}

	node->prev = list;
	node->next = list->next;
	list->next->prev = node;
	list->next = node;
}

static inline void list_node_insert_before(struct list_node *list,
							struct list_node *node)
{
	if (list == NULL) {
		list_node_init(node);
		return;
	}

	node->next = list;
	node->prev = list->prev;
	list->prev->next = node;
	list->prev = node;
}

static inline void list_node_append_list(struct list_node *list1,
							struct list_node *list2)
{
	struct list_node *list1_last;

	if (list1 == NULL) {
		list_node_init(list2);
		return;
	}

	list1->prev->next = list2;
	list2->prev->next = list1;

	/* cache the last, because we will lose the pointer */
	list1_last = list1->prev;

	list1->prev = list2->prev;
	list2->prev = list1_last;
}

struct kmod_list *kmod_list_append(struct kmod_list *list, const void *data)
{
	struct kmod_list *new;

	new = malloc(sizeof(*new));
	if (new == NULL)
		return NULL;

	new->data = (void *)data;
	list_node_append(list ? &list->node : NULL, &new->node);

	return list ? list : new;
}

struct kmod_list *kmod_list_insert_after(struct kmod_list *list,
							const void *data)
{
	struct kmod_list *new;

	if (list == NULL)
		return kmod_list_append(list, data);

	new = malloc(sizeof(*new));
	if (new == NULL)
		return NULL;

	new->data = (void *)data;
	list_node_insert_after(&list->node, &new->node);

	return list;
}

struct kmod_list *kmod_list_insert_before(struct kmod_list *list,
							const void *data)
{
	struct kmod_list *new;

	if (list == NULL)
		return kmod_list_append(list, data);

	new = malloc(sizeof(*new));
	if (new == NULL)
		return NULL;

	new->data = (void *)data;
	list_node_insert_before(&list->node, &new->node);

	return new;
}

struct kmod_list *kmod_list_append_list(struct kmod_list *list1,
						struct kmod_list *list2)
{
	if (list1 == NULL)
		return list2;

	if (list2 == NULL)
		return list1;

	list_node_append_list(&list1->node, &list2->node);

	return list1;
}

struct kmod_list *kmod_list_prepend(struct kmod_list *list, const void *data)
{
	struct kmod_list *new;

	new = malloc(sizeof(*new));
	if (new == NULL)
		return NULL;

	new->data = (void *)data;
	list_node_append(list ? &list->node : NULL, &new->node);

	return new;
}

struct kmod_list *kmod_list_remove(struct kmod_list *list)
{
	struct list_node *node;

	if (list == NULL)
		return NULL;

	node = list_node_remove(&list->node);
	free(list);

	if (node == NULL)
		return NULL;

	return container_of(node, struct kmod_list, node);
}

struct kmod_list *kmod_list_remove_data(struct kmod_list *list,
							const void *data)
{
	struct kmod_list *itr;
	struct list_node *node;

	for (itr = list; itr != NULL; itr = kmod_list_next(list, itr)) {
		if (itr->data == data)
			break;
	}

	if (itr == NULL)
		return list;

	node = list_node_remove(&itr->node);
	free(itr);

	if (node == NULL)
		return NULL;

	return container_of(node, struct kmod_list, node);
}

/*
 * n must be greater to or equal the number of elements (we don't check the
 * condition)
 */
struct kmod_list *kmod_list_remove_n_latest(struct kmod_list *list,
							unsigned int n)
{
	struct kmod_list *l = list;
	unsigned int i;

	for (i = 0; i < n; i++) {
		l = kmod_list_last(l);
		l = kmod_list_remove(l);
	}

	return l;
}

KMOD_EXPORT struct kmod_list *kmod_list_prev(const struct kmod_list *list,
						const struct kmod_list *curr)
{
	if (list == NULL || curr == NULL)
		return NULL;

	if (list == curr)
		return NULL;

	return container_of(curr->node.prev, struct kmod_list, node);
}

KMOD_EXPORT struct kmod_list *kmod_list_next(const struct kmod_list *list,
						const struct kmod_list *curr)
{
	if (list == NULL || curr == NULL)
		return NULL;

	if (curr->node.next == &list->node)
		return NULL;

	return container_of(curr->node.next, struct kmod_list, node);
}

KMOD_EXPORT struct kmod_list *kmod_list_last(const struct kmod_list *list)
{
	if (list == NULL)
		return NULL;
	return container_of(list->node.prev, struct kmod_list, node);
}
