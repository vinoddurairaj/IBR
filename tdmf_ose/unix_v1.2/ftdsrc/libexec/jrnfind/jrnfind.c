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
/***************************************************************************
 *
 * jrnfind.c
 *
 * (c) Copyright 2006 Softek Storage Solutions Inc. All Rights Reserved
 *
 *  $Id: jrnfind.c,v 1.2 2010/12/20 20:21:18 dkodjo Exp $
 *
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "network.h"
#include "common.h"

typedef const char * blk_data_t;
typedef unsigned long blkno_t;

typedef struct volume {
    const char *name;
    blk_data_t *blk_list;
} volume_t;

typedef struct match_s {
    off_t journal_offset;
    off_t volume_offset;
    const char *jrn_name;
    blk_data_t data;
    blkno_t blkno;
    time_t ts;
    int devid;
    volume_t **blk_matches;
} match_t;

typedef struct journal_file {
    const char *name;
    ino_t inode;
    char type;
    struct journal_file *link;
} journal_file_t;

typedef struct hash {
    const journal_file_t *entry;
    struct hash *next;
} hash_t;

int verbose = 0;
int print_header = 1;
int print_summary = 1;
int print_all = 0;
const char *who = NULL;

volume_t **vols = NULL;
int vol_count = 0;
int vol_slots = 0;

blkno_t *blks = NULL;
int blk_count = 0;
int blk_slots = 0;
size_t blksize = 1024;

int devices[1024];
int device_count = 0;
int header_count = 0;

const journal_file_t *jrn_list[4096];
const int max_journals = sizeof(jrn_list) / sizeof(*jrn_list);
int jrn_count = -1;

void
record(void *arg)
{
    headpack_t *header = arg;
    int i;
    header_count++;
    for (i = 0; i < device_count; i++)
        if (devices[i] == header->devid)
            return;
    devices[device_count++] = header->devid;
}

static int
cmp_devices(const void *a, const void *b)
{
    const int *ia = a, *ib = b;
    return *ia < *ib ? -1 : *ia == *ib ? 0 : 1;
}

void
playback(void)
{
    int i;
    if (device_count > 0) {
        printf("%d devices found:\n", device_count);
        qsort(devices, device_count, sizeof(devices[0]), cmp_devices);
        for (i = 0; i < device_count; i++)
            printf("    %4.4x[%d]\n", devices[i], devices[i]);
    }
}

static int
add_volume(const char *volname)
{
    const int fd = open(volname, O_RDONLY);
    volume_t *vol = NULL;;
    blk_data_t *blk_data = NULL;
    int result = 0;
    int err;
    int i;

    if (fd < 0) {
        err = errno;
        fprintf(stderr, "%s: open(%s, O_RDONLY) failed with error %s\n",
            who, volname, strerror(err));
        errno = err;
        return 0;
    }
    vol = (volume_t *)malloc(sizeof(*vol));
    if (!vol) {
        err = errno;
        fprintf(stderr, "%s: malloc(%u) failed with error %s\n",
            who, sizeof(*vol), strerror(err));
        errno = err;
        result = 0;
        goto EXIT;
    }
    vol->name = strdup(volname);
    if (blk_count > 0) {
        blk_data = (blk_data_t *)calloc(blk_count, sizeof(blk_data_t));
        blk_data_t blk;
        if (!blk_data) {
            err = errno;
            fprintf(stderr, "%s: calloc(%u, %u) failed with error %s\n",
                who, blk_count, sizeof(blk_data_t), strerror(err));
            errno = err;
            goto EXIT;
        }
        result = 1;
        for (i = 0; i < blk_count; i++) {
            blk = (blk_data_t)malloc(blksize);
            if (!blk) {
                err = errno;
                fprintf(stderr, "%s: malloc(%u) failed with error %s\n",
                    who, blksize, strerror(err));
                errno = err;
                result = 0;
            }
            if (pread(fd, (void *)blk, blksize, blks[i] * blksize) < blksize) {
                err = errno;
                fprintf(stderr, "%s: pread(%d, %p, %#x, %#llx) failed with error %s\n",
                    who, fd, blk, blksize,
                    (unsigned long long)(blksize * blks[i]),
                    strerror(err));
                result = 0;
                free((void *)blk);
            }
            blk_data[i] = blk;
        }
        vol->blk_list = blk_data;
    }
    if (vol_count >= vol_slots) {
        vol_slots += 10;
        vols = (volume_t **)realloc(vols, vol_slots * sizeof(volume_t *));
    }
    vols[vol_count++] = vol;

EXIT:
    close(fd);
    if (!result) {
        if (blk_data) {
            int i;
            for (i = 0; i < blk_count; i++)
                if (blk_data[i])
                    free((void *)blk_data[i]);
            free(blk_data);
        }
        if (vol) {
            if (vol->name)
                free((void *)vol->name);
            free(vol);
        }
    }
    return result;
}

static int
add_blk_by_blkno(const blkno_t blkno)
{
    int i;
    for (i = 0; i < blk_count; i++)
        if (blks[i] == blkno)
            return 1;
    if (blk_count >= blk_slots) {
        blk_slots += 10;
        blks = (blkno_t *)realloc((void *)blks, blk_slots * sizeof(blkno_t));
    }
    blks[blk_count++] = blkno;
    return 1;
}

static int
add_blk_by_offset(const off_t offset)
{
    return add_blk_by_blkno(offset / blksize);
}

void
usage(void)
{
    fprintf(stderr, "Usage: %s\n"
        "\t-g <logical group>\n"
        "\t{ -b <block to find> | -o <byte_offset> }\n"
        "\t[ -d <dev id> ]            # default = -1, match any\n"
        "\t[ -j <journal directory> ] # default = /journal\n"
        "\t[ -l <block size> ]        # default = 1024\n"
        "\t[ -p <name prefix> ]\n"
        "\t[ -t <type> ]              # any = a, coherent = c(default), incoherent = i\n"
        "\t[ -s ]                     # no summary\n"
        "\t[ -q ]                     # no report headings\n"
        "\t[ -a ]                     # print all block instances\n"
        "\t[ <match volume ]\n", who);
}

match_t **matches = NULL;
int match_count = 0;
int match_next = 0;

match_t *
new_match(const char *path)
{
    const int match_inc = 10;
    static const char *savepath = NULL;
    match_t *m = NULL;
    if (match_next >= match_count) {
        matches = (match_t **)realloc((void *)matches,
                                      (match_count + match_inc) * sizeof(match_t *));
        if (!matches) {
            fprintf(stderr, "%s: realloc(%p, %d) failed with error %s\n", who,
                matches, (match_count + match_inc) * sizeof(match_t *), strerror(errno));
        }
        match_count += match_inc;
    }
    m = (match_t *)malloc(sizeof(*m));
    if (!m) {
        fprintf(stderr, "%s: malloc(%d) failed with error %s\n", who, sizeof(*m), strerror(errno));
    }
    m->blk_matches = NULL;
    if (!savepath || strcmp(path, savepath))
        savepath = strdup(path);
    m->jrn_name = savepath;
    matches[match_next++] = m;
    return m;
}

int
match_size()
{
    return match_next;
}

int
cmp_match(const void *a, const void *b)
{
    const match_t *ma = *(match_t **)a;
    const match_t *mb = *(match_t **)b;
    if (ma->blkno == mb->blkno) {
        long int diff = ma->ts - mb->ts;
        return diff < 0 ? -1 : diff > 0 ? 1 : 0;
    }
    return ma->blkno < mb->blkno ? -1 : 1;
}

static const char *
csprintf(const int width, const char *text)
{
    static char buf[LINE_MAX];
    size_t len = strlen(text);
    size_t line_width = width > sizeof(buf) ? sizeof(buf) : width;
    if (len > line_width) {
        strncpy(buf, text, line_width - 1);
        buf[line_width] = '\0';
        return buf;
    }
    memset(buf, ' ', (line_width - len) / 2);
    strcpy(&buf[(line_width - len) / 2], text);
    return buf;
}

static char *
match_list(volume_t **vols)
{
    static char buf[32];
    char *p = buf;
    int i;
    *p = '\0';
    if (vols) {
        for (i = 0; i < vol_count; i++) {
            if (vols[i])
                p += sprintf(p, "[%d] ", i);
        }
    }
    return buf;
}

static void
print_matches(volume_t **vols, const int type, const int devid)
{
    char buf[PATH_MAX];
    char timestr[32];
    int N = match_size();
    char *p, *typestr = "coherent";
    match_t *m;
    int i;

    if (N) {
        qsort(matches, N, sizeof(match_t *), cmp_match);
        if (print_header) {
            if (type == 'a')
                typestr = "all";
            else if (type == 'i')
                typestr = "incoherent";
            printf("%s\n", csprintf(100, vols ?
                (snprintf(buf, sizeof(buf), "Block data instances"), buf) :
                (snprintf(buf, sizeof(buf), "%d block number occurrences in %s journals",
                          blk_count, typestr), buf)));
            printf("%-32s %-10s %-10s %-4s %-10s %-10s\n",
                "          Journal Name", "   File ", "  Chunk ", "Dev", "Timestamp", "  Blkno   ");
            printf("%-32s %-10s %-10s\n",
                " ",                      "  Offset", "  Offset");
        }
        for (i = 0; i < N; i++) {
            m = matches[i];
            if (vols && !m->blk_matches)
                continue;
            p = strrchr(ctime_r(&m->ts, timestr), '\n');
            if (p)
                *p = '\0';
            printf("%-32s 0x%8.8lx 0x%8.8lx %4.4x 0x%8.8lx %8lu%s\n",
                m->jrn_name,
                (unsigned long)m->journal_offset,
                (unsigned long)m->volume_offset,
                m->devid,
                m->ts,
                /* timestr, */
                m->blkno,
                match_list(m->blk_matches));
        }
    }
}

hash_t *inodes[127];
const unsigned hash_mod = sizeof(inodes) / sizeof(*inodes);
int hash_count = 0;

static const journal_file_t *
find_entry(const journal_file_t *entry)
{
    hash_t **p;
    int i;
    ino_t inode = entry->inode;

    if (hash_count < 1)
        for (i = 0; i < hash_mod; i++)
            inodes[i] = NULL;
    p = &inodes[inode % hash_mod];
    while (*p) {
        if ((*p)->entry->inode == inode)
            break;
        p = &((*p)->next);
    }
    if (!*p) {
        *p = (hash_t *)malloc(sizeof(**p));
        (*p)->next = NULL;
        (*p)->entry = entry;
        hash_count++;
    }
    return (*p)->entry;
}

int
get_journal_files(const char *jrn_path, const char *prefix, int group)
{
    DIR *dfd;
    struct dirent *dent;
    int lgnum, jnum;
    char ext;
    ino_t inode;
    journal_file_t *entry;
    const journal_file_t *p;
    const char *pattern = "%llu.j%3d.%3d.%1c";
    char format[NAME_MAX];

    if (!jrn_path)
        return -1;
    if ((dfd = opendir(jrn_path)) == NULL) {
        int err = errno;
        fprintf(stderr, "%s: opendir(%s) failed with error %s\n", who, jrn_path, strerror(err));
        errno = err;
        return -1;
    }
    snprintf(format, sizeof(format), "%s%s", prefix ? prefix : "", pattern);
    jrn_count = 0;
    while (NULL != (dent = readdir(dfd))) {
        if (sscanf(dent->d_name, format, &inode, &lgnum, &jnum, &ext) == 4 && lgnum == group) {
            if (verbose)
                printf("found journal: %s\n", dent->d_name);
            entry = (journal_file_t *)malloc(sizeof(*entry));
            entry->name = strdup(dent->d_name);
            entry->inode = dent->d_ino;
            entry->type = ext;
            entry->link = NULL;
            p = find_entry(entry);
            if (p == entry) {
                if (jrn_count < max_journals)
                    jrn_list[jrn_count++] = p;
                else {
                    fprintf(stderr, "%s: %s(%s, %s, %d): max_journals limit(%d) exceeded\n",
                        who, __func__, jrn_path, prefix, group, max_journals);
                    return 0;
                }
            }
            if (inode != entry->inode)
                fprintf(stderr, "%s: %s(%s, %s, %d): %s inode = %llu\n",
                    who, __func__, jrn_path, prefix, group, entry->name, entry->inode);
        }
    }
    (void)closedir(dfd);
    return jrn_count;
}

static const journal_file_t *
find_journal(const journal_file_t *jrn, const char type)
{
    const journal_file_t *j;
    if (type == 'a')
        return jrn;
    for (j = jrn; j->link; j = j->link)
        if (j->type == type && j->inode == jrn->inode)
            return j;
    return j->type == type ? j : NULL;
}

static volume_t **
search_vol(const int index, blk_data_t data)
{
    volume_t **vol_list = (volume_t **)calloc(vol_count, sizeof(volume_t *));
    int i, count = 0;
    for (i = 0; i < vol_count; i++)
        if (!memcmp(vols[i]->blk_list[index], data, blksize))
            (count++, vol_list[i] = vols[i]);
    if (count)
        return vol_list;
    free((void *)vol_list);
    return NULL;
}

static int
search_journal(char * path, int devid)
{
    int fd;
    jrnheader_t *jrnheader;
    headpack_t *data_header;
    char *offset, *eof;
    off_t jrn_size;
    struct stat jrn_stat;
    void *jrn_origin;
    int count = 0;
    int err = 0;
    int hdr_count = 0;

    if (verbose)
        fprintf(stderr, "%s: search_journal(%s, <%d:%d>)\n",
            who, path, (devid >> 8) & 0xff, devid & 0xff);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        err = errno;
        fprintf(stderr, "%s: open(%s) failed with error %s\n", who, path, strerror(err));
        errno = err;
        return -1;
    }
    if (fstat(fd, &jrn_stat) < 0) {
        err = errno;
        fprintf(stderr, "%s: stat(%s, ..) failed %s\n", who, path, strerror(err));
        errno = err;
        return -1;
    }
    jrn_size = jrn_stat.st_size;
    jrn_origin = mmap(NULL, jrn_size, PROT_READ, MAP_SHARED, fd, 0);
    if (!jrn_origin) {
        err = errno;
        fprintf(stderr, "%s: mmap(NULL, %llu, PROT_READ, MAP_SHARED, %s, 0) failed with error %s",
            who, jrn_size, path, strerror(err));
        close(fd);
        errno = err;
        return -1;
    }

    offset = (char *)jrn_origin;
    eof = offset + jrn_size;

    jrnheader = (jrnheader_t *)offset;
    if (jrnheader->magicnum != MAGICJRN) {
        err = errno;
        fprintf (stderr, "%s: bad journal file magic number %#lx\n", who, jrnheader->magicnum);
        goto EXIT;
    }

    offset += sizeof(*jrnheader);
    data_header = (headpack_t *)offset;
    offset += sizeof(*data_header);

    while ((offset + data_header->len) < eof) {
        hdr_count++;
        if (data_header->magicvalue != MAGICHDR) {
            fprintf(stderr, "%s: bad header magic number; file %s offset %#lx count %d\n",
                who, path,
                (unsigned long)(((void *)data_header) - ((void *)jrn_origin)), hdr_count);
            goto EXIT;
        }
        record(data_header);
        if (devid < 0 || devid == data_header->devid) {
            off_t boffset = data_header->offset << DEV_BSHIFT;
            int i;
            for (i = 0; i < blk_count; i++) {
                off_t toffset = blks[i] * blksize;
                if (((unsigned long)(toffset - boffset)) < (unsigned long)data_header->len) {
                    match_t *m = new_match(path);
                    blk_data_t data = ((blk_data_t)(offset)) + (toffset - boffset);
                    m->volume_offset = boffset;
                    m->journal_offset = (off_t)((void *)data - (void *)jrn_origin);
                    m->ts = data_header->ts;
                    m->blk_matches = search_vol(i, data);
                    m->blkno = blks[i];
                    m->devid = data_header->devid;
                    count++;
                }
            }
        }
        offset += data_header->len;
        data_header = (headpack_t *)offset;
        offset += sizeof(*data_header);
    }
EXIT:
    munmap(jrn_origin, jrn_size);
    close(fd);
    errno = err;
    return count;
}
        
int
main (const int argc, char *const argv[])
{
    int devid = -1;
    int lgnum = -1;
    const char *jrndir = "/journal";
    char fullpath[PATH_MAX];
    char *jrnprefix = "";
    char type = 'c';
    int hits, file_hits, files_read;
    int i, c, n;
    unsigned long long size;
    int errors = 0;

    const char *options = ":aj:g:d:vho:p:b:l:t:sq";

    who = strrchr(argv[0], '/');
    if (who)
        who++;
    else
        who = argv[0];

    opterr = 0;
    while ((c = getopt(argc, argv, options)) != -1) {
        switch (c) {
        case 'a':
            print_all = 1;
            break;
        case 's':
            print_summary = 0;
            break;
        case 'q':
            print_header = 0;
            break;
        case 'b':
            if (!add_blk_by_blkno(atol(optarg)))
                errors++;
            break;
        case 'p':
            jrnprefix = optarg;
            break;
        case 'j':
            jrndir = (const char *)optarg;
            break;
        case 'o':
            if (!add_blk_by_offset(atoll(optarg)))
                errors++;
            break;
        case 'g':
            lgnum = atoi(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            usage();
            return 0;
        case 'l':
            n = strlen(optarg);
            size = atoll(optarg);
            switch (tolower(optarg[n])) {
            case 'k':
                size *= (1 << 10);
                break;
            case 'm':
                size *= (1 << 20);
                break;
            case 'g':
                size *= (1 << 30);
                break;
            }
            break;
        case 'd':
            if (sscanf(optarg, "%i", &devid) != 1) {
                fprintf(stderr, "%s: unable to convert '%s' to device id\n", who, optarg);
                errors++;
            }
            break;
        case 't':
            type = *optarg;
            switch (type) {
            case 'c':
            case 'i':
            case 'a':
                break;
            default:
                fprintf(stderr, "%s: -t %c, value must be 'a', 'c', or 'i'\n", who, type);
            }
            break;
        case ':':
            fprintf(stderr, "%s: missing argument for %c-option\n", who, optopt);
            if (0)
        case '?':
            fprintf(stderr, "%s: unrecognized option %c\n", who, optopt);
            errors++;
            usage();
            return 1;
        }
    }
    if (errors) {
        usage();
        return 1;
    }
    if (blk_count <= 0) {
        fprintf(stderr, "%s: no block numbers specified\n", who);
        errors++;
        return 1;
    }

    while (optind < argc) {
        if (!add_volume(argv[optind])) {
            fprintf(stderr, "%s: add_volume(%s) failed\n",
                who, argv[optind]);
        }
        optind++;
    }

    if ((get_journal_files(jrndir, jrnprefix, lgnum)) <= 0) {
        fprintf(stderr, "%s: get_journal_files(%s, %s, %d) returned 0\n",
            who, jrndir, jrnprefix, lgnum);
    }

    for (i = 0, hits = 0, file_hits = 0, files_read = 0; i < jrn_count; i++ ) {
        const journal_file_t *jrn = find_journal(jrn_list[i], type);
        int n;
        if (!jrn)
            continue;
        files_read++;
        snprintf(fullpath, sizeof(fullpath), "%s/%s", jrndir, jrn->name);
        n = search_journal(fullpath, devid);
        if (n > 0) {
            hits += n;
            file_hits++;
        }
    }
    if (hits) {
        if (vol_count && print_all)
            print_matches(NULL, type, devid);
        print_matches(vols, type, devid);
        for (i = 0; i < vol_count; i++)
            printf("[%d] %s\n", i, vols[i]->name);
    }
    if (print_summary) {
        printf("Found %d occurrence%s of ", hits, hits != 1 ? "s" : "");
        if (devid >= 0)
            printf("device <%x:%x>, %i blocks\n",
                 (devid >> 8) & 0xff, devid & 0xff, blk_count);
        else
            printf("%i blocks\n", blk_count);
        printf("%d journals found, %d journals read, %d journals matched\n",
            jrn_count, files_read, file_hits);
        printf("%d headers read\n", header_count);
        playback();
    }
    return hits > 0 ? 0 : 1;
}

