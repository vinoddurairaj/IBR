/*
 * panalyze.c - 
 *
 * This program will return the amount of journal size is required
 * to be able to do a smart-refresh from the source to the target
 *
 * NOTE: this is an absolute minimum! It does not take into consideration
 * any amount of data transfers between the primary and the secondary
 * while doing the refresh.
 */

#include "ftd_sock.h"
#include "ftd_ps.h"
#include "ftd_ioctl.h"
#include "ftd_error.h"
#include "ftd_devlock.h"

static int      verify  = FALSE;
static int      verbose = FALSE;
static char     *statstr, *argv0;
static int      targets[FTD_MAX_GRP_NUM];
static LList    *cfglist;

/*
 * usage -- This is how the program is used
 */
void
usage(void)
{
    fprintf(stderr,"\nUsage: %s <options>\n\n", argv0);
    
    fprintf(stderr,"\t\t-g <group_num>\t: group number to analyze\n");

    exit(1);
}

/*
 * proc_argv -- process argument vector 
 */
void
proc_argv(int argc, char **argv)
{
    int             devcnt, ch;
    unsigned long   lgnum = 0;

    argv0 = strdup(argv[0]);

    // At least one argument is required 
    if (argc == 1) 
    {
        usage();
    }
    devcnt = 0;

    while ((ch = getopt(argc, argv, "vg:")) != -1) 
    {
        switch (ch) 
        {
        case 'g':
            lgnum = strtol(optarg, NULL, 10);
            targets[lgnum] = 1;
            break;

        case 'v':
            verbose = TRUE;
            break;

        default:
            usage();
            break;
        }
    }
 
    return;
}

char*
format_drive_size(unsigned int uiSize)
{
	static char szSize[64];
	char szIn[32];
	char szOut[32];
	char LpLCData[255];
	int cchData = 255;

	if (uiSize < 1024)
	{
		_snprintf(szSize, 64, "%d KB", uiSize);
	}
	else if (uiSize < (1024*1024))
	{
		LCID Locale = GetSystemDefaultLCID();
		GetLocaleInfo(Locale, LOCALE_IDIGITS, LpLCData, cchData); 
		SetLocaleInfo(Locale, LOCALE_IDIGITS, "0");

		_snprintf(szIn, 32, "%d", uiSize);
		if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szIn, NULL, szOut, 32) == 0)
		{
			strcpy(szOut, szIn);
		}
		_snprintf(szSize, 64, "%.1f MB (%s KB)", (double)uiSize/1024, szOut);

		SetLocaleInfo(Locale, LOCALE_IDIGITS, LpLCData);
	}
	else
	{
		LCID Locale = GetSystemDefaultLCID();
		GetLocaleInfo(Locale, LOCALE_IDIGITS, LpLCData, cchData); 
		SetLocaleInfo(Locale, LOCALE_IDIGITS, "1");

		_snprintf(szIn, 32, "%.1f", (double)uiSize/1024);
		if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szIn, NULL, szOut, 32) == 0)
		{
			strcpy(szOut, szIn);
		}
		_snprintf(szSize, 64, "%.2f GB (%s MB)", (double)uiSize/(1024*1024), szOut);

		SetLocaleInfo(Locale, LOCALE_IDIGITS, LpLCData);
	}

	return szSize;
}

static int
verify_journal_size(int lgnum)
{
    ftd_lg_t            *lgp;
    unsigned int        i;
    int                 rc;
    int                 bhighres    = FALSE;
    int                 blowres     = FALSE;
    int                 checkpoint  = -1;
    ftd_dev_info_t      *dip        = NULL;
    u_char              *hrdb       = NULL;
    u_char              *lrdb       = NULL;
    ps_group_info_t     group_info;
    ps_version_1_attr_t group_attributes;
    char                group_name[MAX_PATH];
    char                dev_name[FTD_MAX_DEVICES][MAX_PATH];
    ps_dev_info_t       dev_info[FTD_MAX_DEVICES];
    
    char *              LowResBitmap[FTD_MAX_DEVICES];
    unsigned int        LowResBitmapSize[FTD_MAX_DEVICES];
    dirtybit_array_t    dbHighResBitmap;
    dirtybit_array_t    dbLowResBitmap;

    unsigned int        LRDBoffset32        = 0;
    unsigned int        HRDBoffset32        = 0;
    unsigned int        uiMBrequiredLRDB    = 0;
    unsigned int        uiMBrequiredHRDB    = 0;

    //
    // Initialize all variables and structures
    //
    memset(dev_name,0,sizeof(dev_name));

    for (i=0;i<FTD_MAX_DEVICES;i++)
    {
        dev_info[i].name = dev_name[i];
    }

    memset(group_name,0,sizeof(group_name));
    group_info.name = group_name;

    memset(LowResBitmap,0,sizeof(LowResBitmap));

    dbHighResBitmap.devs    = NULL;
    dbLowResBitmap.devs     = NULL;
    dbHighResBitmap.numdevs = 0;
    dbLowResBitmap.numdevs  = 0;

    //
    // end init
    //
    if (ftd_dev_lock_create() == -1) 
        goto errret;

    // Verify checkpoint status for down group
    if ((lgp = ftd_lg_create()) == NULL) 
    {
        fprintf(stderr,"ftd_lg_create() failed\n");
        goto errret;
    }

    if (ftd_lg_init(lgp, lgnum, ROLEPRIMARY, 1) < 0) 
    {
        fprintf(stderr,"ftd_lg_init() failed\n");
        goto errret;
    }

    if ((rc = ftd_lg_open(lgp)) < 0)
        goto errret;
    
    //group_info.name = NULL;
    if ((rc = ps_get_group_info(lgp->cfgp->pstore, lgp->devname, &group_info)) != PS_OK) 
    {
        fprintf(stderr,"ps_get_group_info() failed\n");
        goto errret;
    }
    

    if ((rc = ps_get_version_1_attr(lgp->cfgp->pstore, &group_attributes)) != PS_OK)
    {
        fprintf(stderr,"ps_get_version_1_attr() failed\n");
        goto errret;
    }

    //
    // get device name list
    //
    if ((rc = ps_get_device_list(lgp->cfgp->pstore, (char *)dev_name, sizeof(dev_name)) != PS_OK))
    {
        fprintf(stderr,"ps_get_device_list() failed\n");
        goto errret;
    }

    for (i = 0; i< group_attributes.num_device;i++)
    {
        //
        // get device stats list
        //
        if ((rc = ps_get_device_info(lgp->cfgp->pstore,dev_name[i],&dev_info[i]) != PS_OK))
        {
            fprintf(stderr,"ps_get_device_info() failed\n");
            goto errret;
        }
        else
        {
            //
            // Get bitmap from disk!
            //
            LowResBitmap[i]     = malloc(8*1024);
            memset(LowResBitmap[i],0,8*1024);
            LowResBitmapSize[i] = 0;

            if ((rc = ps_get_lrdb(lgp->cfgp->pstore,dev_name[i],LowResBitmap[i],8*1024,&LowResBitmapSize[i])) != PS_OK)
            {
                fprintf(stderr,"ps_get_lrdb() failed\n");
                goto errret;
            }
        }
    }

    if ((dip = (ftd_dev_info_t*)calloc(group_attributes.num_device, sizeof(ftd_dev_info_t))) == NULL) 
    {
        fprintf(stderr,"Unable to allocate memory for device names\n");
        goto errret;
    }

    //
    // Allocate space for low resolution dirty bits data (LRDB)
    //
    if ((lrdb = (u_char*)calloc(group_attributes.num_device, FTD_PS_LRDB_SIZE)) == NULL) 
    {
        fprintf(stderr,"Unable to allocate memory for LRDB's\n");
        goto errret;
    }

    dbLowResBitmap.dbbuf = (int*)lrdb;
    
    if ((rc = ftd_ioctl_get_lrdbs(lgp->ctlfd, lgp->devfd, lgp->lgnum, &dbLowResBitmap, dip)) < 0)
    {
        fprintf(stderr,"Unable to read the HRDB from the driver\n");
        blowres = FALSE;
    }
    else
    {
        blowres = TRUE;
    }

    //
    // Allocate space for high resolution dirty bits data (HRDB)
    //
    if ((hrdb = (u_char*)calloc(group_attributes.num_device, FTD_PS_HRDB_SIZE)) == NULL) 
    {
        fprintf(stderr,"Unable to allocate memory for HRDB's\n");
        goto errret;
    }   

    dbHighResBitmap.dbbuf = (int*)hrdb;

    if ((rc = ftd_ioctl_get_hrdbs(lgp->ctlfd, lgp->devfd, lgp->lgnum, &dbHighResBitmap, dip)) < 0)
    {
        fprintf(stderr,"Unable to read the HRDB from the driver\n");
        bhighres = FALSE;
    }
    else
    {
        bhighres = TRUE;
    }

    if (!blowres && !bhighres)
    {
        fprintf(stderr,"Unable to read the Bitmaps from the driver\n");
        goto errret;
    }

    //
    // Calculate needed journal space!
    //
    for (i = 0; i< group_attributes.num_device;i++)
    {
        unsigned char *          HRDB   = (unsigned char *) (dbHighResBitmap.dbbuf + HRDBoffset32);
        unsigned char *          LRDB   = (unsigned char *) (dbLowResBitmap.dbbuf + LRDBoffset32);//LowResBitmap[i]);
        unsigned int    j               = 0;
        unsigned int    total           = 0;
        double          dPercentage     = 0.0;
        unsigned int    uiDiskRequired  = ((dip[i].disksize / 2) + 1);

        //
        // skip empty devices
        //
        if ((dip[i].hrdb_numbits==0) && (dip[i].lrdb_numbits==0))
        {
            continue;
        }

        if (verbose)
        {
            fprintf(stderr, 
                    "\nSize for device: %s Disk %s\n", 
                    dev_info[group_attributes.num_device - i -1].name,
					format_drive_size(uiDiskRequired));
        }

        //
        // check % of bitmap is in use
        //
        if (bhighres) // use high resolution bitmaps if possible (don't use pstore if at all possible!)
        {
            //
            // Counting bits is just a matter of adding up each byte of the bitmap
            //
            for (j = 0; j < (unsigned int)(dip[i].hrdb_numbits/8); j++)
            {
                unsigned char g = HRDB[j];
                g = (g & 0x55u) + ((g >> 1) & 0x55u);
                g = (g & 0x33u) + ((g >> 2) & 0x33u);
                g = (g & 0x0fu) + ((g >> 4) & 0x0fu);
                total += g;
            }

            //
            // Get percentage of bits dirty
            //
            dPercentage   = (double)total / (double)dip[i].hrdb_numbits;

            uiDiskRequired = (unsigned int)(((dip[i].disksize / 2) + 1) * dPercentage);

            if (verbose)
            {
                fprintf(stderr,
                        "HRDB usage is %d/%dbits %2.3f%% - disk size required is %s\n", 
                        total,
                        dip[i].hrdb_numbits,
                        dPercentage*100,
						format_drive_size(uiDiskRequired));
            }

            uiMBrequiredHRDB += uiDiskRequired;
        }

        if (blowres) // use low resolution bitmaps from driver if possible (don't use pstore)
        {
            total = 0;

            for (j = 0; j < (unsigned int)(dip[i].lrdb_numbits/8); j++)
            {
                    unsigned char g = LRDB[j];
                    g = (g & 0x55u) + ((g >> 1) & 0x55u);
                    g = (g & 0x33u) + ((g >> 2) & 0x33u);
                    g = (g & 0x0fu) + ((g >> 4) & 0x0fu);
                    total += g;
            }

            //
            // dump sizes
            //
            //
            // Get percentage of bits dirty
            //
            dPercentage   = (double)total / (double)dip[i].lrdb_numbits;

            uiDiskRequired = (unsigned int)(((dip[i].disksize / 2) + 1) * dPercentage);

            if (verbose)
            {
                fprintf(stderr,
                        "LRDB usage is %d/%dbits %2.3f%% - disk size required is %s\n", 
                        total,
                        dip[i].lrdb_numbits,
                        dPercentage*100, 
                        format_drive_size(uiDiskRequired));
            }

            uiMBrequiredLRDB += uiDiskRequired;
        }

        HRDBoffset32 += dip[i].hrdbsize32;
        LRDBoffset32 += dip[i].lrdbsize32;
    }

    if (verbose)
    {
        fprintf(stderr,"\nDisk size required for group %d is\n",lgnum);

        if (bhighres)
        {
            fprintf(stderr, 
                    "HRDB based size requires %s for journals\n",
                    format_drive_size(uiMBrequiredHRDB) );
        }

        if (blowres)
        {
            fprintf(stderr, 
                    "LRDB based size requires %s for journals\n",
                    format_drive_size(uiMBrequiredLRDB) );
        }
    }
    else
    {
        if (bhighres)
        {
            fprintf(stderr, 
                    "Journal size required for group %d is %s\n",
                    lgnum,
                    format_drive_size(uiMBrequiredHRDB) );
        }
        else
        {
            fprintf(stderr, 
                    "Journal size required for group %d is %s\n",
                    lgnum,
                    format_drive_size(uiMBrequiredLRDB) );
        }
    }


errret:

    for (i=0;i<FTD_MAX_DEVICES;i++)
    {
        if (LowResBitmap[i])
            free(LowResBitmap[i]);
    }

    if (dip)
        free (dip);
    if (hrdb)
        free (hrdb);
    if (lrdb)
        free (lrdb);

    ftd_lg_close(lgp);
    ftd_lg_delete(lgp);
    ftd_dev_lock_delete();

    return checkpoint;
}

/*
 * main 
 */
int main (int argc, char **argv, char **envp)
{
    ftd_lg_cfg_t    **cfgpp;
    int             found;

    memset(targets, 0, sizeof(targets));

    fprintf (   stderr,
                "panalyze for %s \n",
#ifdef NTFOUR
                "NT");
#else
                "W2K+");
#endif
                

    if (ftd_init_errfac("Replicator", "PANALYZE", NULL, NULL, 1, 1) == NULL) 
        goto errexit;


    if ((cfglist = ftd_config_create_list()) == NULL) 
        goto errexit;
    
    
    if (ftd_config_get_primary_started(PATH_CONFIG, cfglist) < 0) 
        goto errexit;

    proc_argv(argc, argv);
 
    if (SizeOfLL(cfglist) == 0) 
    {
        fprintf(stderr, "Target group %d not started/does not exist.\n");
        goto errexit;
    }

    // at least one target must be specified 
    found = 0;
    ForEachLLElement(cfglist, cfgpp) 
    {
        if (targets[(*cfgpp)->lgnum]) 
        {
            found = 1;
        }
    }

    if (!found) 
    {
        fprintf(stderr, "Target group %d not started/does not exist.\n");
        goto errexit;
    }

    ForEachLLElement(cfglist, cfgpp) 
    {
        if (!targets[(*cfgpp)->lgnum]) 
        {
            continue;
        }
    
        //
        // Just verify journal sizes for group(s)
        //
        verify_journal_size((*cfgpp)->lgnum);

    }

errexit:

    exit(0);
}
