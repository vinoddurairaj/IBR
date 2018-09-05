/********************************************************* {COPYRIGHT-TOP} ***
* IBM Confidential
* OCO Source Materials
* 6949-32F - Softek Replicator for Unix and 6949-32K - Softek TDMF (IP) for Unix
*
*
* (C) Copyright IBM Corp. 2006, 2011  All Rights Reserved.
* The source code for this program is not published or otherwise  
* divested of its trade secrets, irrespective of what has been 
* deposited with the U.S. Copyright Office.
********************************************************* {COPYRIGHT-END} **/
/*-
 * Copyright (c) 2005,2007 Chris Fuller <crf@grandecom.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * License Exception:
 *
 * International Business Machines, Corp. (IBM) is hereby exempted from
 * license condition 2.  Specifically, IBM may redistribute this software
 * in binary form without reproducing this notice in the software or its
 * documentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define HASH_INTERNAL
#include "hash.h"

HashError hash_get_error(const hash_ctx *ctx)
{
	return ctx == NULL ? HE_NO_CONTEXT : ctx->err;
}

HashError hash_clear_error(hash_ctx *ctx)
{
	HashError err;
	if (ctx == NULL) return HE_NO_CONTEXT;
	err = ctx->err;
	ctx->err = HE_NO_ERROR;
	return err;
}

ftd_uint32_t hash_default_hash_fn(const void *data, size_t len)
{
	ftd_uint32_t hash = 0;

	if (data != NULL) {
		const unsigned char *s = data;
		while (len-- > 0)
			hash = (hash << 6) + (hash << 16) - hash + (ftd_uint32_t)*s++;
	}
	return hash;
}

int hash_default_equals_fn(
		const void *s1, size_t len1,
		const void *s2, size_t len2)
{
	return len1 != len2 ? 0 : (0 == memcmp(s1, s2, len1));
}

void hash_default_free_fn(void *x) {
	ftdfree(x);
}

void hash_ignore_free_fn(void *x) {
	/* Do nothing */
}

static const hash_ctx HASH_CONTEXT_INITIALIZER = {
	0,
	HE_NO_ERROR,
	0.75f,
	hash_default_hash_fn,
	hash_default_equals_fn,
	hash_default_free_fn,
	hash_default_free_fn
};

hash_ctx *hash_ctx_new(void) {
	hash_ctx *ctx = (hash_ctx *)ftdmalloc(sizeof(hash_ctx));
	if (ctx != NULL)
		*ctx = HASH_CONTEXT_INITIALIZER;
	return ctx;
}

int hash_ctx_destroy(hash_ctx *ctx) {
	if (ctx->table_count != 0) {
		ctx->err = HE_ACTIVE_CONTEXT;
		return 0;
	}
	ftdfree((void *)ctx);
	return 1;
}


hash_table *hash_new(hash_ctx *ctx, size_t size) {
	hash_table *ht;
	hash_linked_list **data = NULL;
	
	if (size <= 0) {
		ctx->err = HE_INVALID_SIZE;
		return NULL;
	}

	
	if (NULL == (ht = (hash_table *)ftdmalloc(sizeof(hash_table))) ||
			NULL == (data = (hash_linked_list **)ftdcalloc(size, sizeof(hash_linked_list *))) )
	{
		ftdfree((void *)ht);
		ctx->err = HE_OUT_OF_MEMORY;
		return NULL;
	}

	ctx->table_count++;

	ht->ctx = ctx;
	ht->size = size;
	ht->count = 0;
	ht->version = 1;
	ht->data = data;
	return ht;
}

void hash_clear(hash_table *ht) {
	hash_ctx *ctx = ht->ctx;
	hash_linked_list **data = ht->data;
	hash_linked_list *list, *next;
	hash_free_fn_t free_key_fn = ctx->free_key_fn;
	hash_free_fn_t free_value_fn = ctx->free_value_fn;
	int i;

	for (i=0; i<ht->size; i++) {
		list = data[i];
		while (list != NULL) {
			next = list->next;
			if (free_value_fn) (*free_value_fn)(list->value);
			if (free_key_fn) (*free_key_fn)(list->key);
			ftdfree((void *)list);
			list = next;
		}
		data[i] = NULL;
	}

	ht->count = 0;
	++ht->version;
}

void hash_destroy(hash_table *ht) {
	hash_ctx *ctx = ht->ctx;

	hash_clear(ht);
	ftdfree((void *)ht->data);
	ftdfree((void *)ht);
	ctx->table_count--;
}

int hash_put(
		hash_table *ht,
		void *key, size_t keylen,
		void *value)
{
	hash_ctx *ctx = ht->ctx;
	ftd_uint32_t hash = (*ctx->hash_fn)(key, keylen);
	unsigned int idx = hash % ht->size;
	hash_linked_list *entry =
		(hash_linked_list *)ftdmalloc(sizeof(hash_linked_list));
	hash_linked_list **lptr;
	hash_linked_list *list = ht->data[idx];
	hash_equals_fn_t equals_fn = ctx->equals_fn;
	float load, loadf;

	if (entry == NULL) {
		ctx->err = HE_OUT_OF_MEMORY;
		return 0;
	}

	++ht->version;
	ht->data[idx] = entry;
	entry->key = key;
	entry->keylen = keylen;
	entry->value = value;
	entry->next = list;
	lptr = &entry->next;


	while (list != NULL) {
		if ((*equals_fn)(key, keylen, list->key, list->keylen)) {
			*lptr = list->next;
			if (ctx->free_value_fn)(*ctx->free_value_fn)(list->value);
			if (ctx->free_key_fn) (*ctx->free_key_fn)(list->key);
			ftdfree((void *)list);
			return 1;
		}
		lptr = &list->next;
		list = *lptr;
	}

	load = ++ht->count / (float)ht->size;
	loadf = ctx->load_factor;
	if (loadf <= 0.0f)
		loadf = ctx->load_factor = 0.75f;
	if (load >= loadf) {
		int size = (ht->size << 1) + 1;
		while ((load = ht->count / (float)size) > ctx->load_factor)
			size = (size << 1) + 1;
		return hash_resize(ht, size) ? 3 : 4;
	}
	return 2;
}

int hash_resize(hash_table *ht, size_t size) {
	hash_ctx *ctx = ht->ctx;
	hash_hash_fn_t hash_fn = ctx->hash_fn;
	ftd_uint32_t hash;
	unsigned int i;
	hash_linked_list **data;
	hash_linked_list *list;
	hash_linked_list *next;

	if (size <= 0) {
		ctx->err = HE_INVALID_SIZE;
		return 0;
	}

	data = (hash_linked_list **)ftdcalloc(size, sizeof(hash_linked_list *));
	if (data == NULL) {
		ctx->err = HE_OUT_OF_MEMORY;
		return 0;
	}

	for (i=0; i<ht->size; i++) {
		list = ht->data[i];
		while (list != NULL) {
			hash = (*hash_fn)(list->key, list->keylen) % size;
			next = list->next;
			list->next = data[hash];
			data[hash] = list;
			list = next;
		}
	}

	ftdfree((void *)ht->data);
	++ht->version;
	ht->data = data;
	ht->size = size;
	return 1;
}

static hash_linked_list **find_entry(
		hash_table *ht,
		const void *key, size_t keylen)
{
	hash_ctx *ctx = ht->ctx;
	ftd_uint32_t hash = (*ctx->hash_fn)(key, keylen);
	unsigned int idx = hash % ht->size;
	hash_equals_fn_t equals_fn = ctx->equals_fn;
	hash_linked_list **lptr = &ht->data[idx];
	hash_linked_list *list = *lptr;

	while (list != NULL) {
		if ((*equals_fn)(key, keylen, list->key, list->keylen))
			return lptr;
		lptr = &list->next;
		list = *lptr;
	}

	return NULL;
}

void *hash_get(
		hash_table *ht,
		const void *key, size_t keylen)
{
	hash_linked_list **lptr = find_entry(ht, key, keylen);
	return lptr == NULL ? NULL : (*lptr)->value;
}

int hash_contains_key(
		hash_table *ht,
		const void *key, size_t keylen)
{
	hash_linked_list **lptr = find_entry(ht, key, keylen);
	return lptr == NULL ? 0 : 1;
}
		

int hash_del(
		hash_table *ht,
		const void *key, size_t keylen)
{
	hash_ctx *ctx = ht->ctx;
	hash_linked_list **lptr = find_entry(ht, key, keylen);
	hash_linked_list *entry;

	if (lptr == NULL) return 0;
	entry = *lptr;

	if (ctx->free_value_fn) (*ctx->free_value_fn)(entry->value);
	if (ctx->free_key_fn) (*ctx->free_key_fn)(entry->key);
	*lptr = entry->next;
	ftdfree((void *)entry);
	++ht->version;
	--ht->count;
	return 1;
}

/** Checks for whether or not a hash_table was modified by some
  * means other than a remove using this iterator.
  */
static int modified(hash_iter *hi)
{
	hash_table *ht = hi->ht;

	if (hi->version != ht->version) {
		ht->ctx->err = HE_MODIFIED;
		return 1;
	}
	return 0;
}

hash_iter *hash_iter_new(hash_table *ht)
{
	hash_iter *hi = (hash_iter *)ftdmalloc(sizeof(hash_iter));
	hi->ht = ht;
	hi->version = ht->version;
	hi->bucket = -1;
	hi->entry = NULL;
	hi->ref = NULL;

	if (hi == NULL) {
		ht->ctx->err = HE_OUT_OF_MEMORY;
		return NULL;
	}

	return hi;
}

void hash_iter_destroy(hash_iter *hi)
{
	ftdfree((void *)hi);
}


int hash_iter_next(hash_iter *hi)
{
	hash_table *ht = hi->ht;
	hash_linked_list *entry;

	/* Check for concurrent modification */
	if (modified(hi)) {
		hi->bucket = -1;
		hi->entry = NULL;
		hi->ref = NULL;
		return 0;
	}

	/* Check for more entries in the same linked list */
	if (NULL != (entry = hi->entry)) {
		if (NULL != entry->next) {
			hi->entry = entry->next;
			hi->ref = &entry->next;
			return 1;
		}
	}
	/* Check for saved reference from a remove */
	else if (NULL != hi->ref && NULL != *hi->ref) {
		hi->entry = *hi->ref;
		return 1;
	}

	/* Find the next linked list that has entries */
	do {
		/* Out of entries */
		if (++hi->bucket >= ht->size) {
			hi->bucket = -1;
			hi->entry = NULL;
			hi->ref = NULL;
			return 0;
		}
	} while (NULL == (entry = ht->data[hi->bucket]));

	/* Found the next entry */
	hi->entry = entry;
	hi->ref = &ht->data[hi->bucket];
	return 1;
}

void hash_iter_remove(hash_iter *hi)
{
	hash_table *ht = hi->ht;
	hash_ctx *ctx = ht->ctx;
	hash_linked_list *entry = hi->entry;

	if (entry == NULL) {
		ctx->err = HE_INVALID_REMOVE;
		return;
	}
	if (modified(hi)) return;

	hi->entry = NULL;
	*hi->ref = entry->next;
	if (ctx->free_value_fn) (*ctx->free_value_fn)(entry->value);
	if (ctx->free_key_fn) (*ctx->free_key_fn)(entry->key);
	ftdfree((void *)entry);
	++hi->version;
	++ht->version;
	--ht->count;
}

#ifdef HASH_TEST
static const char *OUT_OF_MEMORY  = "Out of memory.";
static const char *INVALID_SIZE   = "The hash table size must be positive.";
static const char *INVALID_REMOVE = "Called remove without a successful next.";
static const char *MODIFIED       = "Concurrent modification of hash table.";
static const char *ACTIVE_CONTEXT = "Cannot free an active hash context.";

static void check_error(hash_ctx *ctx)
{
	int err = ctx == NULL ? HE_OUT_OF_MEMORY : ctx->err;
	const char *s = NULL;

	switch (err) {
		case HE_NO_ERROR: return;
		case HE_OUT_OF_MEMORY:   s = OUT_OF_MEMORY;   break;
		case HE_INVALID_SIZE:    s = INVALID_SIZE;    break;
		case HE_INVALID_REMOVE:  s = INVALID_REMOVE;  break;
		case HE_MODIFIED:        s = MODIFIED;        break;
		case HE_ACTIVE_CONTEXT:  s = ACTIVE_CONTEXT;  break;
		default:
			fprintf(stderr, "??? %d\n", err);
			exit(2);
	}
	fprintf(stderr, "%s\n", s);
	exit(1);
}

/** Warning: This function assumes that only strings are
  	used as keys in the hash. */
static void free_key(void *x) {
	printf("free_key: %s\n", (char *)x);
}

/** Warning: This function assumes that only strings are
  	used as values in the hash. */
static void free_value(void *x) {
	printf("free_value: %s\n", (char *)x);
}

/** Warning: This function assumes that only strings are
  	used as keys and values in the hash. */
static void hash_debug(hash_table *ht) {
	hash_ctx *ctx = ht->ctx;
	hash_linked_list *list;
	int i;

	printf("hash_debug: ht=%p\n", ht);
	printf("  ht->ctx=%p (table_count=%u load_factor=%f)\n"
			"  ht->count/ht->size (load)=%u/%u (%f)\n",
			ht->ctx,
			ht->ctx->table_count,
			ht->ctx->load_factor,
			ht->count,
			ht->size,
			ht->count / (float)ht->size);


	for (i=0; i<ht->size; i++) {
		printf("  %3d:", i);
		list = ht->data[i];
		while (list != NULL) {
			printf("  %s->%s", list->key, list->value);
			list = list->next;
		}
		printf("\n");
	}
	printf("\n");
}

int main(int argc, char *argv[]) {
	hash_table *ht;
	hash_ctx *ctx;
	
	ctx = hash_ctx_new();
	check_error(ctx);
	ctx->free_key_fn = free_key;
	ctx->free_value_fn = free_value;

	ht = hash_new(ctx, 5);
	check_error(ctx);
	printf("--- Initial table\n");
	hash_debug(ht);

	hash_put_str(ht, "key1", "value1");
	check_error(ctx);
	printf("--- Added key1\n");
	hash_debug(ht);

	hash_put_str(ht, "key2", "value2");
	hash_put_str(ht, "key3", "value3");
	check_error(ctx);
	printf("--- Added key2, key3\n");
	hash_debug(ht);

	hash_put_str(ht, "keyA", "valueA");
	hash_put_str(ht, "keyB", "valueB");
	hash_put_str(ht, "keyC", "valueC");
	check_error(ctx);
	printf("--- Added more keys A-C\n");
	hash_debug(ht);


	hash_resize(ht, 7);
	check_error(ctx);
	printf("--- After resize to 7\n");
	hash_debug(ht);

	hash_resize(ht, 3);
	check_error(ctx);
	printf("--- After resize to 3\n");
	hash_debug(ht);

	printf("--- deleted key2: %d\n", hash_del_str(ht, "key2"));
	check_error(ctx);
	hash_debug(ht);

	printf("--- deleted keyX: %d\n", hash_del_str(ht, "keyX"));
	check_error(ctx);
	hash_debug(ht);

	printf("--- deleted keyB: %d\n", hash_del_str(ht, "keyB"));
	check_error(ctx);
	hash_debug(ht);

	printf("--- deleted key3: %d\n", hash_del_str(ht, "key3"));
	check_error(ctx);
	hash_debug(ht);

	hash_put_str(ht, "key1", "newValue1");
	check_error(ctx);
	printf("--- Replaced key1\n");
	hash_debug(ht);

	hash_put_str(ht, "key4", "value4");
	printf("--- Added key4\n");
	hash_debug(ht);
	check_error(ctx);

	hash_resize(ht, 1);
	check_error(ctx);
	printf("--- After resize to 1\n");
	hash_debug(ht);

	printf("--- destroying hash...\n");
	hash_destroy(ht);
	check_error(ctx);

	printf("\n--- destroying context...\n");
	if (!hash_ctx_destroy(ctx))

		check_error(ctx);

	printf("\n*** TESTS COMPLETE\n");
	return 0;
}


#endif /* HASH_TEST */
