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
#ifndef _CFG_INTR_H_
#define _CFG_INTR_H_

/*
 * cfg_intr.h - system config file interface
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

/* function return values, if anyone cares */
#define CFG_OK                  0
#define CFG_BOGUS_PS_NAME      -1
#define CFG_MALLOC_ERROR       -2
#define CFG_READ_ERROR         -3
#define CFG_WRITE_ERROR        -4
#define CFG_BOGUS_CONFIG_FILE  -5

#define CFG_IS_NOT_STRINGVAL   0
#define CFG_IS_STRINGVAL       1

int cfg_get_key_value(char *key, char *value, int stringval);
int cfg_set_key_value(char *key, char *value, int stringval);
void dtc_open_err_check_driver_config_file_and_log_error_message(int error_code);
#if defined(linux)
int update_config_value(void);
#endif /* defined(linux) */

#endif /* _CFG_INTR_H_ */
