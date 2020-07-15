

#ifndef _LINKTRAVE_H_
#define _LINKTRAVE_H_

#ifdef ENABLE_LINKTRAVE

#include "browser.h"
#include "dw_ext_iterator.h"

typedef struct _LinktraveState LinktraveState;
typedef struct _LinktraveManage LinktraveManage;
typedef struct _DwLinkIterator DwLinkIterator;

struct _DwLinkIterator
{
   DwContentType type;

   DwIterator **stack;
   gint stack_top;
   gint stack_max;
   DwContent content; 
};



struct _LinktraveManage
{
   void * top_dd;
   LinktraveState ** stack;
   int stack_top;
   int stack_max;

   int link_number_focus;
   LinktraveState *linktrave_state;

};

LinktraveManage * a_Create_LinktraveManage(void);

void a_Release_LinktraveManage(LinktraveManage * mg);


#define LSM_SINGLE  0
#define LSM_MULIT   1


struct _LinktraveState
{
   gint link_no;
  
   DWORD mode;
   mSpiderDoc** ddary;
   int nr_dd;
   int cur_dd;
   int max_dd; 
   void* cur_set;

   DwWidget *widget;
   void * link_block;

   gint num_word; 
   gboolean widget_change;
   DwLinkIterator *iterator;     /* The position from where the next search
                                    will start. */
   DwLinkIterator *hl_iterator;  /* The position from where key->len words
                                    are highlighted. */
   gint first_hl_start, last_hl_end;
};

LinktraveState* a_Linktrave_state_new        (void);
void            a_Linktrave_state_destroy    (LinktraveState *state);
void            a_Linktrave_state_set_widget (LinktraveState *state,
                                              DwWidget  *widget);

#define LTS_FORWARD     0
#define LTS_BACKWARD    1


gboolean        a_Linktrave_step             (LinktraveState *state,
                                              gint link_no,
                                              int direct);
void            a_Linktrave_reset            (LinktraveState *state);

#endif /* ENABLE_LINKTRAVE */

#endif
