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
/**
 * @file
 * @brief @ref hash_table declarations.
 *
 * @author Bradley Musolff (original code from tdmf's add/deldeventry and lookupdev)
 * @author Martin Proulx   (API creation and enhancements)
 *
 */
#ifndef _FTD_HASH_TABLE_H_
#define _FTD_HASH_TABLE_H_

#include "ftd_kern_ctypes.h"
#include "ftd_def.h" // Just for the ftd_lock_t type.

/**
 * @brief The numerical key value type.
 *
 * @ingroup hash_table
 */
typedef unsigned long hash_table_key_t;

/**
 * @brief The hash table size value type.
 * @note This type must be able to hold the maximum key value.
 *
 * @ingroup hash_table
 */ 
typedef hash_table_key_t hash_table_size_t;

/**
 * @brief Typedef to ease the usage of a 'struct hash_table_entry*'.
 *
 * @ingroup hash_table
 */
typedef struct hash_table_entry* hash_table_entry_p;

/**
 * @brief The OO representation of the hash_table implementation.
 *
 * @ingroup hash_table
 */
struct hash_table {
    /** @brief Holds how many slots are in the array. */
    hash_table_size_t size;

    /** @brief A dynamically allocated array holding size slots. */
    hash_table_entry_p* array;

    /** @brief A lock used to synchronize accesses and updates to the hash table. */
    ftd_lock_t lock;
};

/**
 * @brief The opaque type used to represent the hash table.
 *
 * @ingroup hash_table
 */
typedef struct hash_table hash_table_t;

/** 
 * @brief Initializes the implementation of the @ref hash_table.
 *
 * This must be called before hash_table_add(), hash_table_() and hash_table_get() are called.
 *
 * @param size            The number of slots the hash table will fold its entries into.
 * @param hash_table[in]  A non initialized opaque hash table object.
 * @param hash_table[out] An initialized and ready to be used opaque hash table object.
 *
 * @return ENOMEM if the hash table couldn't be allocated.
 *         0 otherwise.
 *
 * @post hash_table_finish() Must be called when we're done with the hash table.
 *
 * @ingroup hash_table
 */
int hash_table_init(hash_table_size_t size, hash_table_t* hash_table);

/** 
 * @brief Terminates the implementation of the @ref hash_table.
 *
 * @param hash_table  A pointer to an opaque hash table object.
 *
 * @pre hash_table_init() Must have previously been called on hash_table.
 * @post The hash_table cannot be reused afterwards.
 *
 * @ingroup hash_table
 */
void hash_table_finish(hash_table_t* hash_table);

/** 
 *
 * @brief Add an entry in the hash table.
 *
 * @param hash_table An initialized opaque hash table object.
 * @param key        The key of the entry we are asking for.
 * @param value      The pointer to be remembered under the given key.
 *
 * @return 0 if successful.
 *         ENOMEM if we cannot allocate the new hash table entry to hold the new value.
 *         EEXIST if the entry was already present.
 *
 * @pre hash_table_init() Must have previously been called on hash_table.
 *
 * @ingroup hash_table
 */
int hash_table_add(hash_table_t* hash_table, hash_table_key_t key, void* value);

/** 
 *
 * @brief Updates an entry in the hash table.
 *
 * @param hash_table An initialized opaque hash table object.
 * @param key        The key of the entry we want to update.
 * @param new_value  The new pointer to be remembered under the given key.
 *
 * @return 0 if successful.
 *         ENOENT if no value was previouly registered under this key.
 *
 * @pre hash_table_init() Must have previously been called on hash_table.
 *
 * @ingroup hash_table
 */
int hash_table_update(hash_table_t* hash_table, hash_table_key_t key, void* new_value);

/**
 *
 * @brief Forgets about a value in the hash table.
 *
 * @param hash_table An initialized opaque hash table object.
 * @param key        The key of the entry we want to forget about.
 *
 * @return 0 If the deletion was succesful, ENOENT if no value was previouly registered under this key.
 *
 * @pre hash_table_init() Must have previously been called on hash_table.
 *
 * @ingroup hash_table
 */
int hash_table_del (hash_table_t* hash_table, hash_table_key_t key);

/**
 *
 * @brief Obtains the stored pointer value for the given hash table key.
 *
 * @param hash_table An initialized opaque hash table object.
 * @param key        The key of the entry we are asking for.
 *
 * @return The remembered pointer for this key, NULL otherwise.
 *
 * @pre hash_table_init() Must have previously been called on hash_table.
 *
 * @ingroup hash_table
 */
void* hash_table_get (hash_table_t* hash_table, hash_table_key_t key);

#endif // _FTD_HASH_TABLE_H_
