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
/**************************************************************************
 * ftd_pcmp.c
 *
 *   (c) Copyright 2005 Softek Solutions, Inc.  All Rights Reserved 
 *
 *   Description: Compare volumes block-by-block (or by volume md5 checksum).
 *      Input volume names are specified by name-pairs in an input file, or
 *	    as primary/mirror pairs extracted from volume group configuration files
 *	    (pxxx.cfg).
 *
 *  $Id: ftd_pcmp.c,v 1.6 2010/12/20 20:21:18 dkodjo Exp $
 *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/param.h>
#include <locale.h>
#if defined(linux)
#include <time.h>
#endif /* defined(linux) */

#include <stropts.h> /* for HP-UX ioctl */

#include "pathnames.h"
#include "config.h"
#include "errors.h"
#include "ps_intr.h"
#include "cfg_intr.h"
#include "ftd_cmd.h"
#include "common.h"
#include "md5.h"
#include "ftdio.h"

static char *who = NULL;
static char *optstring = ":TRdg:F:lcmhsvVb:q";
static char  paths[MAXLG][32];
static char version[512];

static int nthreads = 0;
static int verbose = 0;
static int debug = 0;
static int show_progress = 1;
static size_t blocksize = 1024;
static long long max_blocks = -1;
static unsigned short chunk_size = 4;
static int show_all = 0;
static const char *flist = NULL;
static int tdmf_vols = 0;
static int repl_vols = 0;

static const char *driver = QNM;
static int master = -1;
static int NumGroups = 0;
static int NumLgArgs = 0;
static int vol_group = -1;
static int lgnum_list[MAXLG];

typedef int (*cmp_t)(const char *, const char *);
typedef int (*thread_proc_t)(const int pr_fd,
							 const int sec_fd,
							 const size_t blksize,
							 const off_t offset,
							 const u_longlong_t count);

static int compare_range(const int prim_fd,
						 const int sec_fd,
						 const size_t blksize,
						 off_t offset,
						 u_longlong_t count);
static int open_vol(const char *, struct statvfs *, struct stat *);
static int cksum_compare(const char *, const char *);
static int attr_compare(const char *, const char *);
static int data_compare(const char *, const char *);
static void progress(unsigned long long, unsigned long long, unsigned short);

const char *compare_mode = "data";
cmp_t compare = data_compare;

struct args {
	const int pr_fd;
	const int sec_fd;
	const size_t blksize;
	const off_t offset;
	const u_longlong_t count;
};

static pid_t
thread_start(thread_proc_t func, struct args *tap)
{
	pid_t child = fork();
	if (!child) {
		int ret = func(tap->pr_fd,
					   tap->sec_fd,
					   tap->blksize,
					   tap->offset,
					   tap->count);
		exit(ret);
	}
	return child;
}

static int indent_printf(const int indent, const char *fmt, ...)
{
	va_list ap;
	int i;
   int ret;
   
	for (i = 0; i < indent; i++)
		printf("  ");
	va_start(ap, fmt);

   ret = vprintf(fmt, ap);
  
   va_end(ap);   
   return ret;
}

static void
display(const int indent, const char *label, const char *fname, struct statvfs *vfs)
{
	#define STAT_DEC(x) printf("%-12.12s %ld\n", "st_" #x, s.st_##x)
	#define STAT_UINT(x) printf("%-12.12s %lu\n", "st_" #x, s.st_##x)
	#define STAT_OCT(x) printf("%-12.12s %l#O\n", "st_" #x, s.st_##x)
	#define STAT_HEX(x) printf("%-12.12s %lx\n", "st_" #x, s.st_##x)
	#define STAT_DEV(x) printf("%-12.12s %#16.16llx\n", "st_" #x, s.st_##x)
	#define STAT_SIZE(x) printf("%-12.12s %lu\n", "st_" #x, s.st_##x)
	#define STATVFS_STR(x) printf("%-12.12s %s\n", "f_" #x, vfs->f_##x)
	#define STATVFS_DEC(x) printf("%-12.12s %ld\n", "f_" #x, vfs->f_##x)
	#define STATVFS_UINT(x) printf("%-12.12s %lu\n", "f_" #x, vfs->f_##x)
	#define STATVFS_OCT(x) printf("%-12.12s %l#O\n", "f_" #x, vfs->f_##x)
	#define STATVFS_HEX(x) printf("%-12.12s %lx\n", "f_" #x, vfs->f_##x)
	#define STATVFS_DEV(x) printf("%-12.12s %#16.16llx\n", "f_" #x, vfs->f_##x)
	#define STATVFS_SIZE(x) printf("%-12.12s %lu\n", "f_" #x, vfs->f_##x)

	indent_printf(indent, "%s volume %s\n", label, fname);
	indent_printf(indent + 1, "blocksize %lu\n", vfs->f_bsize);
	indent_printf(indent + 1, "frag size %lu\n", vfs->f_frsize);
	indent_printf(indent + 1, "blocks (frags) %llu\n", (unsigned long long)vfs->f_blocks);
	if (debug) {
		indent_printf(indent + 1, "I/O blocksize %lu\n",
			(unsigned long)(vfs->f_frsize * chunk_size));
	}
}

static void
diff(const off_t blkloc,
	 const size_t blksize,
	 char *a,
	 char *b,
	 const off_t blkoff,
	 const size_t incsize)
{
	int i;
	for (i = 0; i < incsize; i++)
		if (*a++ != *b++)
			break;
    progress(blkloc, blkloc, 0);
	indent_printf(3, "difference in block %llu; byte offset %llu; block offset %lu\n",
				  (unsigned long long)((blkloc + blksize - 1) / blksize),
				  (unsigned long long)(blkloc + blkoff + i),
				  (blkloc + blkoff + i) % blocksize);
}

static void
progress(unsigned long long blk, unsigned long long blocks, unsigned short pct)
{
    if (!show_progress)
        return;
	if (blk < blocks) {
		u_longlong_t incr = (blocks * pct) / 100;
		if (!(blk % incr)) {
			printf("\r%2d%c complete", (int)(blk / incr), '%');
		}
	} else
		printf("\r%30.30s\r", " ");
	fflush(stdout);
}

static int
readblk(const int fd, off_t loc, const size_t blksize, void *buf)
{
	int result = 0;
	int res, n;
	if (!buf) return -1;
	res = blksize;
	while (res > 0) {
		n = pread64(fd, &((char *)(buf))[blksize - res], res, loc);
		if (n < 0) {
			result = -1;
			break;
		} else if (n == 0) {
			result = res;
			break;
		}
		res -= n;
		loc += n;
	}
	return result;
}

static int
md5_sum(const int fd, MD5_CTX *ctx, off_t loc, unsigned char *buf, const size_t len)
{
	int result = readblk(fd, loc, len, buf);
	if (!result)
		MD5Update(ctx, buf, len);
	else if (result > 0)
		MD5Update(ctx, buf, len - result);

	return result;
}

static char *stoh(const unsigned char *s)
{
	static char buf[DIGESTSIZE * 2 + 1];
	int i;
	char *p = buf;
	for (i = 0; i < DIGESTSIZE; i++)
		p += sprintf(p, "%2.2x", s[i]);
	*p = '\0';
	return buf;
}

static int
cksum_compare(const char *primary, const char *secondary)
{
	union sum_un {
		unsigned int chunk[1];
		unsigned char sum[DIGESTSIZE];
	} prim, sec;
	MD5_CTX primary_ctx;
	MD5_CTX secondary_ctx;
	u_longlong_t i, step, step_incr = 1000;
	struct statvfs pr_vfs, se_vfs;
	struct stat pr_stat, se_stat;
	int primary_fd = -1;
	int secondary_fd = -1;
	off_t loc = 0;
	unsigned char *data = NULL;
	u_longlong_t blkcnt;
	int result = attr_compare(primary, secondary);

	if (result)
		goto EXIT;
	primary_fd = open_vol(primary, &pr_vfs, &pr_stat);
	if (primary_fd < 0) {
		result = 1;
		goto EXIT;
	}
	secondary_fd = open_vol(secondary, &se_vfs, &se_stat);
	if (secondary_fd < 0) {
		result = 1;
		goto EXIT;
	}

	blkcnt = max_blocks < 0 ? pr_stat.st_size / blocksize : max_blocks;
	data = malloc(blocksize);
	if (!data) {
		result = -1;
		goto EXIT;
	}
	indent_printf(2, "Starting md5 checksum comparison\n");
	MD5Init(&primary_ctx);
	MD5Init(&secondary_ctx);
	for (i = 0; i < blkcnt; i++) {
		progress(i, blkcnt, 1);
		if (md5_sum(primary_fd, &primary_ctx, loc, data, blocksize) ||
			md5_sum(secondary_fd, &secondary_ctx, loc, data, blocksize)) {
			result = -1;
			break;
		}
		loc += blocksize;
	}
	progress(blkcnt, blkcnt, 1);
	if (!result) {
		MD5Final(prim.sum, &primary_ctx);
		MD5Final(sec.sum, &secondary_ctx);
		result = strncmp((char *)prim.sum, (char *)sec.sum, sizeof(prim.sum));
		indent_printf(2, "%s %s\n", primary, stoh(prim.sum));
		indent_printf(2, "%s %s\n", secondary, stoh(sec.sum));
	}
	if (!result)
		indent_printf(2, "Volume md5 checksums are identical\n");
EXIT:
	if (data)
		free(data);

	if (primary_fd >= 0)
		close(primary_fd);
	if (secondary_fd >= 0)
		close(secondary_fd);
	return result;
	return 0;
}

static int
attr_compare(const char *primary, const char *secondary)
{
	int result = 0;
	const char *p;
	struct statvfs pr_vfs, se_vfs;
    struct stat pr_stat, se_stat;
	if (stat(p = primary, &pr_stat) < 0 ||
		stat(p = secondary, &se_stat) < 0) {
		fprintf(stderr, "%s: %s(%s, %s): Unable to stat %s, error %s(%d)\n",
			who, __func__, primary, secondary,
            p, strerror(errno), errno);
		result = 1;
	}
    if (!(pr_stat.st_mode & S_IFBLK) || !(se_stat.st_mode & S_IFBLK))
        return 0; /* one or both devices is not a block device so skip
                   * the attribute check.
                   */
	if (statvfs(p = primary, &pr_vfs) < 0 ||
		statvfs(p = secondary, &se_vfs) < 0) {
		fprintf(stderr, "%s: %s(%s, %s): Unable to statvfs %s, error %s(%d)\n",
			who, __func__, primary, secondary,
            p, strerror(errno), errno);
		result = 1;
	}
	display(2, "primary", primary, &pr_vfs);
	display(2, "mirror", secondary, &se_vfs);
	result = pr_vfs.f_blocks != se_vfs.f_blocks ||
			 pr_vfs.f_bfree != se_vfs.f_bfree ||
			 pr_vfs.f_bavail != se_vfs.f_bavail ||
			 pr_vfs.f_files != se_vfs.f_files ||
			 pr_vfs.f_ffree != se_vfs.f_ffree ||
			 pr_vfs.f_favail != se_vfs.f_favail;
	return result;
}

static int
compare_range(const int prim_fd,
			  const int sec_fd,
			  const size_t blksize,
			  off_t offset,
			  u_longlong_t count)
{
	int result = 0;
	u_longlong_t *pr_data = NULL, *p, *sec_data = NULL, *s;
	char *end;
	u_longlong_t i;
	long pr_len, sec_len;
	off_t loc = offset;
    off_t nextblk;
	pr_data = (u_longlong_t *)malloc(blksize);
	if (!pr_data) {
		result = -1;
		goto EXIT;
	}
	sec_data = (u_longlong_t *)malloc(blksize);
	if (!sec_data) {
		result = -1;
		goto EXIT;
	}
	for (i = 0; i < count; i++) {
		progress(i, count, 1);
		if ((pr_len = readblk(prim_fd, loc, blksize, pr_data) < 0) ||
			(sec_len = readblk(sec_fd, loc, blksize, sec_data) < 0) ||
			(sec_len != pr_len)) {
			result = -1;
			break;
		}
		end = &((char *)(pr_data))[blksize];
		p = pr_data;
		s = sec_data;
		while ((char *)&p[1] < end) {
			if (*p++ != *s++) {
				diff(loc,
					 blksize,
					 (char *)&p[-1],
                     (char *)&s[-1],
					 (&p[-1] - pr_data) * sizeof(*p),
					 sizeof (*p));
				result = 1;
                if (!show_all)
                    break;
                nextblk = ((blocksize - 1 + ((char *)p) - ((char *)pr_data) - 1) / blocksize) * blocksize;
                (char *)p = ((char *)pr_data) + nextblk;
                (char *)s = ((char *)sec_data) + nextblk;
				break;
			}
		}
		if (result && !show_all) break;
		if ((char *)p < end && strncmp((char *)p, (char *)s, end - (char *)p)) {
			diff(loc,
				 blksize,
				 (char *)p, (char *)s,
				 (p - pr_data) * sizeof(*p),
				 end - (char *)p);
			result = 1;
			if (!show_all) break;
		}
		loc += blksize;
	}
	progress(count, count, 1);
EXIT:
	if (pr_data)
		free(pr_data);
	if (sec_data)
		free(sec_data);

	return result;
}

static int
open_vol(const char *volname, struct statvfs *vfs, struct stat *st)
{
	int fd = open64(volname, O_RDONLY|O_LARGEFILE, 0);
	if (fd < 0) {
		fprintf(stderr, "%s: %s(%s, ..): open64(%s, ..) failed with %s(%d)\n",
			who, __func__, volname, volname, get_error_str(errno), errno);
		return -1;
	}
	if (vfs && fstatvfs(fd, vfs) < 0) {
		fprintf(stderr, "%s: %s(%s, ..): fstatvfs(%d, ..) failed with %s(%d)\n",
			who, __func__, volname, fd, get_error_str(errno), errno);
		close(fd);
		return -1;
	}
	if (st && fstat(fd, st) < 0) {
		fprintf(stderr, "%s: %s(%s, ..): fstat(%d, ..) failed with %s(%d)\n",
			who, __func__, volname, fd, get_error_str(errno), errno);
		close(fd);
		return -1;
	}
	return fd;
}

static int
data_compare(const char *primary, const char *secondary)
{
	struct statvfs pr_vfs, se_vfs;
	struct stat pr_stat, se_stat;
	int primary_fd = -1;
	int secondary_fd = -1;
	u_longlong_t blkcnt;
	int result = attr_compare(primary, secondary);

	if (result)
		goto EXIT;
	primary_fd = open_vol(primary, &pr_vfs, &pr_stat);
	if (primary_fd < 0) {
		result = 1;
		goto EXIT;
	}
	secondary_fd = open_vol(secondary, &se_vfs, &se_stat);
	if (secondary_fd < 0) {
		result = 1;
		goto EXIT;
	}

	blkcnt = max_blocks < 0 ? pr_stat.st_size / blocksize : max_blocks;
    if (verbose)
        indent_printf(2, "Starting block-by-block comparison\n");
	result = compare_range(primary_fd,
						   secondary_fd,
						   blocksize,
						   0,
						   blkcnt);
	if (!result)
		indent_printf(2, "Volumes are identical\n");
EXIT:
	if (primary_fd >= 0)
		close(primary_fd);
	if (secondary_fd >= 0)
		close(secondary_fd);
	return result;
}

static int
process_config(char *cfgname)
{
	int result = 0;
	int i;
	group_t *group;
	sddisk_t *disk;
	indent_printf(0, "Config file %s\n", cfgname);
	if (!readconfig(1, 0, 0, cfgname)) {
		group = sys[0].group;
		disk = group->headsddisk;
		for (i = 0; i < group->numsddisks; i++) {
			indent_printf(1, "dtc device %s\n", disk->sddevname);
			if (compare(disk->devname, disk->mirname)) {
				indent_printf(2, "Volume %s does not match mirror volume %s\n",
					disk->devname, disk->mirname);
			}
			disk = disk->n;
		}
	} else {
		fprintf(stderr, "%s: error reading config file %s\n", who, cfgname);
		result = 1;
	}
	return result;
}

static int
init_vols(const char *qnm)
{
	int result = 0;
	char buf[MAXPATHLEN];
	sprintf(buf, "/dev/%s/ctl", qnm);
	master = open64(buf, O_RDWR);
	if (master < 0) {
        fprintf(stderr,"Error '%s(%d)': Failed to open "
						PRODUCTNAME
						" master device: %s.  \nHas %s dtcstart been run?\n",
						strerror(errno), errno, buf, qnm);
		result = -1;
	}
	NumGroups = GETCONFIGS(paths, 1, 1);
	if (NumGroups < 1) {
		result = -1;
	}
	return result;
}

static void
usage(const char *who, const int code)
{
    fprintf(stderr,"Usage: %s <options>\n\n", who);
    fprintf(stderr,"\
    One of the following three options is mandatory:\n\
        -F <files>  : Compare Replicator primary/secondary pairs in <files>\n\
        -F -        : Compare Replicator primary/secondary pairs in <stdin>\n\
        -R          : Compare configured (pNNN.cur) Replicator primary/secondary pairs\n\
        -T          : Compare configured TDMF primary/secondary pairs\n\
        -g <group#> : Restrict comparison to logical <group#>\n\
        -l          : Compare byte-by-byte for all blocks\n\
        -m          : Compare attributes\n\
        -c          : Compare md5 sums\n\
        -b          : volume blocksize, default = 1024\n\
        -s          : Show all data mis-compares\n\
        -sf         : Show first data mis-compare per block\n\
        -v          : Display version info\n\
        -h          : Print this listing\n");
    exit(code);
}

static void
check_env(void)
{
	char *p;
	p = getenv("_MAX_BLOCKS");
	if (p) {
		max_blocks = atoi(p);
	}
	p = getenv("_CHUNK_SIZE");
	if (p) {
		chunk_size = atoi(p);
	}
}

int
main(const int argc, char *const argv[])
{
    char			buf[MAXPATHLEN];
    char			ps_name[MAXPATHLEN];
    char			group_name[MAXPATHLEN];
    stat_buffer_t	statbuf;
    disk_stats_t	DiskInfo;
    ftd_stat_t		Info;
    ps_group_info_t ginfo;
	int				result = 0;
	int				err = 0;
	int 			c;
	int 			i, j;

	who = strrchr(argv[0], '/');
	if (who)
		who++;
	else
		who = argv[0];

    /* Make sure we are root */
    if (geteuid()) {
        fprintf(stderr, "Error: You must be root to run this program...aborted\n");
        exit(1);
    }

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }

    if (argc < 2) {
        usage(who, 1);
    }
    
    sprintf (version,"%s  Version %s%s", PRODUCTNAME, VERSION, VERSIONBUILD );

    opterr = 0;
	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'T':
			tdmf_vols = 1;
			break;
		case 'R':
			repl_vols = 1;
			break;
		case 'F':
			flist = optarg;
			break;
		case 'g':
            if (NumLgArgs >= 0 && isdigit(optarg[0])) {
                vol_group = ftd_strtol(optarg);
                if (vol_group < 0 || vol_group >= MAXLG) {
                    fprintf(stderr, "ERROR: [%s] is invalid number for replication group\n", optarg);
                    result = 1;
                    err = 1;
                }
                lgnum_list[NumLgArgs++] = vol_group;
            } else if (optarg[0] == 'a')
                NumLgArgs = -1;
            else {
                fprintf(stderr, "Error: '%c' invalid argument for -g option\n", *optarg);
                err = 1;
            }
			break;
        case 'b':
            blocksize = atoi(optarg);
            break;
		case 'c':
			compare = cksum_compare;
			compare_mode = "MD5 sums";
			break;
		case 'l':
			compare = attr_compare;
			compare_mode = "attributes";
			break;
		case 'm':
			compare = data_compare;
			compare_mode = "data";
			break;
		case 's':
            show_all = 1;
			break;
		case 't':
			nthreads = ftd_strtol(optarg);
			break;
        case 'q':
            show_progress = 0;
            break;
		case '?':
			/* unrecognized arg */
            fprintf(stderr, "Error: '%c' - unrecognized option character\n", optopt);
            err = 1;
			break;
		case ':':
			/* missing arg */
            fprintf(stderr, "Error: '%c' requires an argument\n", optopt);
            err = 1;
			break;
		case 'v':
			printf("%s\n", version);
			exit(0);
		case 'V':
			verbose = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 'h':
		default:
			usage(who, 0);
			break;
		}
	}

    if (optind != argc || err) {
        fprintf(stderr, "Error: Invalid arguments\n");
        usage(who, 1);
    }
	if ((tdmf_vols && repl_vols) ||
		(tdmf_vols && flist) ||
		(repl_vols && flist)) {
		fprintf(stderr, "only one of '-R', '-T', or '-F' may be specified\n");
		result = 1;
	}

	check_env();
	if (flist) {
		FILE *name = NULL;
		char source[MAXPATH + 1];
		char mirror[MAXPATH + 1];
		if (strcmp(flist, "-"))
			name = fopen(flist, "r");
		else
			name = stdin;
		if (!name) {
			fprintf(stderr, "Unable to open %s: %s(%d)\n", flist, strerror(errno), errno);
			goto EXIT;
		}
		indent_printf(0, "Comparing source/mirror pairs in file %s\n",
			name == stdin ? "<stdin>" : flist);
		while (fscanf(name, "%s %s\n", source, mirror) == 2) {
			indent_printf(1, "Comparing %s of %s and %s\n", compare_mode, source, mirror);
			if (compare(source, mirror)) {
				indent_printf(2, "Volume %s does not match mirror volume %s\n",
					source, mirror);
			}
		}
		fclose(name);
	} else {
		init_vols(driver);
		initconfigs();
		for (i = 0; i < NumGroups; i++) {
			if (NumLgArgs < 0) {
				process_config(paths[i]);
			}
			for (j = 0; j < NumLgArgs; j++) {
				sprintf(buf, "p%03d.cur", lgnum_list[j]);
				if (!strncmp(paths[i], buf, strlen(buf))) {
					process_config(paths[i]);
				}
			}
		}
		if (master >= 0)
			close(master);
	}
EXIT:
	return result;
}
