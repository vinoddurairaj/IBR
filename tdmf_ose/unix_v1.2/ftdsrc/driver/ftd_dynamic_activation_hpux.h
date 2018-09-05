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
#ifndef _FTD_DYNAMIC_ACTIVATION_HPUX_H_
#define _FTD_DYNAMIC_ACTIVATION_HPUX_H_

// --------------------------------------------------------------------------------------
//                                     I N C L U D E S
// --------------------------------------------------------------------------------------

#include <sys/conf.h>

#include "ftd_def.h" /* major_t */

// --------------------------------------------------------------------------------------
//                        C O N S T A N T S   A N D   T Y P E S
// --------------------------------------------------------------------------------------

typedef void (*strat_func_ptr) (struct buf *bp);
typedef int (*read_func_ptr) (dev_t dev, struct uio * uio);
typedef int (*write_func_ptr) (dev_t dev, struct uio * uio);

typedef struct io_function_ptrs {
    strat_func_ptr strategy;
    read_func_ptr read;
    write_func_ptr write;
    aio_ops_t* block_aio_ops;
    aio_ops_t* char_aio_ops;
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
        } read_write;
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

#define SWAP_KEYS_DESCRIPTION "block major device: %d and char major device: %d"

#endif // _FTD_DYNAMIC_ACTIVATION_HPUX_H_
