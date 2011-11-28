/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011  ProFUSION embedded systems
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation version 2.1.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdlib.h>

#include "libkmod.h"
#include "libkmod-private.h"

static inline struct list_node *list_node_init(struct list_node *node)
{
	node->next = node;
	node->prev = node;

	return node;
}

static inline struct list_node *list_node_next(struct list_node *node)
{
	if (node == NULL)
		return NULL;

	return node->next;
}

static inline struct list_node *list_node_prev(struct list_node *node)
{
	if (node == NULL)
		return NULL;

	return node->prev;
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

	return node->prev;
}

struct kmod_list *kmod_list_append(struct kmod_list *list, void *data)
{
	struct kmod_list *new;

	new = malloc(sizeof(*new));
	if (new == NULL)
		return NULL;

	new->data = data;
	list_node_append(list ? &list->node : NULL, &new->node);

	return list ? list : new;
}

struct kmod_list *kmod_list_prepend(struct kmod_list *list, void *data)
{
	struct kmod_list *new;

	new = malloc(sizeof(*new));
	if (new == NULL)
		return NULL;

	new->data = data;
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

KMOD_EXPORT struct kmod_list *kmod_list_next(struct kmod_list *list,
							struct kmod_list *curr)
{
	if (list == NULL || curr == NULL)
		return NULL;

	if (curr->node.next == &list->node)
		return NULL;

	return container_of(curr->node.next, struct kmod_list, node);
}
