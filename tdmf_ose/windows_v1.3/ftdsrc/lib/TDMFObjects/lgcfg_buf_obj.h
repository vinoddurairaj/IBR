/*
 * lgcfg_buf_obj.h   -- Utility functions to convert CTDMFLogicalGroup
 *                      objects to text buffer containing the text of a 
 *                      logical group configuration files (.cfg)
 *
 * Copyright (c) 2002 Fujitsu Softek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */


class CReplicationGroup;

void tdmf_create_lgcfgfile_buffer(/*in*/ CReplicationGroup *repGroup, 
                                  ///*in*/ CServer *srvrSource, 
                                  ///*in*/ CServer *srvrTarget, 
                                  /*in*/ bool bIsPrimary, 
                                  /*out*/char **ppCfgData, 
                                  /*out*/unsigned int *piCfgDataSize ,
								  /*in*/ bool bSymmetric
                                  );
void tdmf_fill_objects_from_lgcfgfile(  char *pCfgData, unsigned int iCfgDataSize, 
                                        bool bSourceSrvr, bool bIsWindows,
										short sGrpNumber, CReplicationGroup & replGrp,
                                        std::string  &  strHostName,
                                        std::string  &  strOtherHostName
                                        );
  