#ifndef __DBIO_h
#define __DBIO_h

#include <mgdconfig.h>

#ifdef ENABLE_SSL

#include <openssl/bio.h>

#include "io.h"

/*
 * Exported functions
 */
IOData_t* a_DBIO_new(BIO *bio);
void a_DBIO_ccc(int Op, int Br,int Dir, ChainLink *Info, void *Data, void *ExtraData);

#endif /* ENABLE_SSL */

#endif /* __DBIO_h */


