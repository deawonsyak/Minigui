#ifndef __MSG_H__
#define __MSG_H__

#include "prefs.h"

/*
 * You can disable any MSPIDER_MSG* macro by adding the '_' prefix.
 */
#ifndef WIN32
#define _MSG(fmt...)
#define _MSG_HTML(fmt...)
#define _MSG_HTTP(fmt...)

#define MSG(fmt...)                    \
   G_STMT_START {                      \
      if (prefs.show_msg)              \
         g_print(fmt);                 \
   } G_STMT_END

#define MSPIDER_MSG(fmt...)                    \
   G_STMT_START {                      \
      if (prefs.show_msg)              \
           printf (fmt);               \
   } G_STMT_END

#define MSG_HTML(fmt...)               \
   G_STMT_START {                      \
         Html_msg(html, fmt);          \
} G_STMT_END

#define MSG_HTTP(fmt...)  printf("HTTP warning: " fmt)
#else


static __inline void  _MSG(const char * fmt, ...)
{
	return;
}
static __inline void  _MSG_HTML(const char * fmt, ...)
{
	return;
}
static __inline void  _MSG_HTTP(const char * fmt, ...)
{
	return;
}

static __inline void MSPIDER_MSG(const char * fmt, ...)
{
	return;
}
static __inline void  MSG_HTML(const char * fmt, ...)
{
	return;
}
static __inline void MSG_HTTP(const char *format, ...)
{
	return;
}
#endif

#endif /* __MSG_H__ */

