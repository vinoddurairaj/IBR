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

#include <sys/debug.h>

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
/** @ingroup dynamic_activation_os */
io_function_ptrs_t ftd_dynamic_activation_swap_io_routines(swap_table_key_t block_swap_key,
                                                           dev_t block_devno,
                                                           swap_table_key_t char_swap_key,
                                                           const io_function_ptrs_t* replacement_routines)
{
    io_function_ptrs_t old_routines = {NULL, NULL, NULL};
    int block_major = block_swap_key;
    int char_major = char_swap_key;

    VASSERT(block_swap_key == major(block_devno));
    
    /* bdm>> should add the following check for all platforms */
    /* ensure proper word alignment so that swap is atomic */
    if (((int64_t)bdevsw[block_major].d_strategy % 8) != 0) {
        panic("ftd_swap_strategy improper alignment");
    }
    if (((int64_t)cdevsw[char_major].d_read % 8) != 0) {
        panic("tdmf_swap_read improper alignment");
    }
    /* ensure proper word alignment so that swap is atomic */
    if (((int64_t)cdevsw[char_major].d_write % 8) != 0) {
        panic("tdmf_swap_write improper alignment");
    }
    
    old_routines.strategy = (strat_func_ptr)bdevsw[block_major].d_strategy;
    old_routines.read = cdevsw[char_major].d_read;
    old_routines.write = cdevsw[char_major].d_write;
    old_routines.block_aio_ops = bdevsw[block_major].d_aio_ops;
    old_routines.char_aio_ops = cdevsw[char_major].d_aio_ops;
        
    /* atomic store is safe */
    bdevsw[block_major].d_strategy = (d_strategy_t)replacement_routines->strategy;
    cdevsw[char_major].d_read = replacement_routines->read;
    cdevsw[char_major].d_write = replacement_routines->write;

    if (old_routines.block_aio_ops != NULL)
    {
        bdevsw[block_major].d_aio_ops = replacement_routines->block_aio_ops;
    }
    
    if (old_routines.char_aio_ops != NULL)
    {
        cdevsw[char_major].d_aio_ops = replacement_routines->char_aio_ops;
    }

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
 * @attention The driver_strategy() routine must not call sleep(), because a strategy routine may be executing on
 *            the interrupt stack.
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
 * @return 0, as there's no return code from HP-UX's strategy routines.
 *
 * @internal The signature needs to conform to the generic_pass_io_through_func_ptr.
 */
static int call_captured_strategy(const captured_io_parameters_t* io_params, const io_function_ptrs_t* original_io_routines)
{
    original_io_routines->strategy(io_params->params.strategy.bp);
    // HP-UX's strategy routine doesn't return any value.
    return 0;
}

/**
 * @brief Our replacement driver_strategy function that receives all captured block IO requests.
 */
static void captured_driver_replacement_strategy(struct buf *userbp)
{
    captured_io_parameters_t io_params;
    io_params.params.strategy.bp = userbp;
    
    ftd_dynamic_activation_captured_driver_generic_replacement_routine(userbp->b_dev,
                                                                       &io_params,
                                                                       process_captured_strategy_io,
                                                                       call_captured_strategy);
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
    return ftd_rw(io_params->params.read_write.dev, io_params->params.read_write.uio, B_READ);
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
    return original_io_routines->read(io_params->params.read_write.dev,
                                      io_params->params.read_write.uio);
}

/**
 * @brief Our replacement driver_read function that receives all captured read IO requests.
 */
static int captured_driver_replacement_read(dev_t devno, struct uio* uio)
{
    captured_io_parameters_t io_params;
    io_params.params.read_write.dev = devno;
    io_params.params.read_write.uio = uio;
    
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
    return ftd_rw(io_params->params.read_write.dev, io_params->params.read_write.uio, B_WRITE);
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
    return original_io_routines->write(io_params->params.read_write.dev,
                                       io_params->params.read_write.uio);
}

/**
 * @brief Our replacement driver_write function that receives all captured write IO requests.
 */
static int captured_driver_replacement_write(dev_t devno, struct uio* uio)
{
    captured_io_parameters_t io_params;
    io_params.params.read_write.dev = devno;
    io_params.params.read_write.uio = uio;
    
    return ftd_dynamic_activation_captured_driver_generic_replacement_routine(devno,
                                                                              &io_params,
                                                                              process_captured_write_io,
                                                                              call_captured_write);
}

// See the ftd_dynamic_activation.h for the documentation.
/** @ingroup dynamic_activation_os */
const io_function_ptrs_t* ftd_dynamic_activation_get_replacement_routines(void)
{
    static aio_ops_t captured_driver_replacement_aio_ops = {minphys, captured_driver_replacement_strategy, NULL, 8};
    
    static const io_function_ptrs_t replacement_routines = {captured_driver_replacement_strategy,
                                                            captured_driver_replacement_read,
                                                            captured_driver_replacement_write,
                                                            &captured_driver_replacement_aio_ops,
                                                            &captured_driver_replacement_aio_ops};
    
    return &replacement_routines;
}
