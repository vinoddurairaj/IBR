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
#ifndef _STAT_INTR_H_
#define _STAT_INTR_H_

/*
 * stat_intr.h - FTD Statistics interface
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

int initdvrstats(void);
void initstats(machine_t *mysys);
int savestats(int force, int network_bandwidth_analysis_mode);
void dumpstats(machine_t *sys, int network_bandwidth_analysis_mode);
void dump_pmd_stats(machine_t *sys, int network_bandwidth_analysis_mode);
void dump_rmd_stats(machine_t *sys);
void init_csv_stats( machine_t *mysys, int net_analysis_duration, char *fictitious_dtc_dev_name );
int	 save_csv_stats( int net_analysis_duration, char *fictitious_dtc_dev_name );
void dump_csv_stats(machine_t *sys);
void save_network_analysis_total( machine_t *sys, int net_analysis_duration, u_longlong_t net_analysis_total_KBs, char *fictitious_dtc_dev_name );
int  calculate_product_usage_file_checksum( char *product_usage_tracking_file, unsigned char *checksum_digests_ptr );
int  verify_product_usage_file_checksum( char *product_usage_tracking_file, char *product_usage_checksum_file );
int  set_product_usage_checksum_failed( char *product_usage_tracking_file );
int  update_product_usage_checksum_file( char *product_usage_tracking_file, char *product_usage_checksum_file );
int  update_product_usage_tracking_file( machine_t *mysys, int lgnum, int Full_Refresh_start, int Full_Refresh_completion, int register_devices );

/*
 NOTE: for the Product usage tracking file (pxxx_migration_tracking.csv) checksum buffer allocation:
       if we consider a possibility of 1024 device pairs in a group (maximum) and allow for an average
	   of 120 characters per line in the tracking file, the file on which we calculate the checksum could
	   potentially be 128 KB in size; one checksum digest is calculated on a segment of max 32 KB; so
	   we could potentially need 128 KB / 32 KB = 4 checksum digests. To be safe, we allocate four times this
	   amount.
*/
#define MAX_PROD_USAGE_CHECKSUM_DIGESTS 16


#if defined(HPUX)
#if (SYSVERS >= 1100)
#define ISNANF(x) isnan(x)
#else /* (SYSVERS >= 1100) */
#define ISNANF(x) isnanf(x)
#endif /* (SYSVERS >= 1100) */
#endif /* defined(HPUX) */
#endif /* _STAT_INTR_H_ */
