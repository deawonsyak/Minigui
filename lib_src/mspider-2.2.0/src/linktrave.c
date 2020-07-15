/* * File: linktrave.c *
 *
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mgdconfig.h"

#ifdef ENABLE_LINKTRAVE

#include <string.h>
#include "list.h"
#include "linktrave.h"
#include "mgwidget.h"
#include "dw_image.h"
#include "dw_viewport.h"
#include "dw_ext_iterator.h"

static DwLinkIterator* a_Dw_link_iterator_new      (DwWidget *widget);
static gboolean        a_Dw_link_iterator_next     (DwLinkIterator *eit);
static gboolean        a_Dw_link_iterator_prev     (DwLinkIterator *eit);

static void            a_Dw_link_iterator_highlight      (DwLinkIterator *it,
                                                   gint start,
                                                   gint end,
                                                   DwHighlightLayer layer);

static void            a_Dw_link_iterator_scroll_to      (DwLinkIterator *it1,
                                                   DwLinkIterator *it2,
                                                   gint start,
                                                   gint end,
                                                   DwHPosition hpos,
                                                   DwVPosition vpos);

static DwLinkIterator* a_Dw_link_iterator_clone          (DwLinkIterator *it);
static void            a_Dw_link_iterator_free           (DwLinkIterator *it);

#define         a_Dw_link_iterator_unhighlight(it, l) \
                   a_Dw_link_iterator_highlight(it, -1, -1, l)


static gboolean Link_compare (DwLinkIterator * itr, gint no)
{
    if (itr->content.x_link == no)
        return TRUE;

    return FALSE;
}


LinktraveState* a_Linktrave_state_new (void)
{
    LinktraveState * ls;

    ls = g_new0 (LinktraveState, 1);
    ls->max_dd = 2;
    return ls;
}



void a_Linktrave_state_destroy (LinktraveState *state)
{
   if (state->iterator)
      a_Dw_link_iterator_free (state->iterator);
   if (state->hl_iterator)
      a_Dw_link_iterator_free (state->hl_iterator);
   g_free (state);
}


void a_Linktrave_state_set_widget (LinktraveState *state,
                                    DwWidget  *widget)
{
   state->widget = widget;
   if (state->iterator)
      a_Dw_link_iterator_free (state->iterator);
   state->iterator = NULL;
   if (state->hl_iterator)
      a_Dw_link_iterator_free (state->hl_iterator);
   state->hl_iterator = NULL;

   state->widget_change = TRUE;
}

static gboolean findnext_link (LinktraveState * state)
{
    int i;
   gboolean nextit;
   nextit = TRUE;

    if (state->link_no < 0)
        return FALSE;

   do { 
    if (Link_compare (state->iterator, state->link_no)) {
        state->num_word++;
        state->first_hl_start = 0;
        state->last_hl_end = 1;

        nextit = a_Dw_link_iterator_next (state->iterator);
        
        while (nextit && Link_compare (state->iterator, state->link_no)) {
            state->num_word++;
            state->last_hl_end++;

            nextit = a_Dw_link_iterator_next (state->iterator);
        }
        goto exit;
      }

         nextit = a_Dw_link_iterator_next (state->iterator);
     } while ( nextit);
        
        return FALSE;
exit:
        for (i = 0; i < state->num_word; i++)
         a_Dw_link_iterator_prev (state->iterator);

        return TRUE;
}

static gboolean redirect_iterator (DwLinkIterator * eit, DwWidget * w, int dir)
{
    mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (w->hwnd);
    DwViewport* viewport;
    LinktraveManage *mg;

    a_Dw_link_iterator_next (eit);
    eit->type =eit->content.type;

    if (dd && dd->is_iframe) {
        viewport = (DwViewport*)dd->viewport;
        mg = (LinktraveManage *)dd->bw->linktrave_manage;

        mg->stack_top++;
        mg->linktrave_state = a_Linktrave_state_new();
        a_List_add (mg->stack, mg->stack_top, mg->stack_max); 
        mg->stack[mg->stack_top] = mg->linktrave_state; 
        mg->linktrave_state->link_block = dd->html_block;

        if (dir == LTS_FORWARD)
            mg->link_number_focus = -2;
        else
            mg->link_number_focus = ((mSpiderHtmlLB*)dd->html_block)->num_links;
        
        a_Linktrave_state_set_widget (mg->linktrave_state, viewport->child);

    }

    return TRUE;
}

/*
*   Major function for link traversal
*/
gboolean a_Linktrave_step (LinktraveState *state, gint link_no, int direct)
{
   int i;

   if (state->widget == NULL)
      return FALSE;

   /* If there is still highlighted text */
   if (state->hl_iterator) {
      for (i = 0; i < state->num_word - 1; i++) {
         a_Dw_link_iterator_unhighlight (state->hl_iterator,
                                         DW_HIGHLIGHT_SELECTION);
         a_Dw_link_iterator_next (state->hl_iterator);
      }
      a_Dw_link_iterator_unhighlight (state->hl_iterator,
                                      DW_HIGHLIGHT_SELECTION);
      a_Dw_link_iterator_free (state->hl_iterator);
      state->hl_iterator = NULL;
   }

    if (state->iterator && state->iterator->type == DW_CONTENT_MGWIDGET) {
    
         redirect_iterator (state->iterator, state->iterator->content.data.widget, direct); 

        return TRUE;
    }
    /* key change FIXME: widget change!*/

    if ((link_no != state->link_no) || (state->widget_change)) {
      state->link_no = link_no;
      state->widget_change = FALSE;
      state->num_word = 0;

      if (state->iterator)
         a_Dw_link_iterator_free (state->iterator);
      state->iterator = a_Dw_link_iterator_new (state->widget);
      if (!state->iterator)
        return FALSE;
      a_Dw_link_iterator_next (state->iterator);
    }

    if (findnext_link (state)) {
      /* Highlighlighting is done with a clone. */
      state->hl_iterator = a_Dw_link_iterator_clone (state->iterator);

        if (state->num_word == 1) {
         a_Dw_link_iterator_scroll_to (state->iterator, state->iterator,
                                       state->first_hl_start,
                                       state->last_hl_end,
                                       DW_HPOS_INTO_VIEW, DW_VPOS_INTO_VIEW);
         a_Dw_link_iterator_highlight (state->hl_iterator,
                                       state->first_hl_start,
                                       state->last_hl_end,
                                       DW_HIGHLIGHT_SELECTION);
        } else {

         a_Dw_link_iterator_highlight (state->hl_iterator,
                                       state->first_hl_start,
                                       state->last_hl_end,
                                       DW_HIGHLIGHT_SELECTION);
         a_Dw_link_iterator_next (state->hl_iterator);
         for (i = 1; i < state->num_word - 1; i++) {
            a_Dw_link_iterator_highlight (state->hl_iterator, 0,
                                            state->last_hl_end,
                                          DW_HIGHLIGHT_SELECTION);
            a_Dw_link_iterator_next (state->hl_iterator);
         }
         a_Dw_link_iterator_highlight (state->hl_iterator, 0,
                                       state->last_hl_end,
                                       DW_HIGHLIGHT_SELECTION);

         a_Dw_link_iterator_scroll_to (state->iterator, state->iterator,
                                       state->first_hl_start,
                                       state->last_hl_end,
                                       DW_HPOS_INTO_VIEW, DW_VPOS_INTO_VIEW);
      }
      for (i = 0; i < state->num_word - 1; i++)
         a_Dw_link_iterator_prev (state->hl_iterator);

      /* The search will continue from the word after the found position. */
      a_Dw_link_iterator_next (state->iterator);

        
        return TRUE;
    }

    return FALSE;
}

void a_Linktrave_reset (LinktraveState *state)
{
   int i;

   if (state->hl_iterator) {
      for (i = 0; i < state->num_word - 1; i++) {
         a_Dw_link_iterator_unhighlight (state->hl_iterator,
                                         DW_HIGHLIGHT_SELECTION);
         a_Dw_link_iterator_next (state->hl_iterator);
      }
      a_Dw_link_iterator_unhighlight (state->hl_iterator,
                                      DW_HIGHLIGHT_SELECTION);
      a_Dw_link_iterator_free (state->hl_iterator);
      state->hl_iterator = NULL;
   }

}

static DwIterator *Dw_ext_iterator_search_downward (DwIterator *it,
                                                    gboolean from_end,
                                                    int indent)
{
   DwIterator *it2, *it3;


   g_return_val_if_fail (it->content.type == DW_CONTENT_WIDGET, NULL);
   it2 = a_Dw_widget_iterator (it->content.data.widget, it->mask, from_end);

   if (it2 == NULL) {
      /* Moving downwards failed. */
      return NULL;
   }

   while (from_end ? a_Dw_iterator_prev (it2) : a_Dw_iterator_next (it2)) {

      if (it2->content.type == DW_CONTENT_WIDGET) {
         /* Another widget. Search in it downwards. */
         it3 = Dw_ext_iterator_search_downward (it2, from_end, indent + 3);
         if (it3 != NULL) {
            a_Dw_iterator_free (it2);
            return it3;
         }
         /* Else continue in this widget. */
      } else {
         return it2;
      }
   }

   /* Nothing found. */
   a_Dw_iterator_free (it2);
   return NULL;
}

/*
 * Search sidewards. from_end specifies the direction, FALSE means forwards,
 * TRUE means backwards.
 * The pararameter indent is only for debugging purposes.
 */
static DwIterator *Dw_ext_iterator_search_sideward (DwIterator *it,
                                                    gboolean from_end,
                                                    int indent)
{
   DwIterator *it2, *it3;


   g_return_val_if_fail (it->content.type == DW_CONTENT_WIDGET, NULL);
   it2 = a_Dw_iterator_clone (it);

   while (from_end ? a_Dw_iterator_prev (it2) : a_Dw_iterator_next (it2)) {
      if (it2->content.type == DW_CONTENT_WIDGET) {
         /* Search downwards in this widget. */
         it3 = Dw_ext_iterator_search_downward (it2, from_end, indent + 3);
         if (it3 != NULL) {
            a_Dw_iterator_free (it2);
            return it3;
         }
         /* Else continue in this widget. */
      } else {
         /* Success! */
         return it2;
      }
   }

   /* Nothing found, go upwards in the tree (if possible). */
   a_Dw_iterator_free (it2);
   if (it->widget->parent) {
      it2 = a_Dw_widget_iterator (it->widget->parent, it->mask, FALSE);
      while (TRUE) {
         if (!a_Dw_iterator_next(it2)) {
            a_Dw_iterator_free (it2);
            return NULL;
         }
         if (it2->content.type == DW_CONTENT_WIDGET &&
             it2->content.data.widget == it->widget) {
            it3 = Dw_ext_iterator_search_sideward (it2, from_end, indent + 3);
            a_Dw_iterator_free (it2);
            return it3;
         }
      }
   }

   /* Nothing found at all. */
   return NULL;
}

static gboolean construct_iterator (DwLinkIterator* lit, DwIterator* it)
{
    DwIterator *it2;
    DwWidget *w;
    int sp;

    if (NULL == lit || NULL == it)
        return FALSE;

   if (it->content.type == DW_CONTENT_WIDGET && !DW_IS_IMAGE(it->content.data.widget)) {
      /* The second argument of Dw_ext_iterator_search_downward is
       * actually a matter of taste :-) */
      if ((it2 = Dw_ext_iterator_search_downward (it, FALSE, 3)) ||
          (it2 = Dw_ext_iterator_search_sideward (it, FALSE, 3)) ||
          (it2 = Dw_ext_iterator_search_sideward (it, TRUE, 3))) {
         a_Dw_iterator_free (it);
         it = it2;
      } else {
         a_Dw_iterator_free (it);
         return FALSE;
      }
   }

   lit->stack_top = 0;

    for (w = it->widget; w->parent != NULL; w = w->parent)
        lit->stack_top++;
    lit->stack_max = 4;
    while (lit->stack_top >= lit->stack_max)
        lit->stack_max <<= 1;
    lit->stack = g_new (DwIterator*, lit->stack_max);

    for (w = it->widget, sp = lit->stack_top -1; 
            w->parent != NULL; w = w->parent,sp--) {
      lit->stack[sp] = a_Dw_widget_iterator (w->parent, it->mask, FALSE);

      while (TRUE) {
         if (!a_Dw_iterator_next(lit->stack[sp])) {
            a_Dw_iterator_free (lit->stack[sp]);
            g_free (lit->stack);
            return FALSE;
         }
         if (lit->stack[sp]->content.type == DW_CONTENT_WIDGET &&
             lit->stack[sp]->content.data.widget == w)
            break;
      }
    }

   lit->stack[lit->stack_top] = it;
   lit->content = it->content;
   //lit->content.x_link = -1;
   lit->type = it->content.type;
    return TRUE;
}

static DwLinkIterator* a_Dw_link_iterator_new (DwWidget *widget)
{
    DwLinkIterator *it;
    DwIterator *it0;

    it = g_new (DwLinkIterator, 1);
    it0 = a_Dw_widget_iterator (widget, DW_CONTENT_TEXT | DW_CONTENT_WIDGET, FALSE);
   
    if (!construct_iterator (it, it0))
    {
        g_free (it);
        return NULL; 
    }
    
    return it;
}



static gboolean a_Dw_link_iterator_next (DwLinkIterator *eit)
{
    DwIterator *it = eit->stack[eit->stack_top]; 

    if (a_Dw_iterator_next(it)) {
        if (it->content.type == DW_CONTENT_WIDGET && DW_IS_MGWIDGET(it->content.data.widget)) { 
            eit->content = it->content;
            eit->type = DW_CONTENT_MGWIDGET;
            return TRUE;
        }
        else if (it->content.type == DW_CONTENT_WIDGET && !DW_IS_IMAGE(it->content.data.widget)) {

            eit->stack_top++;
            a_List_add (eit->stack, eit->stack_top, eit->stack_max);
            eit->stack[eit->stack_top] =
              a_Dw_widget_iterator (it->content.data.widget, it->mask, FALSE);
            return a_Dw_link_iterator_next (eit);
        } else {
            eit->content = it->content;
            eit->type = it->content.type;
            return TRUE;
        }
    } else {
        if (eit->stack_top > 0) {
            a_Dw_iterator_free (it);
            eit->stack_top--;
            return a_Dw_link_iterator_next (eit);
        } else {
             eit->content.type = DW_CONTENT_END;
             eit->type = it->content.type;
             eit->content.x_link = -1;
             return FALSE;
        }
    }
}

static gboolean a_Dw_link_iterator_prev (DwLinkIterator *eit)
{
    DwIterator *it = eit->stack[eit->stack_top]; 

    if (a_Dw_iterator_prev(it)) {
        if (it->content.type == DW_CONTENT_WIDGET && DW_IS_MGWIDGET(it->content.data.widget)) { 
            eit->content = it->content;
            eit->type = DW_CONTENT_MGWIDGET;
            return TRUE;
        }
        else if (it->content.type == DW_CONTENT_WIDGET && !DW_IS_IMAGE(it->content.data.widget)) {

            eit->stack_top++;
            a_List_add (eit->stack, eit->stack_top, eit->stack_max);
            eit->stack[eit->stack_top] =
                 a_Dw_widget_iterator (it->content.data.widget, it->mask, TRUE);
            return a_Dw_link_iterator_prev (eit);
        } else {
            eit->content = it->content;
            eit->type = it->content.type;
            return TRUE;
        }
    } else {
        if (eit->stack_top > 0) {
            a_Dw_iterator_free (it);
            eit->stack_top--;
            return a_Dw_link_iterator_prev (eit);
        } else {
             eit->content.type = DW_CONTENT_START;
             eit->type = it->content.type;
             eit->content.x_link = -1;
             return FALSE;
        }
    }
}

static void a_Dw_link_iterator_highlight (DwLinkIterator *it, gint start,
                                    gint end, DwHighlightLayer layer)
{
    if (start == -1) {
        if (it->type == DW_CONTENT_TEXT) {
            a_Dw_iterator_unhighlight (it->stack[it->stack_top], layer);
        } else {
         /* it is a image link */
            DW_IMAGE(it->content.data.widget)->selected[layer] = 0;
            p_Dw_widget_queue_draw (it->content.data.widget);
        }
    } else {
       
        if (it->type == DW_CONTENT_TEXT) {
            a_Dw_iterator_highlight(it->stack[it->stack_top], 0 , strlen(it->content.data.text)+1, layer);
        } else {
         /* it is a image link */
            DW_IMAGE(it->content.data.widget)->selected[layer] = 1;
            p_Dw_widget_queue_draw (it->content.data.widget);
        }
    }
}

static void a_Dw_link_iterator_scroll_to (DwLinkIterator *it1, DwLinkIterator *it2,
                                   gint start, gint end,
                                   DwHPosition hpos, DwVPosition vpos)
{
      a_Dw_iterator_scroll_to(it1->stack[it1->stack_top], it2->stack[it2->stack_top], 
                               start, end, hpos, vpos);
}

static DwLinkIterator* a_Dw_link_iterator_clone (DwLinkIterator *it)
{
   int i;
   DwLinkIterator *eit = g_new (DwLinkIterator, 1);

   *eit = *it;
   eit->stack = g_new (DwIterator*, eit->stack_max);
   for (i = 0; i <= eit->stack_top; i++)
      eit->stack[i] = a_Dw_iterator_clone (it->stack[i]);

   return eit;
}

static void a_Dw_link_iterator_free (DwLinkIterator *it)
{
    int i;
    for (i = 0; i <= it->stack_top; i++)
        a_Dw_iterator_free (it->stack[i]);

    g_free (it->stack);
    g_free (it);
}

LinktraveManage * a_Create_LinktraveManage(void)
{
   LinktraveManage * mg;

   mg = g_new0(LinktraveManage, 1);

   mg->stack_top = 0;
   mg->stack_max = 2; 
   mg->link_number_focus = -2;

   mg->stack = g_new0 (LinktraveState*, mg->stack_max); 
    
   mg->linktrave_state = a_Linktrave_state_new(); 

   a_List_add (mg->stack, mg->stack_top, mg->stack_max);

   mg->stack[mg->stack_top] = mg->linktrave_state; 
   return mg;
}

void a_Release_LinktraveManage(LinktraveManage * mg)
{
    int i;
    for (i = 0; i <= mg->stack_top; i++)
        a_Linktrave_state_destroy (mg->stack[i]);

    g_free (mg->stack);
    g_free (mg);
}

#endif /* ENABLE_LINKTRAVE */
