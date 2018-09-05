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
/*-
 * pacewrite.c - Performance Measurement Tool
 *
 * Copyright (c) 2004 Softek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/vfs.h>
#if defined (linux)
#include <linux/times.h>
#else
#include <sys/times.h>
#endif

extern long int atol();
extern int  optind;
extern int  optopt;
extern char *optarg;
extern int  errno;
extern int  sys_nerr;

int verbose = 0;
int debug = 0;
int bdsk = 0;
int Bflg = 0;
int qflg = 0;
int wrapAround = 0;
int asyncFlag = 0;

#define LONG                    long
#define KB                      1024
#define MB                      (1024 * 1024)
#define LTRESO                  100     /* calling times() per while loop */

#define ERR                     0x8000
#define EOK                     0
#define ERROR                   -1
#define ERR_OK                  EOK
#define ERR_SYSCALL             0x8001
#define ERR_INVALID             0x8002
#define ERR_DEBUG               0x0001
#define ERR_MSG                 0x0002
#define ERR_MSG2                0x0004

int debugFlag = ERR | ERR_MSG;

#if defined (linux)
#define TRACE_APPLY_FUNC(_ec, _fn, _mod, _rest...)  \
    {                                               \
        if (verbose || ((_ec) & debugFlag))         \
        {                                           \
            if ((_ec) & ERR)                        \
            {                                       \
                _fn (_mod, "ERROR(%d): ", (_ec));   \
            } else {                                \
                _fn (_mod, "trace(%d): ", (_ec));   \
            }                                       \
            _fn (_mod, ##_rest);                    \
        }                                           \
    }
#endif

#if defined (linux)
#define logMsg(errCode, fmt, trailingArgs...)   \
    TRACE_APPLY_FUNC(errCode, fprintf, stderr, fmt, ##trailingArgs)
#endif

#define ERRMSG(errCode, msg)    errMsg(__FUNCTION__, (errCode), (msg))

#define TICK_PER_SEC            100
#define MAX_PATTERN_SIZE        256
#define MAX_FILENAME_SIZE       256
#define MAX_ERRMSG_SIZE         128
#define DEFAULT_STACK_SIZE      4096
#define NOLIMIT                 -1

#define DEFAULT_PATTERN         "abcdefghijklmnopqrstuvwxyz"
#define DEFAULT_WRBS            1024         
#define DEFAULT_OFFSET          2048    /* to avoid mess up partition table */
#define DEFAULT_CLONE           1         

#if defined (SOLARIS) || defined (HPUX)
#define __FUNCTION__ "func"
#endif

#if !defined (linux)
int logMsg (int errCode, char *fmt, ...);

int
logMsg
(
    int     errCode,
    char    *fmt,
    ...
)
{
    va_list args;
    int     d;
    char    c;
    char    *p;
    char    *s;

    if (verbose || (errCode & debugFlag)) {
        if (errCode & ERR) {
            fprintf(stderr, "ERROR(%d): ", errCode);
        } else {
            fprintf(stderr, "trace(%d): ", errCode);
        }

        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }

    return EOK;
}
#endif
    
void
usage
(
    void
)
{
#ifndef VERIFY_ONLY
    fprintf(stderr, "usage: pacewrite -d device | -f file "
                    "[-a async write mode] "
                    "[-b bandwidth] "
                    "[-c threads] "
                    "[-D debugFlag] "
                    "[-l writeCnt] "
                    "[-p pattern] "
                    "[-q verify after write] "
                    "[-s output_file/dev_size] "
                    "[-S initial byte skip] "
                    "[-t test_time(minute)] "
                    "[-T test_time(hours)] "
                    "[-w write_block_size] "
                    "\n");
    fprintf(stderr, "\t-a: Sync write mode, default is sync write mode\n");
    fprintf(stderr, "\t-b: KB per second, default is no limit\n");
    fprintf(stderr, "\t-B: MB per second, default is no limit\n");
    fprintf(stderr, "\t-c: Number of forked processes, default is %d\n", 
                           DEFAULT_CLONE);
    fprintf(stderr, "\t-d: Block disk, input required\n");
    fprintf(stderr, "\t-D: Turn on particular debug flag\n");
    fprintf(stderr, "\t-f: File, input required\n");
    fprintf(stderr, "\t-l: Fast writing mode like using dd\n");
    fprintf(stderr, "\t-p: Pattern to write, max size is %d, default is \"%s\""
                           "\n", MAX_PATTERN_SIZE, DEFAULT_PATTERN);
    fprintf(stderr, "\t-q: Verify correctness after write test is done\n");
    fprintf(stderr, "\t-s: Max file/dev size for write test in bytes, default "
                           " is no limit. That is, use up the available space"
                           "\n");
    fprintf(stderr, "\t-S: Skip the initial given byte for writing data, "
                          "useful for preserve disk partition table. Default "
                          "is %d\n", DEFAULT_OFFSET);
    fprintf(stderr, "\t-t: Test time in minutes, default is forever\n");
    fprintf(stderr, "\t-T: Test time in hours, default is forever\n");
    fprintf(stderr, "\t-w: Block size per write, default is %d byte\n",
                           DEFAULT_WRBS);
#else /* VERIFY_ONLY */
    fprintf(stderr, "usage: chkwrite -d device | -f file "
                    "[-p pattern] "
                    "[-s output_file/dev_size] "
                    "[-S initial byte skip] "
                    "[-w write_block_size] "
                    "\n");
    fprintf(stderr, "\t-p: Pattern to write, max size is %d, default is \"%s\""
                           "\n", MAX_PATTERN_SIZE, DEFAULT_PATTERN);
    fprintf(stderr, "\t-s: Max file/dev size for write test in bytes, default "
                           " is no limit. That is, use up the available space"
                           "\n");
    fprintf(stderr, "\t-S: Skip the initial given byte for writing data, "
                          "useful for preserve disk partition table. Default "
                          "is %d\n", DEFAULT_OFFSET);
    fprintf(stderr, "\t-w: Block size per write, default is %d byte\n",
                           DEFAULT_WRBS);
#endif
}

void
errMsg
(
    char    *func,
    int     errCode,
    char    *msg
)
{
    switch (errCode) {
    case ERR_DEBUG:
        fprintf(stderr, "%s: %s\n", func, msg);
        break;

    case ERR_SYSCALL:
        /*
        fprintf(stderr, "ERROR: %s: %s ", func, msg);
        if ((errno > 0) && (errno < sys_nerr)) {
            fprintf(stderr, "%s \n", strerror(errno));
        } else {
            fprintf(stderr, "\n");
        }
        break;
        */

    default:
        fprintf(stderr, "ERROR(%d): %s: %s\n", errCode, func, msg);
        break;
    }
}
 
void
test_time
(
    int rounds
)
{
    time_t      startTime;
    time_t      tloc;
    struct tms  ttloc;
    clock_t     ts;
    time_t      ltl = 0;
    clock_t     htl = 0;
    int         i = 0;
    int         il = 0;
    int         ih = 0;

    if (rounds == 0) {
        rounds = 10;
    }

    do {
        ts = times(&ttloc);
        startTime = time(&tloc);

        if (ltl != startTime) {
            ltl = startTime;
            il++;
            logMsg(ERR_DEBUG, "high/low time diff: %d\n", ih - il);
            il = ih = 0;
        }

        if (htl != ts) {
            htl = ts;
            ih++;
        }

        i++;
    } while (i < rounds);
}
            
int
pw_verify
(
    int     ofd,
    long    initOffset,
    long    outputSize,
    long    blockSize,
    char    *pattern
)
{
    char    *refbuf;
    char    *rdbuf;
    ssize_t nbyte;

    logMsg(ERR_MSG,
            "%s: output fid: %d, initOffset: %ld, output size: %ld, "
            "blockSize: %ld, pattern: \"%s\" \n",
            __FUNCTION__, ofd, initOffset, outputSize, blockSize, pattern);

#ifndef VERIFY_ONLY
    logMsg(ERR_MSG, "Start verify...\n");
#endif

    if (pw_fillRefBuf(&refbuf, pattern, blockSize) != EOK) {
        logMsg(ERROR, "buffer preparation failed\n");
        return ERROR;
    }

    if ((rdbuf = malloc(blockSize)) == NULL) {
        logMsg(ERROR, "No memory\n");
        return ERROR;
    }

    if (lseek(ofd, initOffset, SEEK_SET) == -1) {
        logMsg(ERROR, "lseek() error, fd: %ld offset: %ld\n", ofd, initOffset);
        return ERROR;
    } else {
        logMsg(ERR_DEBUG, "seek to offset %ld\n", initOffset);
    }

    do {
        bzero(rdbuf, blockSize);
        if ((nbyte = read(ofd, rdbuf, (size_t)blockSize)) == -1) {
            logMsg(ERROR, "read() error\n");
            return ERROR;
        } else {
            if (nbyte == 0) {
                logMsg(ERR_MSG, "verify ok.\n");
                return EOK;
            } else if (nbyte < blockSize) {
                if (strncmp(refbuf, rdbuf, (nbyte - 1))) {
                    return ERROR;
                }
            } else {
                if (strncmp(refbuf, rdbuf, nbyte)) {
                    return ERROR;
                }
            }
        }
    } while (1);

    return EOK;
}

ssize_t
pw_write
(
    int         ofd,
    const void  *buf,
    size_t      blockSize
)
{
    ssize_t     rc;

    if (!debug) {
        if ((rc = write(ofd, buf, blockSize)) == -1) {
            /*
             * a work around for not checking device/file max allowence size
             */
            if (wrapAround) {
                return -1;
            } else {
                if (lseek(ofd, 0L, SEEK_SET) == -1) {
                    return -1;
                } else {
                    wrapAround = 1;
                    logMsg(ERR_MSG2, "File wrap around\n");
                    return blockSize;
                }
            }
        } else {
            wrapAround = 0;
            return rc;
        }
    } else {
        return blockSize;
    }
}

int
pw_fillRefBuf
(
    char    **refBuf,
    char    *pattern,
    int     blockSize
)
{
    int     patLen;
    long    leftover = blockSize;
    char    *bufOffset;
    char    *buf = NULL;

    *refBuf = buf;

    if ((buf = malloc(blockSize)) == NULL) {
        logMsg(ERROR, "No memory\n");
        return ERROR;
    } else {
        *refBuf = buf;
        bufOffset = buf;
        bzero((void *)buf, blockSize);
        patLen = strlen(pattern);
        if (patLen >= blockSize) {
            bcopy((void *)pattern, (void *)buf, blockSize);
        } else {
            do {
                if (leftover >= patLen) {
                    bcopy((void *)pattern, (void *)bufOffset, patLen);
                    leftover -= patLen;
                    bufOffset += patLen;
                } else {
                    bcopy((void *)pattern, (void *)bufOffset, leftover);
                    break;
                }
            } while (1);
        }
        logMsg(ERR_DEBUG, "%s\n", *refBuf);
    }

    return EOK;
}

int
paceWrite
(
    int     ofd,
    LONG    bw,
    long    outputSize,
    char    *pattern,
    long    testTime,
    long    blockSize,
    long    writeCnt
)
{
    int         stop = 0;
    time_t      startTime;
    time_t      lastTime;
    time_t      endTime = 0;
    time_t      tloc;
    clock_t     hrtime;
    clock_t     lastHrtime = 0;
    struct tms  hrtloc;
    char        tm[30];
    long        byteWrote = 0;
    char        *buf = NULL;
    int         rc = ERR_OK;
    int         i;
    int         idleTick = 0;
    long        fileOffset = 0;
    long        totalBlock = 0;
    int         chktime = 0;
    long        loop = 0;
    int         bs = 0;
    int         r = 0, m1 = 0, R = 0, m2 = 0;
    LONG        BW = bw;
    
    logMsg(ERR_DEBUG,
            "%s: output fid: %d, BW: %ld%s, output size: %ld, "
            "pattern: \"%s\", test time: %ld, blockSize: %ld\n",
            __FUNCTION__, ofd, bw, (Bflg == 0) ? "KB" : "MB",
            outputSize, pattern, testTime, blockSize);

    if (pw_fillRefBuf(&buf, pattern, blockSize) != EOK) {
        logMsg(ERROR, "buffer preparation failed\n");
        rc = ERROR;
        goto paceWriteExit;
    }

    if ((startTime = time(&tloc)) == -1) {
        logMsg(ERROR, "time() failed\n");
        rc = ERROR;
        goto paceWriteExit;
    }

    startTime = lastTime = tloc;

#if defined (SOLARIS)    
    if (ctime_r(&startTime, tm, sizeof(tm)) == NULL) {
        logMsg(ERROR, "ctime_r() failed\n");
        rc = ERROR;
        goto paceWriteExit;
    }
#else
    if (ctime_r(&startTime, tm) == NULL) {
        logMsg(ERROR, "ctime_r() failed\n");
        rc = ERROR;
        goto paceWriteExit;
    }
#endif

    logMsg(ERR_MSG, "%s: starting time (%ld) %s\n", 
            __FUNCTION__, startTime, tm);

    if (testTime != NOLIMIT) {
        endTime = startTime + testTime;
    }

    if (bw != NOLIMIT) {
        int base;
        if (Bflg) {
            BW = bw * MB;
            base = MB;
        } else {
            BW = bw * KB;
            base = KB;
        }

        if (blockSize < base) {
            r = (base / blockSize);
            bs = 1;
            m1 = 0;
        } else {
            r = 0;
            bs = blockSize / (base);
            m1 = blockSize % (base);
        }

        if (bs == bw) {
            if (r != 0) {
                /* 
                 * must be blockSize < 1 base size
                 */
                idleTick = TICK_PER_SEC / r;
            } else {
                idleTick = TICK_PER_SEC + (TICK_PER_SEC / (base / m1));
            }
        } else if (bs > bw) {
            R = (blockSize / bw);
            m2 = (blockSize % bw);
            idleTick = (TICK_PER_SEC * R + (TICK_PER_SEC / (bw / m2)));
        } else {
            idleTick = (TICK_PER_SEC / (bw / bs));
        }
    }

    logMsg(ERR_MSG2, "bs: %d, r: %d, m1: %d, R: %d m2: %d\n",
            bs, r, m1, R, m2);
    logMsg(ERR_MSG2, "bw: %ld%s bs: %ld idle: %d\n",
            bw, (Bflg) ? "MB" : "KB", blockSize, idleTick);

    if (writeCnt != NOLIMIT) {
        do {
            if ((rc = pw_write(ofd, buf, blockSize)) == -1) {
                logMsg(ERROR, "write failed\n");
                goto paceWriteExit;
            } else {
                totalBlock++;
            }

            if ((outputSize != NOLIMIT) && (fileOffset >= outputSize)) {
                if (lseek(ofd, 0L, SEEK_SET) == -1) {
                    logMsg(ERROR, "lseek failed\n");
                    rc = ERROR;
                    goto paceWriteExit;
                }
                fileOffset = 0;
            } else {
                fileOffset += blockSize;
            }

            loop++;
        } while (loop < writeCnt);
    } else {
        do {
            if (testTime != NOLIMIT) {
                if (tloc > endTime) {
                    break;
                }
            }

            if (((bw == NOLIMIT) || ((bw != NOLIMIT) && (byteWrote < BW))) &&
                ((outputSize == NOLIMIT) || 
                 ((outputSize != NOLIMIT) && (fileOffset < outputSize)))) {
                if ((rc = pw_write(ofd, buf, blockSize)) == -1) {
                    logMsg(ERROR, "write failed\n");
                    goto paceWriteExit;
                } else {
                    byteWrote += blockSize;
                    fileOffset += blockSize;
                    totalBlock++;

                    logMsg(ERR_DEBUG, "%s: block(%d) bw(%d/%d) hrtime(%ld)\n", 
                            __FUNCTION__, blockSize, byteWrote, BW, hrtime);

                    if ((outputSize != NOLIMIT) && (fileOffset >= outputSize)) {
                        if (lseek(ofd, 0L, SEEK_SET) == -1) {
                            logMsg(ERROR, "lseek failed\n");
                            rc = ERROR;
                            goto paceWriteExit;
                        } else {
                            fileOffset = 0;
                        }
                    }
                }
            } else {
                logMsg(ERR_DEBUG, "%s: bs(%ld) bw(%d/%d) os(%ld) fs (%ld)\n", 
                        __FUNCTION__, blockSize, byteWrote, BW, outputSize,
                        fileOffset);
            }

            if (bw != NOLIMIT) {
                if ((hrtime = times(&hrtloc)) == -1) {
                    rc = ERROR;
                    goto paceWriteExit;
                }

                lastHrtime = hrtime;

                while ((hrtime - lastHrtime) < idleTick) {
                    if ((hrtime = times(&hrtloc)) == -1) {
                        rc = ERROR;
                        goto paceWriteExit;
                    }
                }
            }

            if ((testTime != NOLIMIT) && (chktime == LTRESO)) {
                if (time(&tloc) == -1) {
                    logMsg(ERROR, "time failed\n");
                    rc = ERROR;
                    goto paceWriteExit;
                }
                chktime = 0;
            }

            if (tloc > lastTime) {
                lastTime = tloc;
                byteWrote = 0;
                logMsg(ERR_DEBUG, "%s: second increased\n", __FUNCTION__); 
            } 
            chktime++;
        } while (1);
    }

    if ((endTime = time(&tloc)) == -1) {
        logMsg(ERROR, "time() failed\n");
        rc = ERROR;
        goto paceWriteExit;
    }

#if defined (SOLARIS)    
    if (ctime_r(&endTime, tm, sizeof(tm)) == NULL) {
        logMsg(ERROR, "ctime_r failed\n");
        rc = ERROR;
        goto paceWriteExit;
    }
#else
    if (ctime_r(&endTime, tm) == NULL) {
        rc = ERROR;
        goto paceWriteExit;
    }
#endif

    logMsg(ERR_MSG, "%s: end time (%ld) %s\n", __FUNCTION__, endTime, tm);
    logMsg(ERR_MSG, "%s: blockSize: %ld byte, total blocks wrote %ld \n",
            __FUNCTION__, blockSize, totalBlock);

paceWriteExit:
    if (buf != NULL) {
        free((void *)buf);
    }

    return rc;
}

int
pw_parseArg
(
    int     argc,
    char    *argv[],
    char    *ofile,
    LONG    *bw,
    long    *outputSize,
    long    *testTime,
    long    *blockSize,
    char    *pattern,
    int     *cloneNum,
    long    *writeCnt,
    long    *initOffset
)
{
    int     c;
    int     errCode = 0;
    int     errflg = 0;
    int     cflg = 0;
    int     bflg = 0;
    int     dflg = 0;
    int     fflg = 0;
    int     lflg = 0;
    int     pflg = 0;
    int     sflg = 0;
    int     Sflg = 0;
    int     tflg = 0;
    int     Tflg = 0;
    int     wflg = 0;

    while ((c = getopt(argc,
                       argv,
                       "ab:B:c:d:D:f:h?s:S:l:p:qt:T:w:vX:")) != -1) {
        switch (c) {
        case 'a':
            asyncFlag = 1;
            break;

        case 'b':
            *bw = atol(optarg);
            if (*bw < 0) {
                errCode = ERR_INVALID;
                logMsg(errCode, "Bandwidth can not be netgtive\n");
                errflg++;
            }
            
            if (Bflg) {
                errCode = ERR_INVALID;
                logMsg(errCode, "-b & -B are exclusive\n");
                errflg++;
            }
            bflg++;
            break;

        case 'B':
            *bw = atol(optarg);
            if (*bw < 0) {
                errCode = ERR_INVALID;
                logMsg(errCode, "Bandwidth can not be netgtive\n");
                errflg++;
            }
            
            if (bflg) {
                errCode = ERR_INVALID;
                logMsg(errCode, "-b & -B are exclusive\n");
                errflg++;
            }
            Bflg++;
            break;

        case 'c':
            *cloneNum = atoi(optarg);
            if (*cloneNum <= 0) {
                errCode = ERR_INVALID;
                logMsg(errCode, "Process number shall be at least one\n");
                errflg++;
            }
            cflg++;
            break;

        case 'd':
            if (fflg) {
                errCode = ERR_INVALID;
                logMsg(errCode, "-d & -f are exclusive\n");
                errflg++;
            }
            
            if (strlen(optarg) > MAX_FILENAME_SIZE) {
                errCode = ERR_INVALID;
                logMsg(errCode, "Max filename size is %d\n", MAX_FILENAME_SIZE);
                errflg++;
            } else {
                bzero((void *)ofile, MAX_FILENAME_SIZE);
                strcpy(ofile, optarg);
            }

            dflg++;
            bdsk = 1;

            break;

        case 'D':
            debugFlag |= atoi(optarg);
            break;

        case 'f':
            if (dflg) {
                errCode = ERR_INVALID;
                logMsg(errCode, "-d & -f are exclusive\n");
                errflg++;
            }
            
            if (strlen(optarg) > MAX_FILENAME_SIZE) {
                errCode = ERR_INVALID;
                logMsg(errCode, "Max filename size is %d\n", MAX_FILENAME_SIZE);
                errflg++;
            } else {
                bzero((void *)ofile, MAX_FILENAME_SIZE);
                strcpy(ofile, optarg);
            }

            fflg++;

            break;

        case 'h':
        case '?':
            usage();
            exit(0);

        case 'l':
            *writeCnt = atol(optarg);
            if (*writeCnt < 0) {
                errCode = ERR_INVALID;
                logMsg(errCode, "loop count can not be netgtive\n");
                errflg++;
            }
            
            if (Tflg || tflg) {
                errCode = ERR_INVALID;
                logMsg(errCode, "-l & -t & -T are exclusive\n");
                errflg++;
            }
            lflg++;
            break;

        case 'p':
            if (strlen(optarg) > MAX_PATTERN_SIZE) {
                errCode = ERR_INVALID;
                logMsg(errCode, "Max pattern size is %d\n", MAX_PATTERN_SIZE);
                errflg++;
            } else {
                bzero((void *)pattern, MAX_PATTERN_SIZE);
                strcpy(pattern, optarg);
                pflg++;
            }
            break;

        case 'q':
            qflg++;
            break;

        case 's':
            *outputSize = atol(optarg);
            if (*outputSize < 0) {
                errCode = ERR_INVALID;
                logMsg(errCode, "Output size can not be netgtive\n");
                errflg++;
            }
            sflg++;
            break;

        case 'S':
            *initOffset = atol(optarg);
            Sflg++;
            break;

        case 't':
            *testTime = atol(optarg) * 60;
            if (*testTime < 0) {
                errCode = ERR_INVALID;
                logMsg(errCode, "testTime can not be netgtive\n");
                errflg++;
            }
            
            if (Tflg || lflg) {
                errCode = ERR_INVALID;
                logMsg(errCode, "-l & -t & -T are exclusive\n");
                errflg++;
            }
            tflg++;
            break;

        case 'T':
            *testTime = atol(optarg) * 3600;
            if (*testTime < 0) {
                errCode = ERR_INVALID;
                logMsg(errCode, "testTime can not be netgtive\n");
                errflg++;
            }

            if (tflg || lflg) {
                errCode = ERR_INVALID;
                logMsg(errCode, "-l & -t & -T are exclusive\n");
                errflg++;
            }
            Tflg++;
            break;

        case 'v':
            verbose = 1;
            break;

        case 'w':
            *blockSize = atol(optarg);
            if (*blockSize < 0) {
                errCode = ERR_INVALID;
                logMsg(errCode, "Block size can not be netgtive\n");
                errflg++;
            }
            wflg++;
            break;

        case 'X':
            verbose = 1;
            test_time(atol(optarg));
            return EOK;

        default:
            errCode = ERR_INVALID;
            logMsg(errCode, "%s is not a valid parameter\n", c);
            usage();
            errflg++;
        }
    }

    if (errflg || !(fflg || dflg)) {
        usage();
        return ERROR;
    }

    /*
     * set default value
     */
    if ((bflg == 0) && (Bflg == 0)) {
        *bw = NOLIMIT;
    }

    if (sflg == 0) {
        *outputSize = NOLIMIT;
    }

    if (pflg == 0) {
        strcpy(pattern, DEFAULT_PATTERN);
    }

    if (tflg == 0) {
        *testTime = NOLIMIT;
    }

    if (wflg == 0) {
        *blockSize = DEFAULT_WRBS;
    }

    if (cflg == 0) {
        *cloneNum = DEFAULT_CLONE;
    }

    if (lflg == 0) {
        *writeCnt = NOLIMIT;
    }

    if (Sflg == 0) {
        if (dflg) {
            *initOffset = DEFAULT_OFFSET;
        } else {
            *initOffset = 0;
        }
    } else {
        if (!dflg) {
            *initOffset = 0;
        }
    }

    logMsg(ERR_DEBUG,
            "%s: output file: %s, BW: %ld%s, output size: %ld, "
            "pattern: \"%s\", test time: %d, blockSize: %ld, "
            "process number: %d, write count: %ld initOffset: %ld\n",
            __FUNCTION__,
            ofile, *bw, (Bflg == 0) ? "KB" : "MB",
            *outputSize, pattern, *testTime, *blockSize,
            *cloneNum, *writeCnt, *initOffset);
    
    return EOK;
}

int
main
(   
    int     argc,
    char    *argv[]
)
{
    int             errCode = EOK;
    LONG            bw = 0;
    long            outputSize;
    long            testTime;
    long            blockSize;
    long            initOffset;
    int             cloneNum;
    long            writeCnt;
    int             ofd;
    int             ifd;
    char            ofile[MAX_FILENAME_SIZE];
    char            ifile[MAX_FILENAME_SIZE];
    char            pattern[MAX_PATTERN_SIZE];
    int             i;
    

    if ((errCode = pw_parseArg(argc,
                          argv,
                          ofile,
                          &bw,
                          &outputSize,
                          &testTime,
                          &blockSize,
                          pattern,
                          &cloneNum,
                          &writeCnt,
                          &initOffset)) != EOK) {
        return errCode;
    }

    if (cloneNum > 1) {
        for (i = 0; i < argc; i++) {
            if (strcmp(argv[i], "-c") == 0) {
                strcpy(argv[i+1], "1");
            }
        }
        for (i = 0; i < cloneNum; i++) {
            if (fork() == 0) {
                logMsg(ERR_MSG, "Child process %d\n", i+1);
                execvp("pacewrite", argv);
            }
        }
    } else {
#ifndef VERIFY_ONLY
        if (asyncFlag == 1) {
            if ((ofd = open(ofile, O_RDWR | O_CREAT)) == -1) {
                logMsg(ERR_SYSCALL, "open\n");
                exit(-1);
            }
        } else {
            if ((ofd = open(ofile, O_RDWR | O_CREAT | O_SYNC)) == -1) {
                logMsg(ERR_SYSCALL, "open\n");
                exit(-1);
            }
        }
#else
        if ((ofd = open(ofile, O_RDWR)) == -1) {
            logMsg(ERR_SYSCALL, "open\n");
            exit(-1);
        }
#endif

        if (lseek(ofd, initOffset, SEEK_SET) == -1) {
            logMsg(ERROR, "lseek() error, fd: %ld offset: %ld\n",
                    ofd, initOffset);
            goto closeBeforeExit;
        }

#ifndef VERIFY_ONLY
        if ((errCode = paceWrite(ofd,
                                 bw,
                                 outputSize,
                                 pattern,
                                 testTime,
                                 blockSize,
                                 writeCnt)) != ERR_OK) {
            logMsg(errCode, "paceWrite failed\n");
        }
#endif
        
#ifndef VERIFY_ONLY
        if (qflg) {
#endif
            if ((errCode = pw_verify(ofd,
                                     initOffset,
                                     outputSize,
                                     blockSize,
                                     pattern)) != ERR_OK) {
                logMsg(errCode, "Verify failed\n");
            }

#ifndef VERIFY_ONLY
        }
#endif

        logMsg(ERR_DEBUG, "%s: errCode (%ld)\n", __FUNCTION__, errCode);

closeBeforeExit:
        if (close(ofd) == -1) {
            logMsg(ERR_SYSCALL, "close\n");
        }
    }

    exit(errCode);
}

