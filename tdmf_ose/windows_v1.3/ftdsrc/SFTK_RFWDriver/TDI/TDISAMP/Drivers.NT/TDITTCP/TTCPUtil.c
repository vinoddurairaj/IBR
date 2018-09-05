/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"

#include "INetInc.h"
#include "TDITTCP.h"

// Copyright And Configuration Management ----------------------------------
//
//                   TDI Test (TTCP) Utilities - TTCPUtil.c
//
//                  PCAUSA TDI Client Samples For Windows NT
//
//      Copyright (c) 1999-2001 Printing Communications Associates, Inc.
//                                - PCAUSA -
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------



/////////////////////////////////////////////////////////////////////////////
//// TDITTCP_FillPatternBuffer
//
// Purpose
// Fill the specified buffer with a pattern.
//
// Parameters
//
// Return Value
// None.
//
// Remarks
// Sorry for the magic numbers (like "14"). Don't want to spend much time
// extending the Rtlxxx just to get recognizable pattern...
//

void TDITTCP_FillPatternBuffer( PUCHAR cp, int cnt )
{
   UCHAR c;
   UCHAR PBStartChar = ' ';   // Blank or Space Character
   UCHAR PBRestartChar = 128;
   UCHAR PBPreamble[] = "PCAUSA TDITTCP Pattern";   // 22 Bytes

   //
   // Insert "PCAUSA Pattern" Preamble
   //
   if( cnt > 22 )
   {
      RtlCopyMemory( cp, PBPreamble, 22 );
      cp += 22;
      cnt -= 22;
   }

   //
   // Now Fill The Rest With (Mostly) Printable Characters
   // ----------------------------------------------------
   // Should start with:
   //
   //   "PCAUSA Pattern !"#$%&'()*+,-./0123456789:;..."
   // 
   c = PBStartChar;
   while( cnt-- > 0 )
   {
      *cp++ = c++;

      if( c >= PBRestartChar )
      {
         c = PBStartChar;
      }
   }
}


#ifdef DBG

NTSTATUS
TDITTCP_DumpTestParams( PTDITTCP_TEST_PARAMS pTestParams )
{
   PUCHAR   pByte;

   KdPrint(( "  Test Number    : %d\n", pTestParams->m_nTestNumber ));

   if( pTestParams->m_bTransmit )
   {
      //
      // Dump Remote IP Address
      //
      pByte = (PUCHAR )&pTestParams->m_RemoteAddress.s_addr;

      KdPrint(( "  Remote Address : %d.%d.%d.%d\n",
         pByte[0], pByte[1], pByte[2], pByte[3]
         ));

      //
      // Dump Remote IP Port
      //
      KdPrint(( "  Port           : %d\n",
         KS_ntohs( pTestParams->m_Port )
         ));
   }
   else
   {
      //
      // Dump Server IP Port
      //
      KdPrint(( "  Port           : %d\n",
         KS_ntohs( pTestParams->m_Port )
         ));
   }


   KdPrint(( "  Buffer Size    : %d\n", pTestParams->m_nBufferSize ));

   if( pTestParams->m_bTransmit )
   {
      if( pTestParams->m_bUsePatternGenerator )
      {
         if( pTestParams->m_bSendContinuously )
         {
            KdPrint(( "  Buffers To Send: Continuous\n" ));
         }
         else
         {
            KdPrint(( "  Buffers To Send: %d\n", pTestParams->m_nNumBuffersToSend ));
         }
      }
      else
      {
         KdPrint(( "  Sending File   : NOT YET IMPLEMENTED!!!\n" ));
      }
   }

   return( STATUS_SUCCESS );
}

#endif // DBG
