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

#ifndef HASH_H
#define HASH_H

/**********************
 *  TYPE DEFINITIONS  *
 **********************/

/** Error codes */
typedef enum _HashError {
	 HE_NO_ERROR = 0
	,HE_NO_CONTEXT
	,HE_OUT_OF_MEMORY
	,HE_INVALID_SIZE
	,HE_INVALID_REMOVE
	,HE_MODIFIED
	,HE_ACTIVE_CONTEXT
} HashError;

/** Hashing functions */
typedef ftd_uint32_t (*hash_hash_fn_t)(const void *, size_t);

/** Equivilance testing functions */
typedef int (*hash_equals_fn_t)(const void *, size_t, const void *, size_t);

/** Memory releasing functions */
typedef void (*hash_free_fn_t)(void *);

/** Hash context */
typedef struct _hash_ctx {
	unsigned int table_count;
	HashError err;
	float load_factor;
	hash_hash_fn_t hash_fn;
	hash_equals_fn_t equals_fn;
	hash_free_fn_t free_key_fn;
	hash_free_fn_t free_value_fn;
} hash_ctx;

/** Hash table bucket linked list for managing data and key collisions. */
typedef struct _hash_linked_list {
	struct _hash_linked_list *next;
	void *key;
	void *value;
	size_t keylen;
} hash_linked_list;

/** Hash table */
typedef struct _hash_table {
	hash_ctx *ctx;
	int size;
	int count;
	int version;
	hash_linked_list **data;
} hash_table;

/** Hash table iterator */
typedef struct _hash_iter {
	hash_table *ht;
	int version;
	int bucket;
	hash_linked_list *entry;
	hash_linked_list **ref;
} hash_iter;

/** Retrieves the error code for the most recent error to occur.
  *
  * Returns:
  *   A value that indicates the last error to occur.  This value
  *   can be cleared (to HE_NO_ERROR) by calling hash_clear_error(ctx).
  *   If NULL is given for the ctx, then HE_NO_CONTEXT is returned.
  */
HashError hash_get_error(const hash_ctx *ctx);

/** Exactly equivilant to hash_get_error, except that if the context
  * is not NULL, then its error state is cleared to HE_NO_ERROR.
  *
  * Returns:
  *   A value that indicates the last error to occur.
  *   If NULL is given for the ctx, then HE_NO_CONTEXT is returned.
  */
HashError hash_clear_error(hash_ctx *ctx);


/** Creates a new hash table management context.
  *
  * Returns:
  * 	The new context, or NULL if the application is out of memory.
  */
hash_ctx *hash_ctx_new(void);

/** Destroy a context for managing hash tables.
  *
  * Returns:
  *		0	The context cannot be destroyed.  This occurs if the
  *			context is still active (i.e., it still has hash tables)
  *		1	Success
  *
  * Errors:
  * 	HE_ACTIVE_CONTEXT
  *			The context still has active hash tables.
  */
int hash_ctx_destroy(hash_ctx *ctx);

/** Create a new hash table of the specified size.
  *
  * Returns:
  * 	The new hash table, or NULL if there is an error condition
  *
  * Errors:
  *		HE_INVALID_SIZE
  *			The requested size was invalid (0)
  * 	HE_OUT_OF_MEMORY
  * 		The application is out of memory
  */
hash_table *hash_new(hash_ctx *ctx, size_t size);

/** Force the hash table to a new size.
  *
  * Errors:
  *		HE_INVALID_SIZE
  *			The requested size was invalid (0)
  * 	HE_OUT_OF_MEMORY
  * 		The application is out of memory
  */
int hash_resize(hash_table *ht, size_t size);

/** Completely clear a hash table.  All entries that are currently
  * in the hash table are deleted from it.
  */
void hash_clear(hash_table *ht);

/** Completely destroy a hash table.  The hash table is cleared using
  * hash_clear(ht).  Then, the hash table itself is freed, after which
  * it is no longer a valid hash_table reference.
  */
void hash_destroy(hash_table *ht);

/** Add a value to the hash table
  * 
  * Returns:
  *		0	Out of memory
  *		1	Success; replaced an existing entry
  *		2	Success; new entry; table size unchanged
  *		3	Success; new entry; table size increased to satisfy load factor
  *		4	Success; new entry; table size increase attempted, but failed
  *
  * Errors:
  * 	HE_OUT_OF_MEMORY
  * 		The application is out of memory.  Note that this is set
  * 		for case 4, even though the entry is successfully added.
  */
int hash_put(hash_table *ht, void *key, size_t keylen, void *value);

/** Returns whether or not the hash table contains the given key.
  *
  * Returns:
  *		0	The hash table does not contain this key
  *		1	The hash table contains this key
  */
int hash_contains_key(hash_table *ht, const void *key, size_t keylen);

/** Retrieves a value from the hash table.
  *
  * Returns:
  *		The value, or NULL if there is no such value.
  */
void *hash_get(hash_table *ht, const void *key, size_t keylen);

/** Removes a value from the hash table.
  *
  * Returns:
  *		0	Specified key was not found
  *		1	Success
  */
int hash_del(hash_table *ht, const void *key, size_t keylen);

/** Creates a new hash_table iterator.  The iterator must be explicitly
  * destroyed using hash_iter_destroy when the caller is done with it.
  *
  * Returns:
  * 	the new iterator
  */
hash_iter *hash_iter_new(hash_table *ht);

/** Moves to the next hash table entry for this iterator.  If there is
  * an error, or if there are no more entries, then 0 is returned.
  * The iterator will then start over with a new traversal of the
  * hash table.
  *
  * Returns:
  *		0	There was an error, or there are no more entries
  *		1	Success
  *
  * Errors:
  * 	HE_MODIFIED
  * 		The hash table has been modified by some means other than
  * 		this iterator since the iterator was created.  The iterator
  * 		will reset.
  */
int hash_iter_next(hash_iter *hi);

/** Removes the entry pointed to by the iterator.
  * It is only valid to call this once per successful return value
  * from hash_iter_next.
  *
  * Errors:
  * 	HE_MODIFIED
  * 		The hash table has been modified by some means other than
  * 		this iterator since the iterator was created.
  * 	HE_INVALID_REMOVE
  * 		There has not been a successful call to hash_iter_next.
  */
void hash_iter_remove(hash_iter *hi);

/** Destroys an iterator created with hash_iter_new. */
void hash_iter_destroy(hash_iter *hi);

/* Macros that assume the key to be a null-terminated string. */
#define hash_put_str(ht, key, value) (hash_put(ht, key, strlen(key), value))
#define hash_get_str(ht, key) (hash_get(ht, key, strlen(key)))
#define hash_del_str(ht, key) (hash_del(ht, key, strlen(key)))

#endif /* HASH_H */
