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
 * @brief Implementation of the @ref dynamic_libzfs API.
 *
 * The current implementation has been tested against the 6/06 (libzfs.h 1.2) and 5/08 (libzfs.h 1.4) Solaris 10 releases.
 *
 * @todo Check libzfs.h within the 11/06 and 8/07 Solaris 10 releases to make sure that things will work fine there.
 *       Update the api version information if needed.
 *
 * @author Martin Proulx
 */

#include <stdlib.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "dynamic_libzfs.h"

/**
 * @brief Native libzfs type.
 *
 * The type pool_state_t is normally defined by including <libzfs.h> but since we want to be able to
 * compile even on Solaris 9, which doesn't provide this header, we've just copied this snippet
 * from the Solaris 10 header file.
 *
 * We do not even use the values of the enum.
 */
typedef enum pool_state {
        POOL_STATE_ACTIVE = 0,          /* In active use                */
        POOL_STATE_EXPORTED,            /* Explicitly exported          */
        POOL_STATE_DESTROYED,           /* Explicitly destroyed         */
        POOL_STATE_SPARE,               /* Reserved for hot spare use   */
        POOL_STATE_UNINITIALIZED,       /* Internal spa_t state         */
        POOL_STATE_UNAVAIL,             /* Internal libzfs state        */
        POOL_STATE_POTENTIALLY_ACTIVE   /* Internal libzfs state        */
} pool_state_t;

/**
 * @brief Native libzfs type.
 *
 * The type libzfs_handle_t is normally defined by including <libzfs.h> but since we want to be able to
 * compile even on Solaris 9, which doesn't provide this header, we've just copied this snippet
 * from the Solaris 10 header file.
 */
typedef struct libzfs_handle libzfs_handle_t;

// Various typedefs to ease the manipulation of dynamically loaded functions.
// These are the only functions we currently need to use out of libzfs.

/** @internal Signature of as libzfs_init seen in release 1.4 of the libzfs.h header. */
typedef libzfs_handle_t* (*libzfs_init_ptr_v1_4) (void);
/** @internal Signature of libzfs_fini as seen in release 1.4 of the libzfs.h header. */
typedef void (*libzfs_fini_ptr_v1_4) (libzfs_handle_t*);
/** @internal Signature of zpool_in_use as seen in release 1.4 of the libzfs.h header. */
typedef int (*zpool_in_use_ptr_v1_4) (libzfs_handle_t*, int, pool_state_t*, char**, boolean_t*);

/** @internal Signature of zpool_in_use as seen in release 1.2 of the libzfs.h header. */
typedef int (*zpool_in_use_ptr_v1_2) (int, pool_state_t*, char **);

/**
 * @brief Enumeration of all supported versions of the libzfs API.
 *
 * ingroup dynamic_libzfs
 *
 * @{
 */
enum libzfs_api_version {
    /** Version number appearing in libzfs.h of the 6/06 Solaris 10 release. */
    v1_2,
    /** Version number appearing in libzfs.h of the 8/05 Solaris 10 release. */
    v1_4
};

/* @brief Declaration of the functions needed from the version 1.2 of the libzfs API. */
struct libzfs_api_v1_2 {
    zpool_in_use_ptr_v1_2 zpool_in_use;
};

/* @brief Declaration of the functions and handles needed from the the version 1.4 of the libzfs API. */
struct libzfs_api_v1_4 {
    libzfs_handle_t* libzfs_handle;

    libzfs_init_ptr_v1_4 libzfs_init;
    libzfs_fini_ptr_v1_4 libzfs_fini;
    zpool_in_use_ptr_v1_4 zpool_in_use;
};

/**
 * @brief Internal context needed by the @ref dynamic_libzfs
 */
struct dynamic_libzfs_context {
    
    /** @brief Handle to the library obtained by dlopen(). */
    void* dynamically_loaded_libzfs_handle;
    /** @brief Holds which version of the library we're dealing with. */
    enum libzfs_api_version api_version;
    /** @brief Allows easier manipulation of the proper set of functions according to the version used. */
    union
    {
        struct libzfs_api_v1_2 v1_2;
        struct libzfs_api_v1_4 v1_4;
    } api;
};

dynamic_libzfs_context_t* dynamic_libzfs_init()
{
    int succesfully_initialized = 0;
    dynamic_libzfs_context_t* new_context = calloc(1, sizeof(struct dynamic_libzfs_context));

    if(new_context)
    {
        new_context->dynamically_loaded_libzfs_handle = dlopen("libzfs.so", RTLD_LAZY);

        if(new_context->dynamically_loaded_libzfs_handle)
        {
            void* libzfs_init = dlsym(new_context->dynamically_loaded_libzfs_handle, "libzfs_init");

            // Set up and fetch the proper entry points according to the version detected.
            if(libzfs_init)
            {
                // We assume v1_4 if we found the init function.
                new_context->api_version = v1_4;

                new_context->api.v1_4.libzfs_init = (libzfs_init_ptr_v1_4) libzfs_init;

                new_context->api.v1_4.libzfs_fini =
                    (libzfs_fini_ptr_v1_4) dlsym(new_context->dynamically_loaded_libzfs_handle, "libzfs_fini");
                
                new_context->api.v1_4.zpool_in_use =
                    (zpool_in_use_ptr_v1_4) dlsym(new_context->dynamically_loaded_libzfs_handle, "zpool_in_use");

                // Initialize the library.
                if(new_context->api.v1_4.libzfs_init)
                {
                    new_context->api.v1_4.libzfs_handle = new_context->api.v1_4.libzfs_init();
                }

                succesfully_initialized = (new_context->api.v1_4.libzfs_init &&
                                           new_context->api.v1_4.libzfs_fini &&
                                           new_context->api.v1_4.zpool_in_use &&
                                           new_context->api.v1_4.libzfs_handle);
            }
            else
            {
                // Assume v1_2
                new_context->api_version = v1_2;

                new_context->api.v1_2.zpool_in_use =
                    (zpool_in_use_ptr_v1_2) dlsym(new_context->dynamically_loaded_libzfs_handle, "zpool_in_use");

                succesfully_initialized = (new_context->api.v1_2.zpool_in_use != NULL);
            }
        }
    }

    // Clean up if we were not succesful.
    if(!succesfully_initialized)
    {
        dynamic_libzfs_finish(new_context);
        new_context = NULL;
    }
    
    return new_context;
}

/**
 * @note  Be aware that this can also be called from dynamic_libzfs_init() in the case of failures.
 *        In this case, we cannot assume a properly initialized context.
 */
void dynamic_libzfs_finish(dynamic_libzfs_context_t* dynamic_libzfs_context)
{
    if(dynamic_libzfs_context)
    {
        switch(dynamic_libzfs_context->api_version)
        {
            case v1_4:
            {
                if(dynamic_libzfs_context->api.v1_4.libzfs_fini && dynamic_libzfs_context->api.v1_4.libzfs_handle)
                {
                    dynamic_libzfs_context->api.v1_4.libzfs_fini(dynamic_libzfs_context->api.v1_4.libzfs_handle);
                }
                break;
            }
            default:
            {
                // Just to avoid some compiler warnings.
                break;
            }
        }    

        if(dynamic_libzfs_context->dynamically_loaded_libzfs_handle)
        {
            dlclose(dynamic_libzfs_context->dynamically_loaded_libzfs_handle);
        }

        free(dynamic_libzfs_context);
    }
}

int dynamic_libzfs_zpool_in_use(const dynamic_libzfs_context_t* dynamic_libzfs_context,
                                int file_descriptor,
                                char** zpool_name)
{
    int in_zpool = 0;

    switch(dynamic_libzfs_context->api_version)
    {
        case v1_2:
        {
            pool_state_t zpool_state;
            in_zpool = dynamic_libzfs_context->api.v1_2.zpool_in_use(file_descriptor, &zpool_state, zpool_name);
            break;
        }
        case v1_4:
        {
            boolean_t device_is_in_zpool;
            pool_state_t zpool_state;
            
            if(dynamic_libzfs_context->api.v1_4.zpool_in_use(dynamic_libzfs_context->api.v1_4.libzfs_handle,
                                                             file_descriptor,
                                                             &zpool_state,
                                                             zpool_name,
                                                             &device_is_in_zpool) == 0)
            {
                in_zpool = device_is_in_zpool;
            }
            break;
        }
    }
    
    return in_zpool;
}

int dynamic_libzfs_zpool_device_path_in_use(const dynamic_libzfs_context_t* dynamic_libzfs_context,
                                            const char* device_path,
                                            char** zpool_name)
{
    int in_zpool = 0;
    int file_descriptor = open(device_path, O_RDONLY);

    if(file_descriptor)
    {
        in_zpool = dynamic_libzfs_zpool_in_use(dynamic_libzfs_context, file_descriptor, zpool_name);
    
        close(file_descriptor);
    }
    return in_zpool;
}

#ifdef DYNAMIC_LIBZFS_STANDALONE_TEST

#include <stdio.h>

int main(int argc, char* argv[])
{
    const char* device_path = argv[1];
    dynamic_libzfs_context_t* context = dynamic_libzfs_init();

    if(context)
    {
        char* zpool_name;
        int in_use = dynamic_libzfs_zpool_device_path_in_use(context, device_path, &zpool_name);
        printf("%s is use: %d in %s\n", device_path, in_use, in_use ? zpool_name : "-none-");

        if(in_use)
        {
            free(zpool_name);
        }
    }
    
    dynamic_libzfs_finish(context);

    return 0;
}
#endif /* DYNAMIC_LIBZFS_STANDALONE_TEST */

/**
 * @}
 */
