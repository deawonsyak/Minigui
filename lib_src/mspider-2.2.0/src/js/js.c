#include <minigui/common.h>
#include "spidermonkey.h"
#include "html.h"
#include "browser.h"
#include "mgwidget.h"
#include "jsmisc.h"
#include "js.h"
#include "mgdconfig.h"

#ifdef JS_SUPPORT
extern  mSpiderDoc **mspider_doc;
extern  gint num_dd, num_dd_max;
extern const char *Html_get_attr(mSpiderHtml *html, const char *tag, 
								int tagsize, const char *attrname);
/* form */
extern const char*	form_propidx[];
extern const int 	form_propidxlen; 
/* text */
extern const char*	text_propidx[];
extern const int 	text_propidxlen; 
extern const char* 	gettexteventstr(jsobject *jsobj, int rc);
/* button */
extern const char*	button_propidx[];
extern const int 	button_propidxlen; 
extern const char* 	getbuttoneventstr(jsobject *jsobj, int rc);
/* image */
extern const char*	image_propidx[];
extern const int 	image_propidxlen; 
extern const char* 	getimageeventstr(jsobject *jsobj, int rc);
/* checkbox */
extern const char*	checkbox_propidx[];
extern const int 	checkbox_propidxlen; 
extern const char* 	getcheckboxeventstr(jsobject *jsobj, int rc);
/* radio */
extern const char*	radio_propidx[];
extern const int 	radio_propidxlen; 
extern const char*	getradioeventstr(jsobject *jsobj, int rc);
/* select */
extern const char*	select_propidx[];
extern const int 	select_propidxlen; 
extern const char* 	getselecteventstr(jsobject *jsobj, int rc);
/* option */
extern const char*	option_propidx[];
extern const int 	option_propidxlen; 
/* hidden */
extern const char*	hidden_propidx[];
extern const int 	hidden_propidxlen; 
/* textarea */
extern const char*	textarea_propidx[];
extern const int 	textarea_propidxlen; 
extern const char* 	gettextareaeventstr(jsobject *jsobj, int rc);
/* password */
extern const char*	password_propidx[];
extern const int 	password_propidxlen; 
extern const char* 	getpasswordeventstr(jsobject *jsobj, int rc);


static jsobject*	_js_findobjbyhwnd(HWND hwnd, jsobject* parent, jsobject *root);
static jsobject*	js_findobj(const char *str, int mode, jsobject *root);
static void 		get_obj_props(const char *tab[], int len, jsobject *jsobj,
							mSpiderHtml *html, const char *tag, int tagsize);
static int 			_js_ischild(jsobject *root, jsobject *children);
static int 			_js_ischild2(jsobject *root, jsobject *children);

typedef struct _objtab objtab;
struct _objtab {
	int 	key;
	void   *obj;
};
static GList *dftdoclist;
jsobject *js_getdftdocobj(int key)
{
	objtab *tab = NULL;
	GList *tmplist = NULL;

	tmplist = dftdoclist;
	while ( tmplist ) {
		if ( tmplist->data ) {
			tab = (objtab*)tmplist->data;
			if ( tab->key == key ) {
				return tab->obj;
			}
		}	
		tmplist = tmplist->next;
	}
	return NULL;
}

void js_setdftdocobj(int key, void *jsobj)
{
	objtab *item = NULL;

	item = g_malloc0(sizeof(objtab));
	if ( item ) {
		item->key = key;
		item->obj = jsobj;

		dftdoclist = g_list_append(dftdoclist, item);
	}
}

void js_rmdftdocobj(void *jsobj)
{
	objtab *tab = NULL;
	GList *tmplist = NULL;

	tmplist = dftdoclist;
	while ( tmplist ) {
		if ( tmplist->data ) {
			tab = (objtab*)tmplist->data;
			if ( tab->obj == jsobj) {
				dftdoclist = g_list_remove(dftdoclist, tab);
                g_free(tab);
			}
		}	
		tmplist = tmplist->next;
	}
}

static int _js_ischild(jsobject *root, jsobject *children)
{
	int i;
	GList *list = NULL;
	jsobject *tmpobj = NULL;
	jsobject *root2 = NULL;
	mSpiderDoc *doc = NULL;
	mSpiderDoc *tmpdoc = NULL;

	if ( !root || (root->jstype == jsmainwindow) ) {
		return 1;
	}
	if ( root == children ) {
		return 1;
	}
		
	if ( (root->jstype == jswindow) || (root->jstype == jstabwindow) ) {
		/* get cur doc */
		doc = (mSpiderDoc*)root->htmlobj;
		if ( doc ) {
			/* find all doc's child */
			for ( i=0; i< num_dd; i++ ) {
				tmpdoc = mspider_doc[i];
				while ( tmpdoc && (tmpdoc != doc) && (tmpdoc->parent != doc) ) {
					tmpdoc = tmpdoc->parent;
				}
				if ( tmpdoc ) {
					/* mspiderc_doc[i] is doc or doc's child */
					tmpdoc = mspider_doc[i];
					tmpobj = tmpdoc->jsobj;
					if ( tmpobj ) {
						if ( tmpobj == children ) {
							return 1;
						}
						list = tmpobj->children;
						if ( list ) { 
							root2 = list->data;
							if ( root2 ) {
								/* now root2 point to a jsdocument */
								if ( _js_ischild2(root2, children ) ) {
									return 1;
								}
							}
						}
					}
				}
			}
		}
	} else {
		/* < jsdocument */
		return _js_ischild2(root, children);
	} 
	
	return 0;
}

static int _js_ischild2(jsobject *root, jsobject *children)
{
	if ( !root ) {
		return 1;
	}
	if ( root == children ) {
		return 1;
	}

	while ( children && (children->jsparent != root) ) {
		children = children->jsparent;
	}
	if ( children ) {
		return 1;
	}

	return 0;
}

static jsobject *_js_findobjbyhwnd(HWND hwnd, jsobject* parent, jsobject *root)
{
	GList *list = NULL;
    jsobject *listobj = NULL;
	jsobject *retobj = NULL;

	if ( !parent ) {
		return NULL;
	}
    list = (GList*)parent->children;
    while ( list ) {
        listobj = (jsobject*)list->data;
		if ( listobj ) {
			if ( listobj->htmlobj ) {
				if ( ((DwMgWidget*)listobj->htmlobj)->window == hwnd ) {
					if ( _js_ischild(root, listobj) ) {
						return listobj;
					}
				}
			}
			if ( listobj->children ) {
				retobj = _js_findobjbyhwnd(hwnd, listobj, root);
				if ( retobj ) {
					return retobj;
				}
			}
		}
		list = list->next;
	}
	return NULL;
}

jsobject *js_findobjbyhwnd(HWND hwnd, jsobject *root)
{
	int i;
	jsobject *retobj = NULL;
	jsobject *jsobj = NULL;

	if ( !hwnd ) {
		return NULL;
	}

	for ( i=0; i< num_dd; i++ ) {
		jsobj = mspider_doc[i]->jsobj;
		if ( jsobj ) {
			retobj = _js_findobjbyhwnd(hwnd, jsobj, root);
			if ( retobj ) {
				return retobj;
			}
		}
	}	
	return NULL;
}

static jsobject *_js_findobj(const char *str, jsobject* parent, int mode, jsobject *root)
{
	GList *list = NULL;
    jsobject *listobj = NULL;
	jsobject *retobj = NULL;

	if ( !parent ) {
		return NULL;
	}
    list = (GList*)parent->children;
    while ( list ) {
        listobj = (jsobject*)list->data;
		if ( listobj ) {
			if ( mode == 0 ) {	/* by name */
				if ( listobj->jsname ) {
					if ( strcmp(listobj->jsname, str) == 0) {
						if ( _js_ischild(root, listobj) ) {
							return listobj;
						}
					}
				}
			} else if ( mode == 1 ) {	/* by id */
				if ( listobj->jsid) {
					if ( strcmp(listobj->jsid, str) == 0) {
						if ( _js_ischild(root, listobj) ) {
							return listobj;
						}
					}
				}
			}
			if ( listobj->children ) {
				retobj = _js_findobj(str, listobj, mode, root);
				if ( retobj ) {
					return retobj;
				}
			}
		}
		list = list->next;
	}
	return NULL;
}

/*
 *	mode:
 *
 *	0, 	find by name
 *	1,	find by id
 */
static jsobject *js_findobj(const char *str, int mode, jsobject *root)	
{
	int i;
	jsobject *jsobj = NULL;
	jsobject *retobj = NULL;

	if ( !str || !strlen(str) ) {
		return NULL;
	}

	for ( i=0; i< num_dd; i++ ) {
		jsobj = mspider_doc[i]->jsobj;
		/* for mSpiderDoc name*/
		if ( mode == 0 && mspider_doc[i]->name ) {
			if ( strcmp(mspider_doc[i]->name, str) == 0 ) {
				return jsobj;
			}
		}
		if ( jsobj ) {
			retobj = _js_findobj(str, jsobj, mode, root);
			if ( retobj ) {
				return retobj;
			}
		}
	}	
	return NULL;
}

jsobject *js_findobjbyname(const char *name, jsobject *root)
{
	return js_findobj(name, 0, root);
}

jsobject *js_findobjbyid(const char *id, jsobject *root)
{
	return js_findobj(id, 1, root);
}

jsobject* js_objnew(void *htmlobj, int jstype, void* jsparent,
					void *html, const char *tag, int tagsize)
{
	mSpiderHtml *dhtml = NULL;
	const char *attr = NULL;
	jsobject *jsobj = NULL;
	if ( (jstype == jstnone) || !jsparent ) {
		return NULL;
	}

	if ( !html || !tag || (tagsize == 0) ) {
		return NULL;
	}

	dhtml = (mSpiderHtml*)html;
	jsobj =	g_malloc0(sizeof(jsobject));
	if ( !jsobj ) {
		return NULL;
	}

	jsobj->htmlobj = htmlobj;
	jsobj->jstype = jstype;
	jsobj->jsparent = jsparent;
	attr = Html_get_attr(dhtml, tag, tagsize, "name");
	if ( attr ) {
		jsobj->jsname = g_strdup((char*)attr);
	}else
		jsobj->jsname = g_strdup((char*)"");

	attr = Html_get_attr(dhtml, tag, tagsize, "id");
	if ( attr ) {
		jsobj->jsid = g_strdup((char*)attr);
	}
	attr = Html_get_attr(dhtml, tag, tagsize, "src");
	if ( attr ) {
		jsobj->jssrc = g_strdup((char*)attr);
	}

	switch ( jstype ) {
	case jsform:
		get_obj_props(form_propidx, form_propidxlen, jsobj,
						dhtml, tag, tagsize);
		break;
	case jstext:
		get_obj_props(text_propidx, text_propidxlen, jsobj,
						dhtml, tag, tagsize);
		break;
	case jssubmit:
	case jsreset:
	case jsbutton:
		get_obj_props(button_propidx, button_propidxlen, jsobj,
						dhtml, tag, tagsize);
		break;
	case jsimage:
		get_obj_props(image_propidx, image_propidxlen, jsobj,
						dhtml, tag, tagsize);
		break;
	case jscheckbox:
		get_obj_props(checkbox_propidx,checkbox_propidxlen, jsobj,
						dhtml, tag, tagsize);
		break;
	case jsradio:
		get_obj_props(radio_propidx,radio_propidxlen, jsobj,
						dhtml, tag, tagsize);
		break;
	case jsselect:
		get_obj_props(select_propidx, select_propidxlen, jsobj,
						dhtml, tag, tagsize);
		break;
	case jsoption:
		get_obj_props(option_propidx, option_propidxlen, jsobj,
						dhtml, tag, tagsize);
		break;
	case jshidden:
		get_obj_props(hidden_propidx, hidden_propidxlen, jsobj,
						dhtml, tag, tagsize);
		break;
	case jstextarea:
        get_obj_props(textarea_propidx, textarea_propidxlen, jsobj,
						dhtml, tag, tagsize);
        break;
	case jspassword:
        get_obj_props(password_propidx, password_propidxlen, jsobj,
						dhtml, tag, tagsize);
        break;

	default:
		break;
	}

	return jsobj;
}

void js_objfree(jsobject* jsobj)
{
	int i;
	GList *list = NULL;
	jsobject *tmpobj = NULL;

	if ( !jsobj ) {
		return;
	}

	/* recursive free children */
	list = jsobj->children;
	while ( list ) {
		tmpobj = (jsobject*)list->data;
		list = list->next;
		if ( tmpobj ) {
			js_objfree(tmpobj);
		}
	}

	if ( jsobj->jsparent && 
		((jsobject*)jsobj->jsparent)->children ) {
		((jsobject*)jsobj->jsparent)->children =
			g_list_remove((GList*)((jsobject*)jsobj->jsparent)->children, jsobj);
	}

	if ( jsobj->jsname ) g_free(jsobj->jsname);
	if ( jsobj->jsid ) g_free(jsobj->jsid);
	if ( jsobj->jssrc ) g_free(jsobj->jssrc);
	if ( jsobj->commonjs ) g_free(jsobj->commonjs);
	if ( jsobj->onload ) g_free(jsobj->onload);
	if ( jsobj->onunload ) g_free(jsobj->onunload);
	for ( i=0; i<0x40; i++ ) {
		if ( jsobj->jsprops[i] ) g_free(jsobj->jsprops[i]);
	}
    js_cleartimers (jsobj);
	/* other */
	if ( jsobj->jsinterpreter && (jsobj->jstype == jsdocument) ) {
		((jsscript_interpreter*)jsobj->jsinterpreter)->inuse = 0;
        if (((jsobject *)jsobj->jsparent)->htmlobj == mspider_doc[0]){ /*free all*/
            js_put_interpreter ((jsscript_interpreter*)jsobj->jsinterpreter, 1);
        } else {
            js_put_interpreter ((jsscript_interpreter*)jsobj->jsinterpreter, 0);
        }
        g_free (jsobj->jsinterpreter);
        jsobj->jsinterpreter = NULL;
	}

	js_rmdftdocobj(jsobj);
	g_free(jsobj);
}

static void get_obj_props(const char *tab[], int len, jsobject *jsobj,
							mSpiderHtml *html, const char *tag, int tagsize)
{
	int i;
	const char *attr = NULL;

	if ( !jsobj || !tab ) {
		return;
	}

	for ( i=0; i<len; i++ ) {
		attr = Html_get_attr(html, tag, tagsize, tab[i]);
		if ( attr ) {
			jsobj->jsprops[i] = g_strdup((char*)attr);
		}
	}
}

void js_eventprocess(HWND hwnd, int id, int rc, DWORD add_data)
{
	jsstring jscode;
	const char *jsstr = NULL;
	jsobject *jsobj = NULL;
	jsobject *tmpobj = NULL;
	jsscript_interpreter *jsinterpreter = NULL;

	jsobj =  js_findobjbyhwnd(hwnd, NULL);
	if ( !jsobj ) {
		return;
	}

	if ( (jsobj->jstype != jssubmit) && jsobj->oldproc ) {
		/* delayed exec the submit's oldproc */
		(*(NOTIFPROC)jsobj->oldproc)(hwnd, id, rc, add_data);
	}

	/* get interperter */
	tmpobj = jsobj;
	while ( tmpobj && (tmpobj->jstype != jsdocument) ) {
		tmpobj = (jsobject*)tmpobj->jsparent;
	}
	if ( !tmpobj ) {
		return;
	}
	jsinterpreter = tmpobj->jsinterpreter;
	if ( !jsinterpreter ) {
		return;
	}
#if 0
	/* firstly run the common script */
	if ( tmpobj->onload ) {
		jscode.source = tmpobj->onload;
		jscode.length = strlen(tmpobj->onload);
		if ( js_eval(jsinterpreter, &jscode) ) {
			g_free(tmpobj->onload);
			tmpobj->onload = NULL;
		}
	}
#endif
	/* get corresponding event's javascripts str */
	switch ( jsobj->jstype ) {
	case jstext:
		jsstr = gettexteventstr(jsobj, rc);
		break;
	case jssubmit:
	case jsreset:
	case jsbutton:
		jsstr = getbuttoneventstr(jsobj, rc);
		break;
	case jsimage:
		jsstr = getimageeventstr(jsobj, rc);
		break;
    case jscheckbox:
		jsstr = getcheckboxeventstr(jsobj, rc);
		break;
    case jsradio:
		jsstr = getradioeventstr(jsobj, rc);
		break;
	case jsselect:
		jsstr = getselecteventstr(jsobj, rc);
		break;
	case jstextarea:
		jsstr = gettextareaeventstr(jsobj, rc);
		break;
	case jspassword:
		jsstr = getpasswordeventstr(jsobj, rc);
		break;
    default:
		break;
	}

	/* run javascripts */
	if ( jsstr ) {
		jscode.source = (char*)jsstr;
		jscode.length = strlen(jsstr);
        jsinterpreter->thisobj = jsobj;
		js_eval(jsinterpreter, &jscode);
	}

	if ( (jsobj->jstype == jssubmit) && jsobj->oldproc ) {
		/* run submit's oldproc (submit) */
		(*(NOTIFPROC)jsobj->oldproc)(hwnd, id, rc, add_data);
	}
}

void js_onformevent(jsobject *formobj, int event)
{
	int idx;
	jsstring jscode;
	char *eventstr = NULL;
	jsobject *tmpobj = NULL;
	jsscript_interpreter *jsinterpreter = NULL;

	if ( !formobj ) {
		return;
	}

	if ( event == 1 ) {
		/* submit */
		idx = get_props_index(form_propidx, form_propidxlen, "onSubmit");
	} else if ( event == 0 ) {
		/* reset */
		idx = get_props_index(form_propidx, form_propidxlen, "onReset");
	} else {
		return;
	}
	if ( idx < 0) {
		return;
	}
	
	eventstr = formobj->jsprops[idx];
	if ( !eventstr ) {
		return;
	}

	/* get interperter */
	tmpobj = formobj;
	while ( tmpobj && (tmpobj->jstype != jsdocument) ) {
		tmpobj = (jsobject*)tmpobj->jsparent;
	}
	if ( !tmpobj ) {
		return;
	}
	jsinterpreter = tmpobj->jsinterpreter;
	if ( !jsinterpreter ) {
		return;
	}

	/* firstly run the common script */
	if ( tmpobj->onload ) {
		jscode.source = tmpobj->onload;
		jscode.length = strlen(tmpobj->onload);
		if ( js_eval(jsinterpreter, &jscode) ) {
			g_free(tmpobj->onload);
			tmpobj->onload = NULL;
		}
	}

	/* run javascripts */
	if ( eventstr ) {
		jscode.source = eventstr;
		jscode.length = strlen(eventstr);
		jsinterpreter->thisobj = formobj; 
		js_eval(jsinterpreter, &jscode);
	}
}
void js_init(void)
{
	spidermonkey_init();
}

void js_done(void)
{
	spidermonkey_done();
}

void *js_get_interpreter(jsscript_interpreter *interpreter, void *wjsobj)
{
	return spidermonkey_get_interpreter(interpreter, wjsobj);
}

void js_put_interpreter(jsscript_interpreter *interpreter, int mod)
{
	spidermonkey_put_interpreter(interpreter, mod);
}

int js_eval(jsscript_interpreter *interpreter, jsstring *code)
{
#if 0
	int ret;
	static long callc = 0;
	char *codestr = NULL;

	if ( code->length > 0) {
#if 0
		codestr	= g_strdup(code->source);
		if ( codestr ) {
			codestr[code->length] = 0;
			fprintf(stderr, "code len %d: %s\n", code->length, codestr);
			g_free(codestr);
		}
#endif

#if 0
		/* just a tmp method */
		if ( strstr(code->source, "return") == code->source ) {
			code->source += 6;
			code->length -= 6;
		}
#endif

		if ( strstr(code->source, "return") == NULL) {
			 return spidermonkey_eval(interpreter, code);
		}

		if ( strstr(code->source, "function") == NULL ) {
			codestr = g_malloc(code->length + 0x80);
			if ( codestr ) {
				snprintf(codestr, code->length+0x80, "%s%u%s%s%s%u%s", 
					"function mSpider_function_", callc, "() {", code->source, 
					"} mSpider_function_", callc, "();");
				callc ++;
				code->source = codestr;
				code->length = strlen(codestr);
		
				ret = spidermonkey_eval(interpreter, code);
				g_free(codestr);
				return ret;
			}
		}
	}
#endif
	return spidermonkey_eval(interpreter, code);
}

#endif
