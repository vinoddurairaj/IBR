#include <stdio.h>
#include <io.h>
#include <winsock2.h>

#include "getopt.h" 

/*
 *	T T C P . C
 *
 * Test TCP connection.  Makes a connection on port 5001
 * and transfers fabricated buffers or data copied from stdin.
 *
 * Usable on 4.2, 4.3, and 4.1a systems by defining one of
 * BSD42 BSD43 (BSD41a)
 * Machines using System V with BSD sockets should define SYSV.
 *
 * Modified for operation under 4.2BSD, 18 Dec 84
 *      T.C. Slattery, USNA
 * Minor improvements, Mike Muuss and Terry Slattery, 16-Oct-85.
 * Modified in 1989 at Silicon Graphics, Inc.
 *	catch SIGPIPE to be able to print stats when receiver has died 
 *	for tcp, don't look for sentinel during reads to allow small transfers
 *	increased default buffer size to 8K, g_nNumBuffersToSend to 2K to transfer 16MB
 *	moved default port to 5001, beyond IPPORT_USERRESERVED
 *	make sinkmode default because it is more popular, 
 *		-s now means don't sink/source.
 *       in sink/source mode use pattern generator to fill send
 *       buffer (once at init time). received data is tossed.
 *	count number of read/write system calls to see effects of 
 *		blocking from full socket buffers
 *	for tcp, -D option turns off buffered writes (sets TCP_NODELAY sockopt)
 *	buffer alignment options, -A and -O
 *	print stats in a format that's a bit easier to use with grep & awk
 *	for SYSV, mimic BSD routines to use most of the existing timing code
 * Modified by Steve Miller of the University of Maryland, College Park
 *	-b sets the socket buffer size (SO_SNDBUF/SO_RCVBUF)
 * Modified Sept. 1989 at Silicon Graphics, Inc.
 *	restored -s sense at request of tcs@brl
 * Modified Oct. 1991 at Silicon Graphics, Inc.
 *	use getopt(3) for option processing, add -f and -T options.
 *	SGI IRIX 3.3 and 4.0 releases don't need #define SYSV.
 *
 * PCAUSA Version 1.00.00.01 - Modified April, 1999 at Printing
 * Communications Assoc., Inc. (PCAUSA) PCAUSA. Initial port to Winsock.
 *
 * PCAUSA Version 1.00.00.02 - Modified January, 2000 at Printing
 * Communications Assoc., Inc. (PCAUSA) to fix setting of setsockopt call
 * for TCP_NODELAY.
 *
 * Distribution Status -
 *      Public Domain.  Distribution Unlimited.
 */

static char RCSid[] = "ttcp.c $Revision: 1.1 $";

#define  PCATTCP_VERSION   "1.00.00.02"

extern int errno;

/////////////////////////////////////////////////////////////////////////////
//// GLOBAL VARIABLES
//

WSADATA  g_WsaData;

#ifndef IPPORT_TTCP
#define IPPORT_TTCP          5001
#endif

char *g_RemoteHostName = NULL;      // ptr to name of remote host
struct hostent *addr;

BOOLEAN  g_bTransmit = FALSE;
USHORT   g_Protocol = IPPROTO_TCP;

IN_ADDR  g_RemoteAddress;  // Host Byte Order
USHORT   g_Port = IPPORT_TTCP;   // Host Byte Order

BOOLEAN  g_bSinkMode = TRUE;   // FALSE = normal I/O, TRUE = sink/source mode
BOOLEAN  g_bSendContinuously = FALSE;
int      g_nNumBuffersToSend = 2 * 1024; // number of buffers to send in sinkmode
int      g_nBufferSize = 8 * 1024;// length of buffer

int      g_nNoDelay = 0;     // set TCP_NODELAY socket option

int b_flag = 0;      // use mread()
int options = 0;     // socket options
int one = 1;         // for 4.3 BSD style setsockopt()
int zero = 0;        // for 4.3 BSD style setsockopt()

char *buf = NULL;    // ptr to dynamic buffer

int bufoffset = 0;      // align buffer to this
int bufalign = 16*1024; //modulo this

int sockbufsize = 0; // socket buffer size to use


int verbose = 0;     // 0=print basic info, 1=print cpu rate, proc
				         // resource usage.
char fmt = 'K';      // output format:
                     // k = kilobits, K = kilobytes,
                     // m = megabits, M = megabytes, 
                     // g = gigabits, G = gigabytes
int touchdata = 0;   // access data after reading


struct sockaddr_in sinme;     // me
struct sockaddr_in sinhim;    // him

struct sockaddr_in frominet;
int domain, fromlen;
SOCKET fd;                       // fd of network socket

char stats[128];
double nbytes;                   // bytes on net
unsigned long numCalls;		      // # of I/O system calls
DWORD tStart, tFinish;
double cput, realt;		         // user, real time (seconds)

#define  UDP_GUARD_BUFFER_LENGTH  4

//
// Forward Procedure Prototypes
//
void err( char *s );
void mes( char *s );
void KS_FillPattern( char *cp, int cnt );

int Nread( SOCKET fd, PVOID buf, int count );
int Nwrite( SOCKET fd, PVOID buf, int count );
int mread( SOCKET fd, char *bufp, unsigned n);
char *outfmt(double b);
void prep_timer( VOID );
void psecs( long l, char *cp );

void delay( int us );

VOID IntitializeTestDefaults( VOID )
{
   g_bTransmit = FALSE;
   g_Protocol = IPPROTO_TCP;

   memset(
      &g_RemoteAddress,
      0x00,
      sizeof( IN_ADDR )
      );

   g_Port = IPPORT_TTCP;

   //
   // Setup Size Of Send/Receive Buffer
   //
   g_nBufferSize = 8 * 1024;

   //
   // Setup SinkMode Default
   // ----------------------
   // SinkMode description:
   //   TRUE  -> A pattern generator is used fill the send buffer. This
   //            is done only once. Received data is simply counted.
   //   FALSE -> Data to be sent is read from stdin. Received data is
   //            written to stdout.
   //
   // g_nNumBuffersToSend specifies the number of buffers to be sent
   // in SinkMode.
   //
   g_bSinkMode = TRUE;   // FALSE = normal I/O, TRUE = sink/source mode
   g_bSendContinuously = FALSE;
   g_nNumBuffersToSend = 2 * 1024; // number of buffers to send in sinkmode
}

//
// Usage Message
//
char Usage[] = "\
Usage: pcattcp -t [-options] host [ < in ]\n\
       pcattcp -r [-options > out]\n\
Common options:\n\
	-l ##	length of bufs read from or written to network (default 8192)\n\
	-u	use UDP instead of TCP\n\
	-p ##	port number to send to or listen at (default 5001)\n\
	-s	-t: source a pattern to network\n\
		-r: sink (discard) all data from network\n\
	-A	align the start of buffers to this modulus (default 16384)\n\
	-O	start buffers at this offset from the modulus (default 0)\n\
	-v	verbose: print more statistics\n\
	-d	set SO_DEBUG socket option\n\
	-b ##	set socket buffer size (if supported)\n\
	-f X	format for rate: k,K = kilo{bit,byte}; m,M = mega; g,G = giga\n\
Options specific to -t:\n\
	-n ##	number of source bufs written to network (default 2048)\n\
	-D	don't buffer TCP writes (sets TCP_NODELAY socket option)\n\
Options specific to -r:\n\
	-B	for -s, only output full blocks as specified by -l (for TAR)\n\
	-T	\"touch\": access each byte as it's read\n\
";

int main( int argc, char **argv )
{
	unsigned long addr_tmp;
   int   c;

   printf( "PCAUSA Test TCP Utility V%s\n", PCATTCP_VERSION );

   IntitializeTestDefaults();

	if (argc < 2)
   {
      fprintf(stderr,Usage);
      exit( 0 );
   }

   while (optind != argc)
   {
	   c = getopt(argc, argv, "drstuvBDTb:f:l:n:p:A:O:" );

		switch (c)
      {
         case EOF:
            optarg = argv[optind];
            optind++;
            break;

         case 'B':
            b_flag = 1;
            break;

         case 't':
            g_bTransmit = TRUE;
            break;

         case 'r':
            g_bTransmit = FALSE;
            break;

         case 'd':
            options |= SO_DEBUG;
            break;

         case 'D':
#ifdef TCP_NODELAY
            g_nNoDelay = 1;
#else
            fprintf(stderr, 
            "pcattcp: -D option ignored: TCP_NODELAY socket option not supported\n");
#endif
            break;

         case 'n':
            g_nNumBuffersToSend = atoi(optarg);
            break;

         case 'l':
            g_nBufferSize = atoi(optarg);
            break;

         case 's':
            g_bSinkMode = !g_bSinkMode;
            break;

         case 'p':
            g_Port = atoi(optarg);
            break;

         case 'u':
            g_Protocol = IPPROTO_UDP;
            break;

         case 'v':
            verbose = 1;
            break;

         case 'A':
            bufalign = atoi(optarg);
            break;

         case 'O':
            bufoffset = atoi(optarg);
            break;

         case 'b':
#if defined(SO_SNDBUF) || defined(SO_RCVBUF)
            sockbufsize = atoi(optarg);
#else
            fprintf(stderr, 
               "pcattcp: -b option ignored: SO_SNDBUF/SO_RCVBUF socket options not supported\n");
#endif
            break;

         case 'f':
            fmt = *optarg;
            break;

         case 'T':
            touchdata = 1;
            break;

		   default:
            fprintf(stderr,Usage);
            exit( 0 );
      }
   }

   //
   // Start Winsock 2
   //
   if( WSAStartup( MAKEWORD(0x02,0x00), &g_WsaData ) == SOCKET_ERROR )
   {
      fprintf(stderr,Usage);
      err( "WSAStartup" );
   }

   //
   // Setup IP Addresses And Port
   //
   if( g_bTransmit )
   {
      printf( "TTCP Transmit Test\n" );

      if( g_Protocol == IPPROTO_TCP )
      {
         printf( "  Protocol   : TCP\n" );
      }
      else if( g_Protocol == IPPROTO_UDP )
      {
         printf( "  Protocol   : UDP\n" );
      }
      else
      {
         printf( "  Protocol   : Invalid\n" );
         exit( 0 );
      }

      printf( "  Port       : %d\n", g_Port );

#ifdef ZNEVER
      if (optind == argc)
      {
         fprintf(stderr,Usage);
         WSACleanup();
         exit( 0 );
      }
#endif

      //
      // Setup Remote IP Addresses For Test
      //
		memset((char *)&sinhim, 0x00, sizeof(sinhim));
		g_RemoteHostName = argv[argc - 1];

      if( atoi( g_RemoteHostName ) > 0 )
      {
         //
         // Numeric
         //
         sinhim.sin_family = AF_INET;
			sinhim.sin_addr.s_addr = inet_addr( g_RemoteHostName );
      }
      else
      {
         printf( "host: \042%s\042\n", g_RemoteHostName );

         if ((addr=gethostbyname( g_RemoteHostName )) == NULL)
         {
            err("bad hostname");
         }

         sinhim.sin_family = addr->h_addrtype;
         memcpy((char*)&addr_tmp, addr->h_addr, addr->h_length );
         sinhim.sin_addr.s_addr = addr_tmp;
      }
      
      sinhim.sin_port = htons( g_Port );

      //
      // Setup Local IP Addresses For Test
      //
      sinme.sin_family = AF_INET;
      sinme.sin_port = 0;		/* free choice */

      printf( "addr: %s\n", inet_ntoa( sinhim.sin_addr ) );
   }
   else
   {
      printf( "TTCP Receive Test\n" );

      if( g_Protocol == IPPROTO_TCP )
      {
         printf( "  Protocol   : TCP\n" );
      }
      else if( g_Protocol == IPPROTO_UDP )
      {
         printf( "  Protocol   : UDP\n" );
      }
      else
      {
         printf( "  Protocol   : Invalid\n" );
         exit( 0 );
      }

      printf( "  Port       : %d\n", g_Port );

      //
      // Setup Local IP Addresses For Test
      //
      sinme.sin_family = AF_INET;
      sinme.sin_port =  htons( g_Port );
   }

   //
   // Setup Buffer Configuration
   //
   if( g_Protocol == IPPROTO_UDP && g_nBufferSize <= UDP_GUARD_BUFFER_LENGTH )
   {
      g_nBufferSize = UDP_GUARD_BUFFER_LENGTH + 1; // send more than the sentinel size
   }

   if ( (buf = (char *)malloc(g_nBufferSize+bufalign)) == (char *)NULL)
      err("malloc");

   if (bufalign != 0)
      buf +=(bufalign - ((int)buf % bufalign) + bufoffset) % bufalign;

   if( g_bTransmit )
   {
      fprintf(
         stdout,
         "pcattcp-t: buflen=%d, nbuf=%d, align=%d/%d, port=%d",
         g_nBufferSize, g_nNumBuffersToSend, bufalign, bufoffset, g_Port
         );

      if( sockbufsize )
         fprintf(stdout, ", sockbufsize=%d", sockbufsize);

      fprintf(stdout, "  %s -> %s\n", g_Protocol == IPPROTO_UDP ? "udp" : "tcp", g_RemoteHostName );
   }
   else
   {
      fprintf(
         stdout,
         "pcattcp-r: buflen=%d, nbuf=%d, align=%d/%d, port=%d",
         g_nBufferSize, g_nNumBuffersToSend, bufalign, bufoffset, g_Port
         );

      if (sockbufsize)
         fprintf(stdout, ", sockbufsize=%d", sockbufsize);

      fprintf(stdout, "  %s\n", g_Protocol == IPPROTO_UDP ? "udp" : "tcp");
   }

   //
   // Open Socket For Test
   //
   if( (fd = socket(
               AF_INET,
               g_Protocol == IPPROTO_UDP ? SOCK_DGRAM : SOCK_STREAM,
               0
               )
            ) == INVALID_SOCKET
      )
   {
      err("socket");
   }

	mes("socket");

   //
   // Bind Socket With Local Address
   //
   if( bind(
         fd,
         (struct sockaddr * )&sinme,
         sizeof(sinme)
         ) == SOCKET_ERROR
      )
   {
      err("bind");
   }

#if defined(SO_SNDBUF) || defined(SO_RCVBUF)
   if( sockbufsize )
   {
      if( g_bTransmit )
      {
         if( setsockopt(
               fd,
               SOL_SOCKET,
               SO_SNDBUF,
               (char * )&sockbufsize,
               sizeof sockbufsize
               ) == SOCKET_ERROR
            )
         {
            err("setsockopt: sndbuf");
         }

         mes("sndbuf");
      }
      else
      {
         if( setsockopt(
               fd,
               SOL_SOCKET,
               SO_RCVBUF,
               (char * )&sockbufsize,
               sizeof sockbufsize
               ) == SOCKET_ERROR
            )
         {
            err("setsockopt: rcvbuf");
         }

         mes("rcvbuf");
      }
   }
#endif

   //
   // Start TCP Connections
   //
   if( g_Protocol != IPPROTO_UDP )
   {
      if( g_bTransmit )
      {
         //
         // We are the client if transmitting
         //
         if( options )
         {
            if( setsockopt(
                  fd,
                  SOL_SOCKET,
                  options,
                  (PCHAR )&one,
                  sizeof(one)
                  ) == SOCKET_ERROR
               )
            {
               err("setsockopt");
            }
         }

#ifdef TCP_NODELAY
         {
            //
            // Set TCP_NODELAY Send Option
            //
            struct protoent *p;
            int optlen = sizeof( g_nNoDelay );
            p = getprotobyname("tcp");

            if( p && setsockopt(
                        fd,
                        p->p_proto,
                        TCP_NODELAY, 
                        (PCHAR )&g_nNoDelay,
                        sizeof( g_nNoDelay )
                        ) == SOCKET_ERROR
               )
            {
               int nError = WSAGetLastError();
               fprintf( stderr, "  Error: 0x%8.8X\n", nError );
               mes("setsockopt: g_nNoDelay option failed");
            }

            //
            // Query And Display TCP_NODELAY Send Option
            //
            if( p && getsockopt(
                        fd,
                        p->p_proto,
                        TCP_NODELAY, 
                        (PCHAR )&g_nNoDelay,
                        &optlen
                        ) != SOCKET_ERROR
               )
            {
               if( g_nNoDelay )
               {
	               fprintf(stderr,"pcattcp%s: g_nNoDelay ENABLED (%d)\n",
                     g_bTransmit ? "-t" : "-r", g_nNoDelay );
               }
               else
               {
	               fprintf(stderr,"pcattcp%s: g_nNoDelay DISABLED (%d)\n",
                     g_bTransmit ? "-t" : "-r", g_nNoDelay );
               }
            }
            else
            {
               int nError = WSAGetLastError();
               fprintf( stderr, "  Error: 0x%8.8X\n", nError );
               mes("getsockopt: g_nNoDelay option failed");
            }
         }
#endif

         //
         // Connect To Remote Server
         //
         if(connect(fd, (struct sockaddr * )&sinhim, sizeof(sinhim) ) == SOCKET_ERROR)
            err("connect");

         mes("connect");
      }
      else
      {
         //
         // Otherwise, We Are The Server
         //
         listen( fd, 0 );  // allow a queue of 0

         if(options)
         {
            if( setsockopt(
                  fd,
                  SOL_SOCKET,
                  options,
                  (PCHAR )&one,
                  sizeof(one)
                  ) == SOCKET_ERROR
               )
            {
               err("setsockopt");
            }
         }
         
         fromlen = sizeof(frominet);
         domain = AF_INET;

         if( (fd=accept(fd, (struct sockaddr * )&frominet, &fromlen) ) == SOCKET_ERROR)
         {
            err("accept");
         }
         else
         {
            struct sockaddr_in peer;
            int peerlen = sizeof(peer);
            if (getpeername(fd, (struct sockaddr *) &peer, 
				   &peerlen) == SOCKET_ERROR)
            {
               err("getpeername");
            }
            fprintf(stderr,"pcattcp-r: accept from %s\n", 
               inet_ntoa(peer.sin_addr));
         }
      }
   }

   prep_timer();
   errno = 0;

   if( g_bSinkMode )
   {      
      register int cnt;
      if( g_bTransmit )
      {
         KS_FillPattern( buf, g_nBufferSize );

         if( g_Protocol == IPPROTO_UDP )
            Nwrite( fd, buf, UDP_GUARD_BUFFER_LENGTH ); /* rcvr start */

         if( g_bSendContinuously )
         {
            while (Nwrite(fd,buf,g_nBufferSize) == g_nBufferSize)
               nbytes += g_nBufferSize;
         }
         else
         {
            while (g_nNumBuffersToSend-- && Nwrite(fd,buf,g_nBufferSize) == g_nBufferSize)
               nbytes += g_nBufferSize;
         }

         if( g_Protocol == IPPROTO_UDP )
            Nwrite( fd, buf, UDP_GUARD_BUFFER_LENGTH ); /* rcvr end */
      }
      else
      {
         if( g_Protocol == IPPROTO_UDP )
         {
            while( (cnt=Nread(fd,buf,g_nBufferSize)) > 0 )
            {
               static int going = 0;
               if( cnt <= UDP_GUARD_BUFFER_LENGTH )
               {
                  if( going )
                     break;	/* "EOF" */
                  going = 1;
                  prep_timer();
               }
               else
               {
                  nbytes += cnt;
               }
            }
         }
         else
         {
            while( (cnt=Nread(fd,buf,g_nBufferSize) ) > 0)
            {
               nbytes += cnt;
            }
         }
      }
   }
   else
   {
      register int cnt;

      if( g_bTransmit )
      {
         //
         // Read From stdin And Write To Remote
         //
         while( ( cnt = read(0,buf,g_nBufferSize ) ) > 0
            && Nwrite(fd,buf,cnt) == cnt
            )
         {
            nbytes += cnt;
         }
      }
      else
      {
         //
         // Read From Remote And Write To stdout
         //
         while( ( cnt = Nread(fd,buf,g_nBufferSize ) ) > 0
            && write(1,buf,cnt) == cnt
            )
         {
            nbytes += cnt;
         }
      }
   }

	if(errno)
      err("IO");

   tFinish = GetTickCount();

   if( g_Protocol == IPPROTO_UDP && g_bTransmit )
   {
      Nwrite( fd, buf, UDP_GUARD_BUFFER_LENGTH );   // rcvr end
      Nwrite( fd, buf, UDP_GUARD_BUFFER_LENGTH );   // rcvr end
      Nwrite( fd, buf, UDP_GUARD_BUFFER_LENGTH );   // rcvr end
      Nwrite( fd, buf, UDP_GUARD_BUFFER_LENGTH );   // rcvr end
   }

   realt = ((double )tFinish - (double )tStart)/1000;

   fprintf(stdout,
      "pcattcp%s: %.0f bytes in %.2f real seconds = %s/sec +++\n",
      g_bTransmit ? "-t" : "-r",
      nbytes, realt, outfmt(nbytes/realt));

   printf( "numCalls: %d; msec/call: %.2f; calls/sec: %.2f\n",
      numCalls,
      1024.0 * realt/((double )numCalls),
      ((double )numCalls)/realt
      );

   WSACleanup();

   return( 0 );
}

void
err( char *s )
{
	fprintf(stderr,"pcattcp%s: ", g_bTransmit ? "-t" : "-r");
	perror(s);
	fprintf(stderr,"errno=%d\n",errno);
   WSACleanup();
	exit(1);
}

void
mes( char *s )
{
	fprintf(stderr,"pcattcp%s: %s\n", g_bTransmit ? "-t" : "-r", s );
}

// Fill Buffer With Printable Characters...
void KS_FillPattern( char *cp, int cnt )
{
   UCHAR PBPreamble[] = "PCAUSA PCATTCP Pattern";   // 22 Bytes
   register char c;

   c = 0;

   //
   // Insert "PCAUSA Pattern" Preamble
   //
   if( cnt > 22 )
   {
      memcpy( cp, PBPreamble, 22 );
      cp += 22;
      cnt -= 22;
   }

   while( cnt-- > 0 )
   {
      while( !isprint((c&0x7F)) )
      {
         c++;
      }

      *cp++ = (c++&0x7F);
   }
}

/*
 *			N R E A D
 */
int Nread( SOCKET fd, PVOID buf, int count )
{
   struct sockaddr_in from;
   int len = sizeof(from);
   register int cnt;

   if( g_Protocol == IPPROTO_UDP )
   {
		cnt = recvfrom( fd, buf, count, 0, (struct sockaddr * )&from, &len );
		numCalls++;
   }
   else
   {
		if( b_flag )
      {
         cnt = mread( fd, buf, count );	/* fill buf */
      }
      else
      {
         cnt = recv( fd, buf, count, 0 );

         if( cnt == SOCKET_ERROR )
         {
            int nError = WSAGetLastError();
         }

         numCalls++;
      }

		if (touchdata && cnt > 0)
      {
			register int c = cnt, sum;
			register char *b = buf;
         sum = 0;
			while (c--)
				sum += *b++;
		}
	}

	return(cnt);
}

/*
 *			N W R I T E
 */
int Nwrite( SOCKET fd, PVOID buf, int count )
{
   register int cnt;

   if( g_Protocol == IPPROTO_UDP )
   {
again:
      cnt = sendto( fd, buf, count, 0,
               (struct sockaddr * )&sinhim,
               sizeof(sinhim)
               );

      numCalls++;

      if( cnt == SOCKET_ERROR && WSAGetLastError() == WSAENOBUFS )
      {
         delay(18000);
         errno = 0;
         goto again;
      }
   }
   else
   {
      cnt = send( fd, buf, count, 0 );
      numCalls++;
   }

   return(cnt);
}

void delay( int us )
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = us;
	select( 1, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tv );
}

/*
 *			M R E A D
 *
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This can be necessary because
 * network connections don't deliver data with the same
 * grouping as it is written with.  Written by Robert S. Miles, BRL.
 */
int mread( SOCKET fd, char *bufp, unsigned n)
{
   register unsigned	count = 0;
   register int		nread;

   do
   {
		nread = recv(fd, bufp, n-count, 0);
		numCalls++;
		if(nread < 0)  {
			perror("ttcp_mread");
			return(-1);
		}
		if(nread == 0)
			return((int)count);
		count += (unsigned)nread;
		bufp += nread;
	 } while(count < n);

	return((int)count);
}

#define END(x)	{while(*x) x++;}

void psecs( long l, char *cp )
{
	register int i;

	i = l / 3600;
	if (i) {
		sprintf(cp,"%d:", i);
		END(cp);
		i = l % 3600;
		sprintf(cp,"%d%d", (i/60) / 10, (i/60) % 10);
		END(cp);
	} else {
		i = l;
		sprintf(cp,"%d", i / 60);
		END(cp);
	}
	i %= 60;
	*cp++ = ':';
	sprintf(cp,"%d%d", i / 10, i % 10);
}

void prep_timer( VOID )
{
   tStart = GetTickCount();
}

char *outfmt( double b )
{
   static char obuf[50];

   switch (fmt) {
	case 'G':
	    sprintf(obuf, "%.2f GB", b / 1024.0 / 1024.0 / 1024.0);
	    break;
	default:
	case 'K':
	    sprintf(obuf, "%.2f KB", b / 1024.0);
	    break;
	case 'M':
	    sprintf(obuf, "%.2f MB", b / 1024.0 / 1024.0);
	    break;
	case 'g':
	    sprintf(obuf, "%.2f Gbit", b * 8.0 / 1024.0 / 1024.0 / 1024.0);
	    break;
	case 'k':
	    sprintf(obuf, "%.2f Kbit", b * 8.0 / 1024.0);
	    break;
	case 'm':
	    sprintf(obuf, "%.2f Mbit", b * 8.0 / 1024.0 / 1024.0);
	    break;
    }

    return obuf;
}

