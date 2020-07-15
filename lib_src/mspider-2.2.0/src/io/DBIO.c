/*
 * File: DBIO.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 2000, 2001 Jorge Arellano Cid <jcid@inf.utfsm.cl>
 * Copyright (C) 2002       Jonathan P Springer <jonathan.springer@verizon.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * In addition, as a special exception, Jorge Arellano Cid and Jonathan
 * Springer give permission to link the code of this program with the OpenSSL
 * library (or modified versions of OpenSSL that use the same license as
 * OpenSSL), and distribute linked combinations including the two.  You must
 * obey the GNU General Public License in all respects for all of the code
 * used other than OpenSSL.  If you modify this file, you may extend this
 * exception to your version of the file, but you are not obligated to do so.
 * If you do not wish to do so, delete this exception from your version.  
 *
 */

/*
 * mSpider's signal driven BIO engine
 */

#include <mgdconfig.h>

#ifdef ENABLE_SSL 

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <glib.h>
/*#include <gdk/gdk.h>*/
#include "../chain.h"
#include "DBIO.h"

#include <openssl/bio.h>
#include <openssl/err.h>

//#define DEBUG_LEVEL 3
#include "../debug.h"


/*
 * Symbolic defines for shutdown() function
 * (Not defined in the same header file, for all distros --Jcid)
 */
#define IO_StopRd   0
#define IO_StopWr   1
#define IO_StopRdWr 2

extern void IO_submit(IOData_t *r_io);
extern void IO_del(IOData_t *io);
extern void IO_ins(IOData_t *io);
extern IOData_t *IO_get(gint Key);

/*
 * IO-module data
 */


/*
 * Return a newly created, and initialized, 'io' struct
 */
IOData_t *a_DBIO_new(BIO *bioin)
{
   IOData_t *io = g_new0(IOData_t, 1);
   io->GioCh = NULL;
   io->FD = -1;
   io->Flags = 0;
   io->bio = bioin;
   return io;
}

/*
 * Free an 'io' struct
 */
void DBIO_free(IOData_t *io)
{
  if (io->Flags & IOFlag_FreeIOBuf) g_free(io->Buf);
  g_free(io);
}

/*
 * Close an open BIO, and remove io controls.
 * (This function can be used for Close and Abort operations)
 */
void DBIO_close_fd(IOData_t *io, gint CloseCode)
{
  if (io->bio) BIO_free_all(io->bio);

  IO_del(io);
}

/*
 * Abort an open FD.
 *  This function is called to abort a BIO connection due to an IO error
 *  or just because the connection is not required anymore.
 */
gboolean DBIO_abort(IOData_t *io)
{
   /* Close and finish this FD's activity */
   DBIO_close_fd(io, IO_StopRdWr);

   return FALSE;
}

/*
 * Read data from a BIO into a specific buffer
 */
gboolean DBIO_Callback_Read(gpointer data)
{
  ssize_t St;
  gboolean ret;
  IOData_t *io = (IOData_t *) data;

  DEBUG_MSG(3, "  IO_read2\n");

  /* Sometimes glib delivers events on already aborted FDs  --Jcid */
  if ( IO_get(io->Key) == NULL ) return FALSE;

  St = BIO_read(io->bio, io->Buf, io->BufSize);

  if ( St < 0 ) {
    if (BIO_should_retry(io->bio)) {
      ret = TRUE;
    } else {
      io->Status = ERR_get_error();
      ret = FALSE;
    }
  } else if ( St == 0 ) {
    if (BIO_should_retry(io->bio)) {
      ret = TRUE;
    } else {
      a_DBIO_ccc(OpEnd, 2,0, io->Info, io, NULL);
      ret = FALSE;
    }
  } else {
    io->Status = St;  /* Status is used for length */
    a_DBIO_ccc(OpSend, 2,0, io->Info, io, NULL);
    ret = TRUE;
  }

  return ret;
}

/*
 * Write data, from a specific buffer, into a file descriptor
 * (** Write operations MUST NOT free the buffer because the buffer
 *     start is modified.)
 * todo: Implement IOWrites, remove the constraint stated above.
 */
gboolean DBIO_Callback_Write(gpointer data)
{
  ssize_t St;
  gboolean ret = FALSE;
  IOData_t *io = (IOData_t *) data;

  DEBUG_MSG(3, "  IO_write\n");

  /* Sometimes glib delivers events on already aborted FDs  --Jcid */
  if ( IO_get(io->Key) == NULL ) return FALSE;

  St = BIO_write(io->bio, io->Buf, io->BufSize);
  io->Status = St;

  DEBUG_MSG(3, "  IO_write: %s [errno %d] [St %d]\n",
	    g_strerror(errno), errno, St);

  if ( St <= 0 ) {
    if (BIO_should_retry(io->bio)) {
      ret = TRUE;
    } else {
      io->Status = ERR_get_error();
      ret = FALSE;
    }
  } else if ( St < io->BufSize ){
    // Not all data written
    io->BufSize  -= St;
    io->Buf = ((gchar *)io->Buf) + St;
    ret = TRUE;
  } else {
    // All data in buffer written
    if ( io->Op == IOWrite ) {
      /* Single write */
      a_DBIO_ccc(OpEnd, 1,0, io->Info, io, NULL);
      ret = FALSE;
    } else if ( io->Op == IOWrites ) {
      /* todo: Writing in small chunks (not implemented) */
    }
  }

  return ret;
}

/*
 * Receive an IO request (IORead | IOWrite | IOWrites),
 */
void DBIO_submit(IOData_t *r_io)
{
  if ( r_io->Op == IORead ) {
    g_idle_add_full(G_PRIORITY_HIGH_IDLE, DBIO_Callback_Read, (gpointer) r_io, NULL);
    //g_idle_add(DBIO_Callback_Read, (gpointer) r_io);
  } else if (r_io->Op == IOWrite || r_io->Op == IOWrites ) {
    g_idle_add_full(G_PRIORITY_HIGH_IDLE, DBIO_Callback_Write, (gpointer) r_io, NULL);
    //g_idle_add(DBIO_Callback_Write, (gpointer) r_io);
  }
  
  /* Add a reference pointer to this request */
  IO_ins(r_io);
}

/*
 * CCC function for the IO module
 * ( Data = IOData_t* ; ExtraData = NULL )
 */
void a_DBIO_ccc(int Op, int Branch,int Dir, ChainLink *Info, void *Data, void *ExtraData)
{
   IOData_t *io = Data;

   if ( Branch == 1 ) {
      /* Send query */
      switch (Op) {
      case OpStart:
         io->Info = Info;
         Info->LocalKey = io;
         DBIO_submit(io);
         break;
      case OpEnd:
         a_Chain_fcb(OpEnd, Info, io, NULL);
         DBIO_free(io);
         break;
      case OpAbort:
         a_Chain_fcb(OpAbort, Info, NULL, NULL);
         DBIO_free(io);
         break;
      }

   } else if ( Branch == 2 ) {
      /* Receive answer */
      switch (Op) {
      case OpStart:
         io->Info = Info;
         Info->LocalKey = io;
         a_Chain_link_new(Info, a_DBIO_ccc, FWD, a_Cache_ccc,2,2);
         a_Chain_fcb(OpStart, Info, io, io->ExtData);
         DBIO_submit(io);
         break;
      case OpSend:
         a_Chain_fcb(OpSend, Info, io, NULL);
         break;
      case OpEnd:
         a_Chain_fcb(OpEnd, Info, io, NULL);
         DBIO_free(io);
         break;
      case OpAbort:
         a_Chain_fcb(OpAbort, Info, io, NULL);
         DBIO_free(io);
         break;
      }

   } else if ( Branch == -1 ) {
      /* Backwards call */
      switch (Op) {
      case OpAbort:
         DEBUG_MSG(3, "IO   : OpAbort [-1]\n");
         io = Info->LocalKey;
         DBIO_abort(io);
         DBIO_free(io);
         g_free(Info);
         break;
      }
   }
}

#endif

