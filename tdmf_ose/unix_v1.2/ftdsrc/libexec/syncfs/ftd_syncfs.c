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
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#if SYSVERS!=430
#  include <j2/j2_types.h>
#  include <j2/j2_cntl.h>
#endif
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <dlfcn.h>

/* TODO - Need to modify Makefile to test for existence of syncvfs.h */
/* Make sure FS_SYNCVFS_QUIESCE is always defined */

#ifdef HAS_SYNCVFS
#  include <sys/syncvfs.h>
#  if FS_SYNCVFS_QUIESCE != 0x02
#    error - The value for FS_SYNCVFS_QUIESCE has changed.
#    error - Correct the define below for non syncvfs environments
#  endif
#else
#  define FS_SYNCVFS_QUIESCE 0x02
#  define FS_SYNCVFS_FS 0x20
#endif

#ifndef FSCNTL_QSFS
#  define FSCNTL_QSFS 7
#endif

/* No prototype for fscntl exists on some maintenance releases */

extern int fscntl ( uint_t vfs_id, int command, char *arg, int argsize );

/*
 * syncvfs started to be available in a 5.2 maintenance release 
 * Instead of compiling in the code we will use a runtime check instead
 */ 

typedef int (*p_syncvfs_t)( char *, int );

p_syncvfs_t syncvfs_func = NULL; /* function pointer to syncvfs call */

void *dll_fd = NULL;	/* handle for the dll */

/* Load up syncvfs function pointer */

void dll_load()
{
    dll_fd = dlopen( "libc.a(shr.o)", RTLD_LAZY | RTLD_MEMBER );
    if (dll_fd)
    {
	syncvfs_func = (p_syncvfs_t)dlsym( dll_fd, "syncvfs" );
    }
    else
    {
	syncvfs_func = NULL;
    }
}

void dll_close()
{
    if (dll_fd)
	dlclose( dll_fd );
}

void atexit_callback()
{
    dll_close();
}

char *argv0;

Usage( int exit_code )
{
    printf( "Usage: %s -f mount_points...\n\n", basename(argv0) );
    printf( "%14sForces JFS2 journals to be applied to the data disk\n\n", "");
    printf( "\t-f\tSpecify a list of mount points that need to be flushed\n");
    exit( exit_code );
}

int fscntl_qsfs( char *mount_point )
{
    struct stat stat_buf;
    int err;

    stat( mount_point, &stat_buf );

    err = fscntl( stat_buf.st_vfs, FSCNTL_QSFS, 0, 0 );
    if (err)
    {
	perror("syncfs.fscntl_qsfs" );
    }
    return err;
}

int syncvfs_fs( char *mount_point )
{
    int err;

    if (syncvfs_func != NULL)
    {
	err = (*syncvfs_func)( mount_point, FS_SYNCVFS_FS | FS_SYNCVFS_QUIESCE );
	if (err)
	{
	    perror("syncfs.syncvfs_fs" );
	}
    }
    else
    {
	err = fscntl_qsfs( mount_point );
    }
    return err;
}

typedef enum { NONE = 0, MOUNT_POINT, LOGICAL_GROUP } e_arg_type; 

main (int argc, char **argv)
{
    /* TODO only allow root to execute */

    e_arg_type mode = NONE;
    int err = 0;

    atexit( atexit_callback );

    dll_load();

    argv0=argv[0];
    if (argc <= 1)
    {
	Usage(1);
    }

    for (int i = 1; i <argc; i++)
    {
	if (argv[i][0] ==  '-')
	{
	    if (argv[i][1] == 'f') mode = MOUNT_POINT;
	    else if (argv[i][1] == 'g') mode = LOGICAL_GROUP;
	    else
	    {
		fprintf(stderr, "Invalid command line argument\n");
		Usage(1);
	    }

	}
	else
	{
	    if (mode == MOUNT_POINT)
	    {
		if (syncvfs_func == NULL)
		{
		    err |= fscntl_qsfs( argv[i] );
		}
		else
		{
		    err |= syncvfs_fs( argv[i] );
		}
	    }
	    else
	    {
		fprintf(stderr, "No command line flag specified\n");
		Usage(1);
	    }
	}
    }
    exit(err?1:0);
}
