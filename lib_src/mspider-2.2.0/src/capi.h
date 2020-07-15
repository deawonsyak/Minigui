#ifndef __CAPI_H__
#define __CAPI_H__

#include <glib.h>
#include "cache.h"
#include "web.h"

/*
 * Function prototypes
 */
gint a_Capi_open_url(mSpiderWeb *web, CA_Callback_t Call, void *CbData);
gint a_Capi_get_buf(const mSpiderUrl *Url, gchar **PBuf, gint *BufSize);
#if 0  
gint a_Capi_dpi_send_cmd(mSpiderWeb *web, void *bw, char *cmd, char *server,
                         gint flags);
gint a_Capi_url_uses_dpi(gchar *url_str, gchar **server_ptr);
#endif

#endif /* __CAPI_H__ */

