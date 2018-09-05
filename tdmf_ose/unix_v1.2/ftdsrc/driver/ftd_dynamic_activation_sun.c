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
/** @ingroup dynamic_activation_os */
io_function_ptrs_t ftd_dynamic_activation_swap_io_routines(swap_table_key_t block_swap_key,
                                                           dev_t block_devno,
                                                           swap_table_key_t char_swap_key,
                                                           const io_function_ptrs_t* replacement_routines)
{
    struct dev_ops  *dp;
    io_function_ptrs_t old_routines = {NULL, NULL, NULL};
    int major = block_swap_key;
    
    ASSERT(block_swap_key == char_swap_key);
    ASSERT(block_swap_key == getmajor(block_devno));
    
    dp = ddi_hold_installed_driver(major);

    old_routines.strategy = dp->devo_cb_ops->cb_strategy;
    old_routines.read = dp->devo_cb_ops->cb_read;
    old_routines.write = dp->devo_cb_ops->cb_write;
    old_routines.aread = dp->devo_cb_ops->cb_aread;
    old_routines.awrite = dp->devo_cb_ops->cb_awrite;
        
    dp->devo_cb_ops->cb_strategy = replacement_routines->strategy;
    dp->devo_cb_ops->cb_read = replacement_routines->read;
    dp->devo_cb_ops->cb_write = replacement_routines->write;
    dp->devo_cb_ops->cb_aread = replacement_routines->aread;
    dp->devo_cb_ops->cb_awrite = replacement_routines->awrite;

    ddi_rele_driver(major);
    
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
 * @note In general, strategy() should not block.  It  can,  however,
 *       perform a kmem_cache_alloc(9F) with both the KM_PUSHPAGE and
 *       KM_SLEEP flags  set,  which  might  block,  without  causing
 *       deadlock in low memory situations.
 *
 * @sa strategy(9E)
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
 * @return he platform specific standard return codes for the strategy function.
 *
 * @internal The signature needs to conform to the generic_pass_io_through_func_ptr.
 */
static int call_captured_strategy(const captured_io_parameters_t* io_params, const io_function_ptrs_t* original_io_routines)
{
    return original_io_routines->strategy(io_params->params.strategy.bp);
}

/**
 * @brief Our replacement strategy function that receives all captured block IO requests.
 */
static int captured_driver_replacement_strategy(struct buf *userbp)
{
    captured_io_parameters_t io_params;
    io_params.params.strategy.bp = userbp;
    
    return ftd_dynamic_activation_captured_driver_generic_replacement_routine(userbp->b_edev,
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
                                      io_params->params.read_write.uio,
                                      io_params->params.read_write.cred_p);
}

/**
 * @brief Our replacement read function that receives all captured read IO requests.
 */
static int captured_driver_replacement_read(dev_t devno, struct uio* uio, cred_t* cred_p)
{
    captured_io_parameters_t io_params;
    io_params.params.read_write.dev = devno;
    io_params.params.read_write.uio = uio;
    io_params.params.read_write.cred_p = cred_p;
    
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
                                       io_params->params.read_write.uio,
                                       io_params->params.read_write.cred_p);
}

/**
 * @brief Our replacement write function that receives all captured write IO requests.
 */
static int captured_driver_replacement_write(dev_t devno, struct uio* uio, cred_t* cred_p)
{
    captured_io_parameters_t io_params;
    io_params.params.read_write.dev = devno;
    io_params.params.read_write.uio = uio;
    io_params.params.read_write.cred_p = cred_p;
    
    return ftd_dynamic_activation_captured_driver_generic_replacement_routine(devno,
                                                                              &io_params,
                                                                              process_captured_write_io,
                                                                              call_captured_write);
}

/***************************************************************************
 * Asynchronous read function handling
 ***************************************************************************/

/**
 * @brief Asynchronous r/w function.
 *
 * Used to send captured asynchronous read/write requests to our dtc device's strategy function.
 *
 * Based on the ftd_rw function.
 *
 */
static int ftd_arw (dev_t dev, struct aio_req * aioreq_p, ftd_int32_t flag)
{
    ftd_int32_t rc = ftd_rw_validate_params(dev, aioreq_p->aio_uio);

    if(rc == 0)
    {
        rc = aphysio(ftd_strategy, anocancel, dev, flag, minphys, aioreq_p);
    }

    return rc;
}

/**
 * Processes (replicates) an IO request received by our replacement aread function.
 *
 * @param io_params The parameters of the captured IO request.
 *
 * @return The platform specific standard return codes for the aread function.
 *
 * @internal The signature needs to conform to the generic_process_io_func_ptr type.
 *
 */
static int process_captured_async_read_io(const captured_io_parameters_t* io_params)
{
    return ftd_arw(io_params->params.aread_write.dev, io_params->params.aread_write.aioreq_p, B_READ);
}

/**
 * @brief Forwards an IO by directly calling the captured aread routine.
 *
 * @param io_params             The parameters of the captured IO request.
 * @param original_io_routines  The IO routines that were replaced.
 *
 * @return The platform specific standard return codes for the aread function.
 *
 * @internal The signature needs to conform to the generic_pass_io_through_func_ptr.
 */
static int call_captured_async_read(const captured_io_parameters_t* io_params, const io_function_ptrs_t* original_io_routines)
{
    return original_io_routines->aread(io_params->params.aread_write.dev,
                                       io_params->params.aread_write.aioreq_p,
                                       io_params->params.aread_write.cred_p);
}

/**
 * @brief Our replacement aread function that receives all captured async read IO requests.
 */
static int captured_driver_replacement_async_read(dev_t devno, struct aio_req* aioreq_p, cred_t* cred_p)
{
    captured_io_parameters_t io_params;
    io_params.params.aread_write.dev = devno;
    io_params.params.aread_write.aioreq_p = aioreq_p;
    io_params.params.aread_write.cred_p = cred_p;
    
    return ftd_dynamic_activation_captured_driver_generic_replacement_routine(devno,
                                                                              &io_params,
                                                                              process_captured_async_read_io,
                                                                              call_captured_async_read);
}


/***************************************************************************
 * Asynchronous write function handling
 ***************************************************************************/

/**
 * Processes (replicates) an IO request received by our replacement awrite function.
 *
 * @param io_params The parameters of the captured IO request.
 *
 * @return The platform specific standard return codes for the awrite function.
 *
 * @internal The signature needs to conform to the generic_process_io_func_ptr type.
 *
 */
static int process_captured_async_write_io(const captured_io_parameters_t* io_params)
{
    return ftd_arw(io_params->params.aread_write.dev, io_params->params.aread_write.aioreq_p, B_WRITE);
}

/**
 * @brief Forwards an IO by directly calling the captured awrite routine.
 *
 * @param io_params             The parameters of the captured IO request.
 * @param original_io_routines  The IO routines that were replaced.
 *
 * @return The platform specific standard return codes for the awrite function.
 *
 * @internal The signature needs to conform to the generic_pass_io_through_func_ptr.
 */
static int call_captured_async_write(const captured_io_parameters_t* io_params, const io_function_ptrs_t* original_io_routines)
{
    return original_io_routines->awrite(io_params->params.aread_write.dev,
                                        io_params->params.aread_write.aioreq_p,
                                        io_params->params.aread_write.cred_p);
}

/**
 * @brief Our replacement awrite function that receives all captured async write IO requests.
 */
static int captured_driver_replacement_async_write(dev_t devno, struct aio_req* aioreq_p, cred_t* cred_p)
{
    captured_io_parameters_t io_params;
    io_params.params.aread_write.dev = devno;
    io_params.params.aread_write.aioreq_p = aioreq_p;
    io_params.params.aread_write.cred_p = cred_p;
    
    return ftd_dynamic_activation_captured_driver_generic_replacement_routine(devno,
                                                                              &io_params,
                                                                              process_captured_async_write_io,
                                                                              call_captured_async_write);
}


// See the ftd_dynamic_activation.h for the documentation.
/** @ingroup dynamic_activation_os */
const io_function_ptrs_t* ftd_dynamic_activation_get_replacement_routines(void)
{
    static const io_function_ptrs_t replacement_routines = {captured_driver_replacement_strategy,
                                                            captured_driver_replacement_read,
                                                            captured_driver_replacement_write,
                                                            captured_driver_replacement_async_read,
                                                            captured_driver_replacement_async_write};
    
    return &replacement_routines;
}
