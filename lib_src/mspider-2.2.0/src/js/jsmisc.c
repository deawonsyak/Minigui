#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser.h"
#include "interface.h"
#include "nav.h"
#include "spidermonkey.h"
#include "jsmisc.h"
#include "mgdconfig.h"

#ifdef JS_SUPPORT

int get_props_index(const char *tab[], int len, const char *name)
{
	int i;

	if ( !tab || !name ) {
		return -1;
	}

	for ( i=0; i<len; i++ ) {
		if ( strcmp(tab[i], name) == 0 ) {
			return i;
		}
	}	

	return -1;
}

void *gethtmldoc(jsobject *jsobj)
{
	while ( jsobj && 
		(jsobj->jstype != jstabwindow) && (jsobj->jstype != jswindow)) {
		jsobj = jsobj->jsparent;
	}

	if ( jsobj ) {
		return jsobj->htmlobj;
	}

	return NULL;
}


int get_jsobj_disabled(jsobject *jsobj)
{
	DwWidget *dw = NULL;
	DwMgWidget *mgdw = NULL;

	dw = (DwWidget*)jsobj->htmlobj;
	if ( !dw ) {
		return -1;
	}
	mgdw = (DwMgWidget*)dw;
	if ( !mgdw->window ) {
		return -1;
	}
	if ( IsWindowEnabled(mgdw->window) )	{
		/* enable */
		return 0;
	} else {
		return 1;
	}

	return -1;
}

void set_jsobj_disabled(int boolean, jsobject *jsobj)
{
	
	DwWidget *dw = NULL;
	DwMgWidget *mgdw = NULL;

	dw = (DwWidget*)jsobj->htmlobj;
	if ( !dw ) {
		return; 
	}
	mgdw = (DwMgWidget*)dw;
	if ( !mgdw->window ) {
		return;
	}
	if ( boolean ) {
		EnableWindow(mgdw->window, FALSE);
	} else {
		EnableWindow(mgdw->window, TRUE);
	}

    p_Dw_widget_queue_draw (dw);
}

void get_jsobj_props(const char *tab[], int len, jsobject *jsobj,char *jsobjtab,char *attr)
{
	int i;
	//char *attr = NULL;

	if ( !jsobj || !tab ) {
		return;
	}

	for ( i=0; i<len; i++ ) {
		if (strcmp(tab[i],jsobjtab)==0) {
            if(attr!=NULL)
                free(attr);
			attr=jsobj->jsprops[i];
            printf("the attr is %s,the number is %d\n",attr,atoi(attr));
            break;
		}
	}
}

void set_jsobj_props(const char *tab[], int len, jsobject *jsobj,char *jsobjtab,char *setvalue)
{
	int i;

	if ( !jsobj || !tab ) {
		return;
	}

	for ( i=0; i<len; i++ ) {
		if (strcmp(tab[i],jsobjtab)==0)
        {
            if(jsobj->jsprops[i]!=NULL)
                free(jsobj->jsprops[i]);
			jsobj->jsprops[i]=strdup(setvalue);
            break;
		}
	}
}
#endif
