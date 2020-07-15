#ifndef __AUTH_H__
#define __AUTH_H__

#include <glib.h>
#include "url.h"
#include "doc.h"

void a_Auth_byrealm(GString *auth_realm,mSpiderUrl *NewUrl, mSpiderDoc *dd);
GString *a_Auth_byurl(mSpiderUrl *url);
void a_Auth_freeall(void);

#endif /* AUTH_H */

