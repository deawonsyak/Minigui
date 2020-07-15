#ifndef __WEB_H__
#define __WEB_H__

#include <glib.h>

#include "dw_widget.h"
#include "cache.h"
#include "html.h"
#include "image.h"     /* for mSpiderImage */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Flag defines
 */
#define WEB_RootUrl  1
#define WEB_Image    2
#define WEB_Download 4   /* Half implemented... */
#define WEB_Frame    8 

typedef struct _mSpiderWeb mSpiderWeb;

struct _mSpiderWeb {
  mSpiderUrl *url;              /* Requested URL */
  mSpiderDoc *dd;
  gint flags;                 /* Additional info */

  mSpiderImage *Image;          /* For image urls [reference] */

  DwWidget* page;

  FILE *stream;               /* File stream for local saving */
  gint SavedBytes;            /* Also for local saving */
  gint refcount;
};


mSpiderWeb* a_Web_new (const mSpiderUrl* url);
gint a_Web_valid (mSpiderWeb *web);
//mSpiderWeb* a_Web_ref (mSpiderWeb *web);
void a_Web_free (mSpiderWeb*);
gint a_Web_dispatch_by_type (const char *Type, mSpiderWeb *web,
                                  CA_Callback_t *Call, void **Data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WEB_H__ */

