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
/***************************************************************************************
 *
 * LICENSED MATERIALS / PROPERTY OF IBM
 *
 * (c) Copyright IBM Corp.  2001-2010.  All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has been
 * deposited with the U.S. Copyright Office.
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 ***************************************************************************************/

/**
 * @file
 * @brief Declarations of platform specific parts of the @ref dynamic_activation_os.
 *
 * @author Martin Proulx
 */
#ifndef _FTD_DYNAMIC_ACTIVATION_SUN_H_
#define _FTD_DYNAMIC_ACTIVATION_SUN_H_

// --------------------------------------------------------------------------------------
//                                     I N C L U D E S
// --------------------------------------------------------------------------------------

#include <sys/devops.h>
#include <sys/types.h>

// --------------------------------------------------------------------------------------
//                        C O N S T A N T S   A N D   T Y P E S
// --------------------------------------------------------------------------------------

typedef int (*strat_func_ptr) (struct buf *bp);
typedef int (*read_func_ptr) (dev_t dev, struct uio * uio, cred_t * cred_p);
typedef int (*write_func_ptr) (dev_t dev, struct uio * uio, cred_t * cred_p);
typedef int (*aread_func_ptr) (dev_t dev, struct aio_req * aioreq_p, cred_t * cred_p);
typedef int (*awrite_func_ptr) (dev_t dev, struct aio_req * aioreq_p, cred_t * cred_p);

typedef struct io_function_ptrs {
    strat_func_ptr strategy;
    read_func_ptr read;
    write_func_ptr write;
    aread_func_ptr aread;
    awrite_func_ptr awrite;
} io_function_ptrs_t;

/** We swap based on the major number of the captured device's driver. */
typedef major_t swap_table_key_t;

typedef struct captured_io_parameters
{
    union {
        struct
        {
            struct buf *bp;
        } strategy;
        struct
        {
            dev_t dev;
            struct uio* uio;
            cred_t * cred_p;
        } read_write;
        struct
        {
            dev_t dev;
            struct aio_req * aioreq_p;
            cred_t * cred_p;
        } aread_write;
    } params;
} captured_io_parameters_t;

// --------------------------------------------------------------------------------------
//                 G L O B A L   V A R I A B L E   R E F E R E N C E S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//               G L O B A L   F U N C T I O N   D E C L A R A T I O N S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//     I N L I N E S,  M A C R O S,  A N D   T E M P L A T E   D E F I N I T I O N S
// --------------------------------------------------------------------------------------

#endif // _FTD_DYNAMIC_ACTIVATION_SUN_H_
