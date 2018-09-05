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
 * (c) Copyright IBM Corp.  2001-2009.  All Rights Reserved.
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has been
 * deposited with the U.S. Copyright Office.
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 ***************************************************************************************/

/**
 * @file @brief Implementation of platform specific parts of the @ref dynamic_activation_os.
 */

// --------------------------------------------------------------------------------------
//                                     I N C L U D E S
// --------------------------------------------------------------------------------------

#include "ftd_dynamic_activation_os.h"
#include "ftd_kern_cproto.h"

// --------------------------------------------------------------------------------------
//                        C O N S T A N T S   A N D   T Y P E S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//               G L O B A L   V A R I A B L E S   D E C L A R A T I O N S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//                 L O C A L   F U N C T I O N   D E C L A R A T I O N S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//                                       M A C R O S
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
//                                         C O D E
// --------------------------------------------------------------------------------------

// See the ftd_dynamic_activation.h for the documentation.
/**
 * @ingroup dynamic_activation_os
 *
 * @todo Confirm if the replacement of the strategy/read/write functions are global for
 *       all minor devices or if it is specific to the given devno.
 */
io_function_ptrs_t ftd_dynamic_activation_swap_io_routines(swap_table_key_t block_swap_key,
                                                           dev_t block_devno,
                                                           swap_table_key_t char_swap_key,
                                                           const io_function_ptrs_t* replacement_routines)
{
    io_function_ptrs_t old_routines = {NULL, NULL, NULL};
    int rc;
    
    ASSERT(block_swap_key == char_swap_key);
    ASSERT(block_swap_key == major_num(block_devno));

    rc = devswchg(block_devno, DSW_BLOCK, replacement_routines->strategy, &old_routines.strategy);
    if (rc != 0)
       panic("devswchg failure.");
    if (!old_routines.strategy)
       panic("ftd_dynamic_activation_swap_io_routines: original strat ptr null.");

    rc = devswchg(block_devno, DSW_CREAD, replacement_routines->read, &old_routines.read);
    if (rc != 0)
       panic("devswchg failure.");
    if (!old_routines.read)
       panic("ftd_dynamic_activation_swap_io_routines: original read ptr null.");

    rc = devswchg(block_devno, DSW_CWRITE, replacement_routines->write, &old_routines.write);
    if (rc != 0)
       panic("devswchg failure.");
    if (!old_routines.write)
       panic("ftd_dynamic_activation_swap_io_routines: original write ptr null.");
    
    return old_routines;
}

/***************************************************************************
 * Strategy function handling
 ***************************************************************************/

/**
 * Processes (replicates) an IO request received by our replacement strategy function.
 *
 * @param io_params The parameters of the captured IO request.
 *
 * @return The platform specific standard return codes for the strategy function.
 *
 * @internal The signature needs to conform to the generic_process_io_func_ptr type.
 *
 * @attention The ddstrategy routine must be coded to execute in an interrupt handler execution environment
 *            (device driver bottom half). That is, the routine should neither touch user storage, nor page fault, nor sleep.
 */
static int process_captured_strategy_io(const captured_io_parameters_t* io_params)
{
    return ftd_strategy(io_params->params.strategy.bp);
}

/**
 * @brief Forwards an IO by directly calling the captured strategy routine.
 *
 * @param io_params             The parameters of the captured IO request.
 * @param original_io_routines  The IO routines that were replaced.
 *
 * @return 0, as there's no return code from AIX's strategy routines.
 *
 * @internal The signature needs to conform to the generic_pass_io_through_func_ptr.
 */
static int call_captured_strategy(const captured_io_parameters_t* io_params, const io_function_ptrs_t* original_io_routines)
{
    original_io_routines->strategy(io_params->params.strategy.bp);
    // Aix's strategy routine doesn't return any value.
    return 0;
}

/**
 * @brief Our replacement ddstrategy function that receives all captured block IO requests.
 */
static void captured_driver_replacement_strategy(struct buf *userbp)
{
    /* for AIX, the incoming buffer may be a linked list of buffers. Unlink and forward each buffer. */
    while (userbp)
    {
        captured_io_parameters_t io_params = {0};
        struct buf* next_bp = userbp->av_forw;

        userbp->av_forw=0;
        io_params.params.strategy.bp = userbp;
        
        ftd_dynamic_activation_captured_driver_generic_replacement_routine(userbp->b_dev,
                                                                           &io_params,
                                                                           process_captured_strategy_io,
                                                                           call_captured_strategy);
        userbp = next_bp;
    }
}

/***************************************************************************
 * Read function handling
 ***************************************************************************/

/**
 * Processes (replicates) an IO request received by our replacement read function.
 *
 * @param io_params The parameters of the captured IO request.
 *
 * @return The platform specific standard return codes for the read function.
 *
 * @internal The signature needs to conform to the generic_process_io_func_ptr type.
 *
 */
static int process_captured_read_io(const captured_io_parameters_t* io_params)
{
    return ftd_rw(io_params->params.read.dev, io_params->params.read.uio, B_READ);
}

/**
 * @brief Forwards an IO by directly calling the captured read routine.
 *
 * @param io_params             The parameters of the captured IO request.
 * @param original_io_routines  The IO routines that were replaced.
 *
 * @return The platform specific standard return codes for the read function.
 *
 * @internal The signature needs to conform to the generic_pass_io_through_func_ptr.
 */
static int call_captured_read(const captured_io_parameters_t* io_params, const io_function_ptrs_t* original_io_routines)
{
    return original_io_routines->read(io_params->params.read.dev,
                                      io_params->params.read.uio,
                                      io_params->params.read.chan,
                                      io_params->params.read.extptr);
}

/**
 * @brief Our replacement ddread function that receives all captured read IO requests.
 */
static int captured_driver_replacement_read(dev_t devno, struct uio* uio, chan_t chan, void* ext_ptr)
{
    captured_io_parameters_t io_params;
    io_params.params.read.dev = devno;
    io_params.params.read.uio = uio;
    io_params.params.read.chan = chan;
    io_params.params.read.extptr = ext_ptr;
    
    return ftd_dynamic_activation_captured_driver_generic_replacement_routine(devno,
                                                                              &io_params,
                                                                              process_captured_read_io,
                                                                              call_captured_read);
 
}

/***************************************************************************
 * Write function handling
 ***************************************************************************/

/**
 * Processes (replicates) an IO request received by our replacement write function.
 *
 * @param io_params The parameters of the captured IO request.
 *
 * @return The platform specific standard return codes for the write function.
 *
 * @internal The signature needs to conform to the generic_process_io_func_ptr type.
 *
 */
static int process_captured_write_io(const captured_io_parameters_t* io_params)
{
    return ftd_rw(io_params->params.write.dev, io_params->params.write.uio, B_WRITE);
}

/**
 * @brief Forwards an IO by directly calling the captured write routine.
 *
 * @param io_params             The parameters of the captured IO request.
 * @param original_io_routines  The IO routines that were replaced.
 *
 * @return The platform specific standard return codes for the write function.
 *
 * @internal The signature needs to conform to the generic_pass_io_through_func_ptr.
 */
static int call_captured_write(const captured_io_parameters_t* io_params, const io_function_ptrs_t* original_io_routines)
{
    return original_io_routines->write(io_params->params.write.dev,
                                       io_params->params.write.uio,
                                       io_params->params.write.chan,
                                       io_params->params.write.ext);
}

/**
 * @brief Our replacement ddwrite function that receives all captured write IO requests.
 */
static int captured_driver_replacement_write(dev_t devno, struct uio* uio, chan_t chan, int ext)
{
    captured_io_parameters_t io_params;
    io_params.params.write.dev = devno;
    io_params.params.write.uio = uio;
    io_params.params.write.chan = chan;
    io_params.params.write.ext = ext;
    
    return ftd_dynamic_activation_captured_driver_generic_replacement_routine(devno,
                                                                              &io_params,
                                                                              process_captured_write_io,
                                                                              call_captured_write);
}

// See the ftd_dynamic_activation.h for the documentation.
/** @ingroup dynamic_activation_os */
const io_function_ptrs_t* ftd_dynamic_activation_get_replacement_routines(void)
{
    static const io_function_ptrs_t replacement_routines = {captured_driver_replacement_strategy,
                                                            captured_driver_replacement_read,
                                                            captured_driver_replacement_write};
    
    return &replacement_routines;
}
