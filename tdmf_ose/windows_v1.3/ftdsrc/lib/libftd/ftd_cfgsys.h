/*
 * ftd_cfgsys.h - system config file interface
 *
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef _FTD_CFGSYS_H_
#define _FTD_CFGSYS_H_

#if defined(_WINDOWS)
#define FTD_DRIVER_KEY "System\\CurrentControlSet\\Services\\" DRIVERNAME
#define FTD_PARAMETERS_KEY "Parameters"

#define FTD_DRIVER_PARAMETERS_KEY_TYPE		1
#define FTD_DRIVER_KEY_TYPE				    2

#endif

/* function return values, if anyone cares */
#define CFG_OK                  0
#define CFG_BOGUS_PS_NAME      -1
#define CFG_MALLOC_ERROR       -2
#define CFG_READ_ERROR         -3
#define CFG_WRITE_ERROR        -4
#define CFG_BOGUS_CONFIG_FILE  -5

#define CFG_IS_NOT_STRINGVAL   0
#define CFG_IS_STRINGVAL       1

#if !defined(_WINDOWS)
extern int cfg_get_key_value(char *key, char *value, int stringval);
extern int cfg_set_key_value(char *key, char *value, int stringval);

#define cfg_get_driver_key_value(a, b, c)	cfg_get_key_value(a, b, c)
#define cfg_get_software_key_value(a, b, c) cfg_get_key_value(a, b, c)
#define cfg_set_driver_key_value(a, b, c)	cfg_set_key_value(a, b, c)
#define cfg_set_software_key_value(a, b, c) cfg_set_key_value(a, b, c)

#else
extern int cfg_get_driver_key_value(char *key, int regkeytype, char *value, int stringval);
extern int cfg_set_driver_key_value(char *key, int regkeytype, char *value, int stringval);
extern int cfg_get_software_key_value(char *key, char *value, int stringval);
extern int cfg_set_software_key_value(char *key, char *value, int stringval);
extern int cfg_get_software_key_sz_value(char *key, char *value, int value_size);
#endif

#endif /* _FTD_CFGSYS_H_ */
