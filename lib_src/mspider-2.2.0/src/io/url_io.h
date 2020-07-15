#ifndef __IO_URL_H__
#define __IO_URL_H__

#include "../chain.h"
#include "url.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Returns a file descriptor to read data from */
typedef gint (*UrlOpener_t) (const mSpiderUrl *url, void *data);

/*
 * Module functions
 */
ChainFunction_t a_Url_get_ccc_funct(const mSpiderUrl *Url);


/*
 * External functions
 */
extern void a_Http_freeall(void);
gint a_Http_init(void);
gint a_Http_proxy_auth(void);
void a_Http_set_proxy_passwd(gchar *str);
gchar *a_Http_make_query_str(const mSpiderUrl *url, gboolean use_proxy);

void a_Http_ccc (int Op, int Branch, int Dir, ChainLink *Info,
                 void *Data1, void *Data2);
void a_About_ccc(int Op, int Branch, int Dir, ChainLink *Info,
                 void *Data1, void *Data2);
void a_IO_ccc   (int Op, int Branch, int Dir, ChainLink *Info,
                 void *Data1, void *Data2);
void a_Dpi_ccc  (int Op, int Branch, int Dir, ChainLink *Info,
                 void *Data1, void *Data2);
void a_Dpi_bye_dpid(void);

#ifdef ENABLE_PTHREADS
void a_File_ccc  (int Op, int Branch, int Dir, ChainLink *Info,
                                void *Data1, void *Data2);
#endif 

#ifdef ENABLE_SSL
void a_Https_ccc(int Op, int Br,int Dir, ChainLink *Info, void *Data, void *ExtraData);
void a_Https_freeall(void);
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IO_URL_H__ */

