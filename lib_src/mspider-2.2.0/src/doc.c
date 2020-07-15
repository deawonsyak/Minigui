/* * File: doc.c *
 *
 * Copyright 2005-2006 Feynman Software
 * Copyright (C) 2003 Frank de Lange
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "dw_widget.h"
#include "dw_viewport.h"
#include "dw_container.h"
#include "doc.h"
#include "list.h"
#include "debug.h"
#include "cache.h"
#include "nav.h"
#include "interface.h"
#include <math.h> /* for rint() */
#include "progressbar.h"
#include "prefs.h"
#include "emspider.h"

#define DEBUG_EVENT  0
#define DEBUG_SIZE  10
#define DEBUG_ALLOC 10

/* #define DEBUG_LEVEL  0 */
#include "debug.h"

/*
 * Local Data
 */

/* mSpiderDoc holds everything pertaining to a single document */
mSpiderDoc **mspider_doc;
gint num_dd, num_dd_max;

/*
 * Initialize global data
 */
void a_Doc_init(void)
{
    num_dd = 0;
    num_dd_max = 32;
    mspider_doc = NULL;
}

void a_Doc_freeall(void)
{
    g_free(mspider_doc);
}


/* callbacks */


/* public functions */

/*
 * Stop all active connections for the document (except downloads)
 */
void a_Doc_stop(mSpiderDoc *dd)
{
    g_return_if_fail ( dd != NULL );
    DEBUG_MSG(3, "a_Doc_stop: hi!\n");

    a_Cache_destroy_delayed_url ();
    /* Remove root clients */
    while ( dd->NumRootClients ) {
        a_Cache_stop_client(dd->RootClients[0]);
        a_List_remove(dd->RootClients, 0, dd->NumRootClients);
    }
    /* Remove image clients */
    while ( dd->NumImageClients ) {
        a_Cache_stop_client(dd->ImageClients[0]);
        a_List_remove(dd->ImageClients, 0, dd->NumImageClients);
    }
}

/*
 * Empty RootClients, ImageClients and PageUrls lists and
 * reset progress bar data.
 */
void a_Doc_clean(mSpiderDoc *dd)
{
    g_return_if_fail ( dd != NULL );

    while ( dd->NumRootClients )
        a_List_remove(dd->RootClients, 0, dd->NumRootClients);

    while ( dd->NumImageClients )
        a_List_remove(dd->ImageClients, 0, dd->NumImageClients);

    while ( dd->NumPageUrls ) {
        a_Url_free(dd->PageUrls[0].Url);
        a_List_remove(dd->PageUrls, 0, dd->NumPageUrls);
    }

    /* Zero image-progressbar data */
    dd->NumImages = 0;
    dd->NumImagesGot = 0;

    /* Zero progressbar data and ready state */
    dd->progress = 0.0;
    dd->ready = TRUE;
}

/*
 * Remove the cache-client from the dd list
 * (client can be a image or a html page)
 */
void a_Doc_remove_client(mSpiderDoc *dd, gint ClientKey)
{
    gint i;
    gboolean Found = FALSE;

    g_return_if_fail ( dd != NULL );

    for ( i = 0; !Found && i < dd->NumRootClients; ++i)
        if ( dd->RootClients[i] == ClientKey ) {
            a_List_remove(dd->RootClients, i, dd->NumRootClients);
            Found = TRUE;
        }

    for ( i = 0; !Found && i < dd->NumImageClients; ++i)
        if ( dd->ImageClients[i] == ClientKey ) {
            a_List_remove(dd->ImageClients, i, dd->NumImageClients);
            dd->NumImagesGot++;
            Found = TRUE;
        }

}


/*
 * Remove the cache-client from the dd list
 * (client can be a image or a html page)
 */
void a_Doc_close_client(mSpiderDoc *dd, gint ClientKey)
{
    g_return_if_fail ( dd != NULL );

    a_Doc_remove_client(dd, ClientKey);
    a_Doc_progress_update(dd);
}

/*
 * update progress bar, set button sensitivity
 */
void
a_Doc_progress_update(mSpiderDoc *dd)
{
#if 0
    gchar progress[PBAR_L];

    g_return_if_fail ( dd != NULL );
   
    if(dd->bw->dd == dd) {
        //a_Interface_set_button_sens(dd->bw);
        g_snprintf(progress, PBAR_L, "%s%d of %d",
                 PBAR_ISTR(prefs.panel_size == 1),
                 dd->NumImagesGot, dd->NumImages);
        a_Progressbar_update(dd->bw->imgprogress, progress,
                         (dd->NumImagesGot == dd->NumImages) ? 0 : 1 );
        g_snprintf(progress, PBAR_L, "%s%.1f Kb",
                 PBAR_PSTR(prefs.panel_size == 1),
                 (float)dd->progress);
        a_Progressbar_update(dd->bw->progress, progress, (dd->ready) ? 0 : 1 );
    }
#endif
}


/*
 * Add a reference to the cache-client in the document's list.
 * This helps us keep track of which are active in the document so that it's
 * possible to abort them.
 * (Root: Flag, whether a Root URL or not)
 */
void a_Doc_add_client(mSpiderDoc *dd, gint Key, gint Root)
{
    gint nc;

    g_return_if_fail ( dd != NULL );

    if ( Root ) {
        nc = dd->NumRootClients;
        a_List_add(dd->RootClients, nc, dd->MaxRootClients);
        dd->RootClients[nc] = Key;
        dd->NumRootClients++;
    } else {
        nc = dd->NumImageClients;
        a_List_add(dd->ImageClients, nc, dd->MaxImageClients);
        dd->ImageClients[nc] = Key;
        dd->NumImageClients++;
        dd->NumImages++;
    }
    a_Doc_progress_update(dd);
}

/*
 * Add an URL to the document's list.
 * This helps us keep track of page requested URLs so that it's
 * possible to stop, abort and reload them.)
 *   Flags: Chosen from {DD_Root, DD_Image, DD_Download}
 */
void a_Doc_add_url(mSpiderDoc *dd, const mSpiderUrl *Url, gint Flags)
{
    gint nu, i;
    gboolean found = FALSE;

    g_return_if_fail ( dd != NULL && Url != NULL );

    nu = dd->NumPageUrls;
    for ( i = 0; i < nu; i++ ) {
        if ( !a_Url_cmp(Url, dd->PageUrls[i].Url) ) {
            found = TRUE;
            break;
        }
    }
    if ( !found ) {
        a_List_add(dd->PageUrls, nu, dd->MaxPageUrls);
        dd->PageUrls[nu].Url = a_Url_dup(Url);
        dd->PageUrls[nu].Flags = Flags;
        dd->NumPageUrls++;
    }
}

/*
 * set document title
 *
 * this will propagate to the current interface element
 *
 * currently just forwards to a_Tab_title_set (when tabs
 * are compiled in) or a_Interface_set_page_title (when
 * compiled without tab support)
 */
void
a_Doc_title_set(mSpiderDoc *dd, gchar *title)
{
    g_return_if_fail ( dd != NULL && title != NULL );

    /* only set title for top level document */
    if(dd->parent == NULL)
        a_Interface_set_page_title(dd, title);
}

/*
 * set document location
 *
 * this will propagate to the current interface element
 *
 * currently just forwards to a_Interface_set_location_text
 */
void
a_Doc_location_set(mSpiderDoc *dd, gchar *location)
{
    g_return_if_fail ( dd != NULL && location != NULL );

    /* only set location text if this is the current tab */
    if((dd->parent == NULL) && dd->bw->dd == dd) {
        a_Interface_set_location_text(dd, location);
    }
}

/*
 * destroy a document
 */
void a_Doc_destroy(mSpiderDoc *dd)
{
    gint i;
 
    g_return_if_fail ( dd != NULL );
   
    /* stop/abort open connections. */
    a_Doc_stop(dd);
   
    for (i = 0; i < num_dd; i++)
        if (mspider_doc[i] == dd) {
            mspider_doc[i] = mspider_doc[--num_dd];
            break;
        }
   
    /* free nav_stack and nav_expect stuff */
    a_Nav_free(dd);
    a_Dw_viewport_destroy(dd->viewport); 
    /* unref the style */
    if(dd->style)
        a_Dw_style_unref((DwStyle*)(dd->style));
   
    g_free(dd->RootClients);
    g_free(dd->ImageClients); 

    if(dd->name)
        g_free(dd->name);

    if (dd->frameset)
    {
        g_signal_emit_by_name (DW_WIDGET(dd->frameset),"destroy",0);
    }
  
    for (i = 0; i < dd->NumPageUrls; i++)
        a_Url_free(dd->PageUrls[i].Url);
    g_free(dd->PageUrls);

#ifdef JS_SUPPORT
	if (dd->jsobj) {
		js_objfree (dd->jsobj);
        dd->jsobj = NULL;
	}

    if (dd->pagetitle) {
        g_free (dd->pagetitle);
        dd->pagetitle = NULL;
    }
#endif
    g_free(dd);
}


/*
 * Get the mSpiderDoc which contains *docwin
 * returns NULL if dd not found
 */
mSpiderDoc *
a_Doc_get_by_docwin(HWND docwin)
{
    gint i;

    g_return_val_if_fail ( docwin != 0, NULL );

    for (i = 0; i < num_dd; i++)
        if (mspider_doc[i]->docwin == docwin)
            return mspider_doc[i];

    return NULL;
}

/*
 * set the dd's name (used for targeted links)
 */
void a_Doc_set_name(mSpiderDoc *dd, gchar *name)
{
    g_return_if_fail((dd != NULL) && (name != NULL));

    if (dd && name) {
        if (dd->name)
            g_free((gchar *)dd->name);
        dd->name = g_strdup(name);
    }
}

/*
 * Get a named mSpiderDoc
 *
 * First checks if name is one of the reserved names
 * ("_blank", "_self", "_parent" and "_top") and acts
 * accordingly if so. If not, it searches for a document
 * with the same root as the document which initiated the
 * search. If still not found, it searches for the first
 * document with the given name.
 *
 * returns NULL if no matching document found
 */
mSpiderDoc *
a_Doc_get_by_name(mSpiderDoc *dd, gchar *name)
{
    gint i, n;
    mSpiderDoc *parent, *document;
    RECT wrc;
    DWORD flags;

    g_return_val_if_fail((dd != NULL) && (name != NULL), NULL);

    parent = a_Doc_get_root(dd);
    document = NULL;

    /* first see if the target name is one of the reserved names,
     * if so act appropriately */
    if(!g_strcasecmp(name, "_blank")) 
    {
        flags = NEW_BW_TOOLBAR | NEW_BW_LOCATIONBAR | 
                        NEW_BW_STATUSBAR | NEW_BW_PROGRESSBAR | NEW_BW_MENUBAR;
        GetWindowRect (parent->bw->main_window , &wrc);

       document = a_Pop_Window (HWND_DESKTOP , name , flags,
                                 wrc.left, wrc.top, wrc.right - wrc.left, wrc.bottom - wrc.top, NULL);
    }
    else if(!g_strcasecmp(name, "_self"))
        document = dd;
    else if(!g_strcasecmp(name, "_parent"))
        document = a_Doc_get_parent(dd);
    else if(!g_strcasecmp(name, "_top"))
        document = a_Doc_get_root(dd);

    /* if previous search did not match, first try to find document with same parent, 
     * if not found try to find first matching named document
     */
    if(!document)
        for (n = 1; n >= 0; n--) {
            for (i = 0; i < num_dd; i++)
                if (mspider_doc[i]->name &&
                    (!g_strcasecmp(name, mspider_doc[i]->name)) &&
                    (n ? (a_Doc_get_root(mspider_doc[i]) == parent) : TRUE)) {
                    document = mspider_doc[i];
                    break;
                }
            if(document)
                break;
        }

    return document;
}

/*
 * Get the parent document
 * For unparented documents it just returns the document itself
 */
mSpiderDoc *
a_Doc_get_parent(mSpiderDoc *dd)
{
    mSpiderDoc *parent;

    g_return_val_if_fail(dd != NULL, NULL);

    parent = dd;
    if(parent->parent)
        parent = parent->parent;

    return parent;
}

/*
 * Get the root document
 * For unparented documents it just returns the document itself.
 */
mSpiderDoc *
a_Doc_get_root(mSpiderDoc *dd)
{
    mSpiderDoc *parent;

    g_return_val_if_fail(dd != NULL, NULL);

    parent = dd;
    while(parent->parent)
        parent = parent->parent;

    return parent;
}

/*
 * get all visible children (nested documents) for this document
 *
 * returns: pointer to GList containing all visible descendants of
 *        the document (including the document itself as the first
 *        item if it is visible. Frameset documents are not visible,
 *        so they are not included in the list).
 *
 * returned GList must be g_list_free()'d by caller
 */
GList *
a_Doc_get_visible_children(mSpiderDoc *dd)
{
    gint i;
    GList *children;

    g_return_val_if_fail(dd != NULL, NULL);

    children = NULL;

    /* first add 'self' to list */
    if(DW_WIDGET_VISIBLE_BY_HANDLE(dd->docwin))
        children = g_list_append(children, dd);
    for (i = 0; i < num_dd; i++)
        if (mspider_doc[i]->parent == dd)
            children = g_list_concat(children, 
                         a_Doc_get_visible_children(mspider_doc[i]));

    return children;
}

/*
 * Set parent
 */
void
a_Doc_set_parent(mSpiderDoc *dd, mSpiderDoc *parent)
{
    g_return_if_fail (dd != NULL && parent != NULL);
   
    dd->parent = parent;
    a_Doc_set_browserwindow(dd, parent->bw);
}

/*   
 * set browserwindow
 */
void a_Doc_set_browserwindow(mSpiderDoc *dd, BrowserWindow *bw)
{
    DwStyle style_attrs;
    DwStyleFont font;

    g_return_if_fail (dd != NULL && bw != NULL);

    /* set dd's current window */
    dd->bw = bw;

    /* create style for Dw */
    font.name = prefs.vw_fontname; /* must be defined */
    font.size = (int)(DOC_DEFAULT_FONT_SIZE * prefs.font_factor + 0.5);
    font.weight = 400;
    font.style = DW_STYLE_FONT_STYLE_NORMAL;
    font.charset = DEF_CHARSET;
   
    a_Dw_style_init_values (&style_attrs, dd->bw->main_window);

    style_attrs.font = a_Dw_style_font_new (&font);
    style_attrs.color =
        a_Dw_style_color_new (prefs.text_color, dd->bw->main_window);
    style_attrs.background_color =
        a_Dw_style_color_new (prefs.bg_color, dd->bw->main_window);
    dd->style = a_Dw_style_new (&style_attrs, dd->bw->main_window);
}

/*
 * Create a new mSpiderDoc
 * (the new document is stored in mspider_doc)
 */
mSpiderDoc *
a_Doc_new(void)
{
    mSpiderDoc *dd;
    dd = g_new0(mSpiderDoc, 1);
    a_List_add(mspider_doc, num_dd, num_dd_max);
    mspider_doc[num_dd++] = dd;


    dd->style = NULL;
    dd->parent = NULL;

    dd->frameset = NULL;
    dd->name = NULL;
    dd->status_is_link = 0;

    dd->docwin = 0;
    dd->mspider_id = (int)dd;

    /* initialize the rest of the bt's data. */
    dd->redirect_level = 0;
    dd->RootClients = NULL;
    dd->NumRootClients = 0;
    dd->MaxRootClients = 16;

    dd->ImageClients = NULL;
    dd->NumImageClients = 0;
    dd->MaxImageClients = 16;
    dd->NumImages = 0;
    dd->NumImagesGot = 0;

    dd->PageUrls = NULL;
    dd->NumPageUrls = 0;
    dd->MaxPageUrls = 16;

    dd->auth_await_url = NULL;

    dd->CursorType = -1;

    dd->progress = 0.0;
    dd->ready = TRUE;

    dd->is_iframe = 0;

    a_Nav_init(dd);

#ifdef JS_SUPPORT
    if ( dd ) {
        dd->jsobj = g_malloc0(sizeof(jsobject));
        if ( dd->jsobj ) {
            dd->jsobj->htmlobj = dd;
            dd->jsobj->jstype = jswindow;
        }
        dd->jsinterpreter = NULL;
        dd->pagetitle = NULL;
    }
#endif

    return dd;
}

/* Two step to create a window functions*/
gboolean a_Doc_CreateEx(mSpiderDoc * dd, 
             HWND parent_hwnd, guint flags, gint x, gint y,
             gint cx, gint cy, gint ctrlId, gchar * startpage)
{
    DwViewport * viewport;

    dd->docwin = CreateWindowEx (CTRL_MSPIDER,
                            startpage,
                            flags,
                            WS_EX_CLIPCHILDREN,
                            ctrlId, 
                            x, y, 
                            cx, cy, parent_hwnd, 0);

    if (dd->docwin == HWND_INVALID) {
        printf ("emSpider: Can not create emSpider Control.\n");
        return FALSE;
    }

    viewport = a_Dw_viewport_new (dd->docwin);
    dd->viewport = viewport;

    SetWindowAdditionalData2(dd->docwin, (DWORD)dd);

    return TRUE;
}


char* correcturl(mSpiderDoc *dd, const char *url)
{
	mSpiderUrl *baseurl = NULL;
	mSpiderUrl *nurl = NULL;
	mSpiderHtmlLB *lb = NULL;
	static char buf[0x200];

	if ( dd ) {
		lb = dd->html_block;
		if ( lb ) {
			baseurl = lb->base_url;
			if ( baseurl ) {
				nurl = a_Url_new(url, baseurl->url_string->str, 0, 0, 0);
				if ( nurl ) {
                    memset (buf, 0 , 0x200);
					strncpy(buf, nurl->url_string->str, 0x1FF);
					a_Url_free(nurl);
					return buf;
				}
			} else {
				return (char*)url;
			}
		}
	}
	return NULL;
}



