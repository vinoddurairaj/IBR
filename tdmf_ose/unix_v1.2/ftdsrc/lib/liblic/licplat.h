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
/*========================================================================
 * licplat.h
 *
 * This file contains non-portable, OS & Hardware specific function call
 * prototypes needed in support of the license key management library.
 *
 * Copyright (c) 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved.
 *========================================================================*/
#ifndef _LICPLAT_H_
#define _LICPLAT_H_

typedef enum hostid_purpose
{
    HOSTID_LICENSE,       /**< An identifier for the purpose of licensing.
                               Unaltered by the lparid.cfg file */
    HOSTID_IDENTIFICATION /**< An identifier for the purpose of identifying the agent.
                               Altered by the lparid.cfg file. */
} hostid_purpose_t;

#ifdef TDMFUNIXGENUS
// This code is borrowed by TDMF UNIX (not for Replicator / TDMF IP)
extern int      my_gethostid(void);
#else
extern int      my_gethostid(hostid_purpose_t purpose);
#endif

extern char*    my_get_machine_model(void);

#endif	/* _LICPLAT_H_ */




