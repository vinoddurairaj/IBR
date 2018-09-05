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
/*
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

#include <sys/wait.h>
#include <signal.h>
#include <sys/procfs.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <malloc.h>
#include <sys/time.h>
#include <unistd.h>
#include <stropts.h>
#include <poll.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/fs/ufs_fs.h>
#include <network.h>

int sock;
char* block = NULL;
int blocksize = 0;

void
waitforconnect (pid_t* childpid)
{
  int n;
  int socket_fd;
  int recfd;
  int length;
  int len;
  int response;
  struct sockaddr_in myaddr;
  struct sockaddr_in client_addr;
#if !defined(FTD_IPV4)
  struct sockaddr_storage ss;
  struct addrinfo *res;
  struct addrinfo hints;
  int ret = -1;
  int retval = -1;
  char tmpport[16] ;
#endif
  int nbytes;
  int family;  
  *childpid = -1;

#if defined(FTD_IPV4)
  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf (stderr, "socket create failure\n");
    exit (1);
  }
  n = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n,
                 sizeof(int)) < 0) {
    fprintf (stderr, "setsockopt failure\n");
    exit(1);
  }
  bzero((char*)&myaddr, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(1575);
  len = sizeof(myaddr);
  if (bind(socket_fd, (struct sockaddr*)&myaddr, len) < 0) {
    fprintf (stderr, "bind failure\n");
    exit (1);
  }
  if (listen(socket_fd, 5) < 0) {
    fprintf (stderr, "listent failure\n");
    exit (1);
  }
  length = sizeof(client_addr);
   
  while (1) {
    if ((recfd = accept(socket_fd, (struct sockaddr*)&client_addr,
        &length)) < 0) {
      fprintf (stderr, "accept failure\n");
      exit (1);
    }
}
#else
    memset(&hints, '\0', sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(tmpport, sizeof(tmpport),"%d", listenport);
    ret = getaddrinfo(NULL, tmpport, &hints, &res);
    if  (ret != 0)
       reporterr(ERRFAC, M_SOCKFAIL, ERRCRIT, gai_strerror(ret));
    while (res)
    {
     socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
     if (!(socket_fd < 0))
     {
       if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(int)) == 0) {
          if (bind(socket_fd, res->ai_addr, res->ai_addrlen) == 0)
          {
                retval = 0;
                break;
          }
        }
     }
     res = res->ai_next;
   }
   freeaddrinfo(res);
  if (retval == -1)
   {
       fprintf (stderr, "bind failure\n");
       exit (1);
   }
  if (listen(socket_fd, 5) < 0) {
      fprintf (stderr, "listent failure\n");
      exit (1);
    }

  length = sizeof(ss);

  while(1) {
  if ((recfd = accept(socket_fd, (struct sockaddr*)&ss,
        &length)) < 0) {
       fprintf (stderr, "accept failure\n");
       exit (1);
     }
 }
#endif
    *childpid = fork();
    switch (*childpid) {
    case -1:
      fprintf (stderr, "fork failure\n");
      exit (1);
    case 0:
      /* child */
      close (socket_fd);
      sock = recfd;
      (void) setsockopt (sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&n,
                         sizeof(int));
      nbytes = 1;
      while (nbytes > 0) {
        nbytes = read (sock, (void*)block, blocksize);
      }
    default:
      close (recfd);
      /* parent */
      break;
    }
}

int main (int argc, char** argv)
{
  pid_t pid;

  if (argc != 2) {
    fprintf (stderr, "Usage:  %s <buffersize>\n\n");
    exit (1);
  }
  blocksize = 1024 * atoi(argv[1]);
  block = (char*) malloc (blocksize);
  waitforconnect (&pid);
}
