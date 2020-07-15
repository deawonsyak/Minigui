#ifndef _JSMISC_H_
#define _JSMISC_H_

#ifdef JS_SUPPORT

#include <minigui/common.h>
#include "spidermonkey.h"
#include "html.h"
#include "browser.h"
#include "mgwidget.h"

int 	get_props_index(const char *tab[], int len, const char *name);
void*	gethtmldoc(jsobject *jsobj);
char* 	correcturl(mSpiderDoc *dd, const char *url);

void set_jsobj_disabled(int boolean, jsobject *jsobj);
int get_jsobj_disabled(jsobject *jsobj);
void get_jsobj_props(const char *tab[], int len, jsobject *jsobj,char *jsobjtab,char *attr);
void set_jsobj_props(const char *tab[], int len, jsobject *jsobj,char *jsobjtab,char *setvalue);
#endif

#endif /* _JSMISC_H_ */
