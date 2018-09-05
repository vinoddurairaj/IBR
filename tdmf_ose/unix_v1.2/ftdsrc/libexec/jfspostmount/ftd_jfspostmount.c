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
#if defined(_AIX)

/*
 * Copyright (c) 2003 Softek Technology Corporation. All rights reserved.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <libgen.h>
#include <jfs/filsys.h>
#include "errors.h"
#include "pathnames.h"

static char *progname = NULL;
extern int errno;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s <data_device>\n", progname);
	fprintf(stderr, "\tThe device name must be full path name.\n");
}

main(int argc, char *argv[])
{
	int	ch;
	char	datadev[MAXPATHLEN];
	char	logdev[MAXPATHLEN];
	char	devidfile[MAXPATHLEN];
	char	buf[PAGESIZE];
	struct superblock	sb;
	int	fd;
	int	rc;
	int	woff;
	FILE	*fp;
	dev_t	devid;

	putenv("LANG=C");

	/* Make sure we are root */
	if (geteuid()) {
		fprintf(stderr, "You must be root to run this process...aborted\n");
		exit(1);
	}

	progname = argv[0];

	if (argc != 2) {
		/* lack of arguments or too many arguments */
		fprintf(stderr, "Invalid arguments\n");
		usage();
		exit(1);
	}

	opterr = 0;
	while ((ch =getopt(argc, argv, "h")) != -1) {
		switch (ch) {
		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	if (argv[1][0] != '/') {
		fprintf(stderr, "Invalid device name\n");
		usage();
		exit(1);
	}

	if (initerrmgt(ERRFAC) < 0) {
		exit(1);
	}

	log_command(argc, argv);	/* trace the command in dtcerror.log */

	(void) strcpy(datadev, argv[1]);

	fd = open(datadev, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "%s open failed - %s\n",
					datadev, strerror(errno));
		exit(1);
	}

	sprintf(devidfile, "%s/%s.premount", PATH_RUN_FILES, basename(datadev));
	if ((fp = fopen(devidfile, "r")) == NULL) {
		fprintf(stderr, "%s open failed - %s\n",
					devidfile, strerror(errno));
		close(fd);
		exit(1);
	}

	rc = fread((void *)&devid, (size_t)1, sizeof(dev_t), fp);
	if (rc != sizeof(dev_t)) {
		fprintf(stderr, "%s read failed - %s\n",
					devidfile, strerror(errno));
		fclose(fp);
		close(fd);
		exit(1);
	}
	fclose(fp);

	/*
	 * seek to head of the superblock structure.
	 *   primary superblock is located in block (PAGE) one.
	 *   see /usr/include/jfs/filsys.h
	 */
	if (lseek(fd, PAGESIZE, SEEK_SET) == -1) {
		fprintf(stderr, "%s seek failed - %s\n",
					datadev, strerror(errno));
		close(fd);
		exit(1);
	}

	/*
	 * read one block (PAGE)
	 */
	if (read(fd, buf, sizeof(sb)) != sizeof(sb)) {
		fprintf(stderr, "%s read failed - %s\n",
					datadev, strerror(errno));
		close(fd);
		exit(1);
	}

	memcpy((char *)&sb, buf, sizeof(sb));

	/*
	 * check the filesystem type from superblock
	 */
	if (strncmp(sb.s_magic, fsv3magic, 4) != 0 &&
	    strncmp(sb.s_magic, fsv3pmagic, 4) != 0) {
		fprintf(stderr, "%s is not JFS\n", datadev);
		close(fd);
		exit(1);
	}

	/*
	 * seek to s_logdev member of the superblock structure
	 */
	woff = (int)(&sb.s_logdev) - (int)(&sb.s_magic[0]);
	if (lseek(fd, PAGESIZE + woff, SEEK_SET) == -1) {
		fprintf(stderr, "%s seek failed - %s\n",
					datadev, strerror(errno));
		close(fd);
		exit(1);
	}

	/*
	 * modify s_logdev to new log device id
	 */
	rc = write(fd, &devid, sizeof(dev_t));
	if (rc != sizeof(dev_t)) {
		fprintf(stderr, "%s write failed - %s\n",
					datadev, strerror(errno));
		close(fd);
		exit(1);
	}

	close(fd);
	unlink(devidfile);
	exit(0);
}
#endif /* defined(_AIX) */
