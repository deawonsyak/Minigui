/* 
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef _JS_H_
#define _JS_H_

#include "mgdconfig.h"
#ifdef    JS_SUPPORT

typedef    struct _jsobject    jsobject;
typedef struct _jsscript_interpreter    jsscript_interpreter;
typedef struct _jsstring    jsstring;
typedef struct _tpara tpara;
enum tptype {
    once = 0,
    conti
};
/* javascript object's type */
enum jstype {
    jstnone = 0,
    jsmainwindow,
    jstabwindow,
    jswindow,
    jsdocument,
    jsanchor,
    jsimage,
    jsform,
    jsbutton,
    jssubmit,
    jsreset,
    jsselect,
    jsoption,
    jstext,
    jspassword,
    jscheckbox,
    jsradio,
    jshidden,
    jstextarea
};

struct _jsobject {
    /* a pointer to dwwidget/browserwindow */
    void *htmlobj;

    int  jstype;
    void *jsparent;
    char *jsname;
    char *jsid;
    char *jssrc;
    /* children */
    void *children;    /* GList */
    /* private var */
    void *pvar1;
    void *pvar2;
    void *pvar3; 
    /* Common JavaScript */
    char *commonjs;
    char *onload;
    int is_onload;
    char *onunload;
    /* other propertys */
    char *jsprops[0x40];
    /* js interpreter, only page */
    void *jsinterpreter;
    /* JSObject (SpiderMonkey object) */
    void *smobj;
    /* old window proc */
    void *oldproc;
    /* for forms&elements */
    void *FormID;
    void *InputID;
};

struct _jsscript_interpreter {
    int     inuse;
    void   *backend_data;
    void   *jsobj;    /* document's jsobj */
    void   *thisobj;    /* jsobject for this.xxx */
    /* other fields */
    int     valid;
    int     dealing;
    void     *point;
};

struct _jsstring {
    char   *source;
    int        length;
};

struct _tpara {
    int type;
    int canceled;
    void *ctx;
    void *jsobj;
    char *jsscript;
    jsscript_interpreter **interpreter;
};

jsobject*    js_getdftdocobj(int key);
void         js_setdftdocobj(int key, void *jsobj);
void         js_rmdftdocobj(void *jsobj);

jsobject*    js_findobjbyhwnd(HWND hwnd, jsobject *root);
jsobject*    js_findobjbyname(const char* name, jsobject *root);
jsobject*    js_findobjbyid(const char* id, jsobject *root);

jsobject*    js_objnew(void *htmlobj, int jstype, void* jsparent,
                        void *html, const char *tag, int tagsize);
void         js_objfree(jsobject* jsobj);
void         js_cleartimers (jsobject * obj);

void         js_eventprocess(HWND hwnd, int id, int rc, DWORD add_data);
void         js_onformevent(jsobject *formobj, int event);

void         js_init(void);
void         js_done(void);

void*        js_get_interpreter(jsscript_interpreter *interpreter, void *wjsobj);
void         js_put_interpreter(jsscript_interpreter *interpreter, int mod);

int          js_eval(jsscript_interpreter *interpreter, jsstring *code);

#endif    /* JS_SUPPORT */

#endif    /* _JS_H_ */
