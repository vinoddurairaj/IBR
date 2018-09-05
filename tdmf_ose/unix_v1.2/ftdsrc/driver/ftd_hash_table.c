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
 * @brief @ref hash_table implementation.
 *
 * @author Bradley Musolff (original code from tdmf's add/deldeventry and lookupdev)
 * @author Martin Proulx   (API creation and enhancements)
 *
 *
 */

/********************************************************************************************************************//**
 *
 * @defgroup hash_table hash_table API
 *
 * An hash table API that can store pointers from a numerical key value.
 *
 * The current implementation's simple hash function is (key % hash_table->size)
 * and each slot holds a linked list of entries under it.
 *
 * @todo Remove the internal locking mechanism?  Leave it to the user to manage their own lock?
 *
 ************************************************************************************************************************/

#include "ftd_kern_ctypes.h"
#include "ftd_ddi.h" // For kmem_xxx functions.
#include "ftd_def.h"
#include "ftd_hash_table.h"

/**
 * @brief Structure used in the hash table and list implementation of the @ref hash_table.
 *
 * An entry hold together a key and a remembered value.
 *
 * @ingroup hash_table
 */
struct hash_table_entry {
    /** @brief Entry key. */
    hash_table_key_t key;
    /** @brief Pointer to the remembered value. */
    void* value;
    /** @brief Member allowing to link together multiple entries in the case there is collision in a slot of the hash table. */
    struct hash_table_entry *next;
};

// See ftd_hash_table.h for the API documentation.
int hash_table_init(hash_table_size_t size, hash_table_t* hash_table)
{
    int rc = 0;
    
    hash_table->size = size;
    hash_table->array = kmem_zalloc(hash_table->size * sizeof(struct hash_table_entry), KM_SLEEP);

    if(hash_table->array)
    {
        ALLOC_LOCK(hash_table->lock, QNM " hash table implementation lock.");
    }
    else
    {
        rc = ENOMEM;
    }
    
    return rc;
}

// See ftd_hash_table.h for the API documentation.
void hash_table_finish(hash_table_t* hash_table)
{
    kmem_free(hash_table->array, hash_table->size * sizeof(struct hash_table_entry));
    DEALLOC_LOCK(hash_table->lock);
}

/**
 * @brief Looks up a hash table entry within the hashtable implementation. 
 *
 * This really is the heart of the hashtable and list implementation logic for the @ref hash_table.
 *
 * @note This isn't part of the API itself, but part of its private implementation.
 *
 * @param      hash_table                         An initialized opaque hash table object.
 * @param      key                                The key of the entry we are asking for.
 * @param[in]  address_of_pointer_to_found_entry  Address where the pointer to the found entry is to be stored,
 *                                                or NULL if this information is not needed.
 * @param[out] address_of_pointer_to_found_entry  The address of the pointer pointing to the found entry, or
 *                                                to the implementation's linked list NULL tail if the entry
 *                                                wasn't found.  By modifying this pointer, we can add information
 *                                                to the end of the linked list when the entry isn't found,
 *                                                or delete the entry, when it is found.
 * 
 * @return A pointer to the hash_table_entry if found, NULL otherwise.
 *
 * @internal The #hash_table_t's lock should be held before calling this.
 *
 * @pre hash_table_init() Must have previously been called on hash_table.
 *
 * @ingroup hash_table
 */

static hash_table_entry_p hash_table_get_entry(hash_table_t* hash_table,
                                               hash_table_key_t key, hash_table_entry_p** address_of_pointer_to_found_entry)
{
    hash_table_size_t pos = key % hash_table->size;
    hash_table_entry_p found_entry = hash_table->array[pos];

    if(address_of_pointer_to_found_entry)
    {
        *address_of_pointer_to_found_entry = &hash_table->array[pos];
    }
    
    // First check if there are entries under this hashed position entry.
    if (!found_entry)
    {
        // If not, we haven't found it.
        found_entry = NULL;
        goto ret;
    }

    // There is a list of entries linked together within the array's hashed position,
    // We loop until we either find a matching entry or we hit the last unmatched entry.
    while ((found_entry->key != key) && (found_entry->next != NULL))
    {
        // We advance our pointers down the linked list.
        if(address_of_pointer_to_found_entry)
        {
            *address_of_pointer_to_found_entry = &(found_entry->next);
        }
        found_entry=found_entry->next;
    }

    // At this point, we either have found the entry or reached the end of the chain.
    // We know we have reached the end of the chain if the found_entry's key isn't the key we were looking for.
    if (found_entry->key != key)
    {
        // The entry wasn't found, we need to return the address that holds the terminating NULL pointer.
        if(address_of_pointer_to_found_entry)
        {
            *address_of_pointer_to_found_entry = &(found_entry->next);
        }
        
        // We're at the end of the chain and haven't found our entry.
        found_entry = NULL;
    }
 ret:
    return found_entry;
}

// See ftd_hash_table.h for the API documentation.
int hash_table_add(hash_table_t* hash_table, hash_table_key_t key, void* value)
{
    int rc = 0;
    ftd_context_t context;
    hash_table_entry_p ent = kmem_zalloc(sizeof(struct hash_table_entry), KM_SLEEP);
    hash_table_entry_p found_entry = NULL;
    
    if(ent == NULL)
    {
        return ENOMEM;
    }
    
    ent->key = key;
    ent->value = value;
    ent->next = NULL;

    ACQUIRE_LOCK(hash_table->lock, context);
    {
        hash_table_entry_p* pointer_to_null_terminator_of_list;
        found_entry = hash_table_get_entry(hash_table, key, &pointer_to_null_terminator_of_list);

        if(found_entry == NULL)
        {
            // We append at the end of the list.
            *pointer_to_null_terminator_of_list = ent;
        }
    }
    RELEASE_LOCK(hash_table->lock, context);

    if(found_entry)
    {
        // We cannot release memory while holding a lock on AIX.
        kmem_free(ent, sizeof(struct hash_table_entry));
        rc = EEXIST;  
    }
    
    return rc;
}

// See ftd_hash_table.h for the API documentation.
int hash_table_update(hash_table_t* hash_table, hash_table_key_t key, void* new_value)
{
    int rc = 0;
    ftd_context_t context;
    
    ACQUIRE_LOCK(hash_table->lock, context);
    {
        hash_table_entry_p found_entry = hash_table_get_entry(hash_table, key, NULL);
        if(found_entry)
        {
            found_entry->value = new_value;
        }
        else
        {
            rc = ENOENT;
        }
    }
    RELEASE_LOCK(hash_table->lock, context);
    
    return rc;
}

// See ftd_hash_table.h for the API documentation.
int hash_table_del (hash_table_t* hash_table, hash_table_key_t key)
{
    int rc = ENOENT;
    ftd_context_t context;
    hash_table_entry_p found_entry = NULL;
    
    ACQUIRE_LOCK(hash_table->lock, context);
    {
        hash_table_entry_p* pointer_to_entry_to_delete;
        found_entry = hash_table_get_entry(hash_table, key, &pointer_to_entry_to_delete);

        if(found_entry)
        {
            // We unlink the found entry to delete from the chained list.
            *pointer_to_entry_to_delete = found_entry->next;
            rc = 0;
        }        
    }
    RELEASE_LOCK(hash_table->lock, context);

    if(found_entry)
    {
        // We cannot release memory while holding a lock on AIX.
        kmem_free(found_entry, sizeof(struct hash_table_entry));
    }   
    
    return rc;
}

// See ftd_hash_table.h for the API documentation.
void* hash_table_get (hash_table_t* hash_table, hash_table_key_t key)
{
    ftd_context_t context;
    void* value = NULL;
    
    ACQUIRE_LOCK(hash_table->lock, context);
    {
        hash_table_entry_p found_entry = hash_table_get_entry(hash_table, key, NULL);
        if(found_entry)
        {
            value = found_entry->value;
        }
    }
    RELEASE_LOCK(hash_table->lock, context);
    
    return value;
}
