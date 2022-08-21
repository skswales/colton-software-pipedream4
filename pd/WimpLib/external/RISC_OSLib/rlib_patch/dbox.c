--- _src	2021-12-31 14:59:29.900000000 +0000
+++ _dst	2022-08-18 16:55:46.550000000 +0000
@@ -32,6 +32,8 @@
  *
  */
 
+#include "include.h" /* for SKS_ACW */
+
 #define BOOL int
 #define TRUE 1
 #define FALSE 0
@@ -61,6 +63,15 @@
 #include "msgs.h"
 #include "VerIntern/messages.h"
 
+/*   07-Mar-89 SKS: hacked format so I can read it!
+ *   22-Mar-89 SKS: added a hook into the window definition
+ *   05-Apr-89 SKS: made PDACTION changes for PipeDream
+ * SKS 08 Jan 1997 No longer gets in BubbleHelp's way by fronting dialogs
+ * SKS 14 Jan 1998 Use correct wimp_IESGMASK definition
+*/
+
+#define wimp_IESG               0x00010000
+
 typedef struct dbox__str {
   struct dbox__str *next;  /* if user wants to link dboxes into a list */
   wimp_w w;                /* only used in live dialog boxes */
@@ -68,7 +79,11 @@
                             * caret.
                             */
   int showing;
+#ifndef SKS_ACW
   wimp_caretstr caretstr;   /* save between fillin's. */
+#else
+  BOOL caret_set;
+#endif /* SKS_ACW */
   dbox_handler_proc eventproc;
   void *eventprochandle;
   dbox_raw_handler_proc raweventproc;
@@ -79,19 +94,45 @@
   int eventdepth;       /* for delaying disposal */
   int disposepending;
 
+#ifndef SKS_ACW
   char name[12];
   char *workspace;
   int workspacesize;
   wimp_wind window;
   /* any icons follow directly after this. */
+#else /* SKS_ACW */
+    struct _dbox_str_bits
+        {
+        int motion_updates : 1;
+        int sent_close     : 1; /* have we returned dbox_CLOSE before now? */
+        }
+    bits;
+
+  wimp_wind * p_window;
+#endif /* SKS_ACW */
 } dbox__str;
 /* Abstraction: a dbox is really a dbox__str*. */
 
+#ifdef SKS_ACW
+
+static BOOL note_position = FALSE;
+
+static struct _noted_position
+{
+    BOOL noted;
+    int x;
+    int y;
+}
+noted_position, saved_position;
+
+#endif /* SKS_ACW */
+
 /* -------- Miscellaneous. -------- */
 
 static dbox dbox__fromtemplate(template *from)
 {
   dbox to;
+#ifndef SKS_ACW
   int j;
   int size = sizeof(dbox__str) + from->window.nicons * sizeof(wimp_icon);
 
@@ -132,6 +173,35 @@
       to->window.title.indirecttext.buffer += to->workspace - from->workspace;
 
   }
+#else /* SKS_ACW */
+  /* SKS for p_window - quite a different implementation used by PipeDream */
+  size_t size = sizeof(dbox__str);
+  wimp_wind * w;
+  wimp_wflags clearbits;
+
+  tracef1("[dbox__fromtemplate, size = %d]\n", size);
+  to = calloc(size, 1);
+  if (to == 0) return 0;
+
+  /* Make a copy of the given dbox template and its workspace */
+  if(NULL == (w = (wimp_wind *) template_copy_new(from)))
+  {
+    free(to);
+    return 0;
+  }
+
+  to->p_window = w;
+
+  to->posatcaret = (0 != (w->flags & wimp_WTRESPASS));
+
+  clearbits = wimp_WTRESPASS;
+
+  /* knock out back bits on certain dboxes */
+  if(w->colours[wimp_WCSCROLLOUTER] == 12)
+      clearbits = (wimp_wflags) (clearbits | wimp_WBACK);
+
+  w->flags = (wimp_wflags) (w->flags & ~clearbits);
+#endif /* SKS_ACW */
 
   return(to);
 
@@ -140,20 +210,38 @@
 
 static void dbox__dispose(dbox d)
 {
+#ifndef SKS_ACW
   if (d->workspacesize != 0) {
     free(d->workspace);
   }
+#else /* SKS_ACW */
+  /* SKS for p_window */
+  if (d->p_window) {
+    free(d->p_window);
+  }
+#endif /* SKS_ACW */
   free(d);
 }
 
+/* This is more logically connected with dbox_dispose below, the
+ * ordering is dictated by importation of process_wimp_event in dbox_new.
+*/
+/* The menu is removed just in case any client had registered one */
 static void dbox__dodispose(dbox d)
 {
   win_register_event_handler(d->w, 0, 0);
   event_attachmenu(d->w, 0, 0, 0);
+#ifndef SKS_ACW
   if (d->showing) {
     win_activedec();
   }
   wimpt_noerr(wimp_delete_wind(d->w));
+#else /* SKS_ACW */
+  {
+  HOST_WND window_handle = d->w;
+  (void) wimpt_complain(winx_delete_window(&window_handle));
+  } /* block */
+#endif /* SKS_ACW */
   dbox__dispose(d);
 }
 
@@ -174,8 +262,15 @@
 /* Rather like SWI WhichIcon, but only finds the first. Returns 0 if not
 found. */
 {
+#ifndef SKS_ACW
   for (; (*j)<d->window.nicons; (*j)++) {
     wimp_icon *i = ((wimp_icon*) (&d->window + 1)) + *j;
+#else /* SKS_ACW */
+  /* SKS for p_window */
+  const wimp_wind *w = d->p_window;
+  for (; (*j)<w->nicons; (*j)++) {
+    const wimp_icon *i = ((const wimp_icon *) (w + 1)) + *j;
+#endif /* SKS_ACW */
     if ((i->flags & mask) == settings) {
       tracef1("Found icon %i.\n", *j);
       return(1);
@@ -184,12 +279,20 @@
   return(0);
 }
 
+#ifndef SKS_ACW
 static int dbox__findiconbefore(dbox d,
   wimp_iconflags mask, wimp_iconflags settings, wimp_i *j)
 /* Does not look at the current icon. */
 {
+#ifndef SKS_ACW
   while ((*j) != 0) {
     wimp_icon *i = ((wimp_icon*) (&d->window + 1)) + (--(*j));
+#else /* SKS_ACW */
+  /* SKS for p_window */
+  const wimp_wind * w = d->p_window;
+  while ((*j) != 0) {
+    const wimp_icon *i = ((const wimp_icon *) (w + 1)) + (--(*j));
+#endif /* SKS_ACW */
     if ((i->flags & mask) == settings) {
       tracef1("Found icon %i.\n", *j);
       return(1);
@@ -197,9 +300,12 @@
   }
   return(0);
 }
+#endif /* SKS_ACW */
 
 /* -------- Icons and Fields. -------- */
 
+#ifndef SKS_ACW
+
 #ifndef UROM
 static dbox_field dbox__icontofield(wimp_i i)
 {
@@ -207,6 +313,8 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 static wimp_i dbox__fieldtoicon(dbox_field f)
 {
   return(f);
@@ -214,7 +322,12 @@
 
 static wimp_icon *dbox__iconhandletoptr(dbox d, wimp_i i)
 {
+#ifndef SKS_ACW
   return(((wimp_icon*) (&d->window + 1)) + i);
+#else /* SKS_ACW */
+  /* SKS for p_window */
+  return(((wimp_icon*) (d->p_window + 1)) + i);
+#endif /* SKS_ACW */
 }
 
 static wimp_icon *dbox__fieldtoiconptr(dbox d, dbox_field f)
@@ -224,7 +337,11 @@
 
 static wimp_iconflags dbox__ibutflags(wimp_icon *i)
 {
+#ifndef SKS_ACW
   return(i->flags & BUTTON_IFLAGS);
+#else /* SKS_ACW */
+  return((wimp_iconflags) (i->flags & BUTTON_IFLAGS)); /* SKS cast for modern warnings */
+#endif /* SKS_ACW */
 }
 
 static dbox_fieldtype dbox__iconfieldtype(wimp_icon *i)
@@ -245,6 +362,8 @@
   }
 }
 
+#ifndef SKS_ACW
+
 static BOOL dbox__has_action_button(dbox d)
 {
    wimp_i j;
@@ -264,26 +383,37 @@
 
 static int dbox__min(int a, int b) {if (a<b) {return(a);} else {return(b);}}
 
+#endif /* SKS_ACW */
+
 
 void dbox_fadefield (dbox d, dbox_field f)
 {
 
   /* set shaded bit in iconflags */
+#ifndef SKS_ACW
   wimpt_noerr(wimp_set_icon_state (d->w, dbox__fieldtoicon(f),
                                    wimp_INOSELECT, wimp_INOSELECT));
+#else /* SKS_ACW */
+  winf_fadefield(dbox_window_handle(d), dbox_field_to_icon_handle(ed, f));
+#endif /* SKS_ACW */
 }
 
 void  dbox_unfadefield (dbox d, dbox_field f)
 {
 
   /* unset shaded bit in iconflags */
+#ifndef SKS_ACW
   wimpt_noerr(wimp_set_icon_state(d->w, dbox__fieldtoicon(f),
                                   0, wimp_INOSELECT));
+#else /* SKS_ACW */
+  winf_unfadefield(dbox_window_handle(d), dbox_field_to_icon_handle(ed, f));
+#endif /* SKS_ACW */
 }
 
 
 void dbox_setfield(dbox d, dbox_field f, char *value)
 {
+#ifndef SKS_ACW
   wimp_icon *i = dbox__fieldtoiconptr(d, f);
   if ((i->flags & wimp_ITEXT) == 0)
   {
@@ -324,10 +454,14 @@
     /* prod it, to cause redraw */
     wimpt_noerr(wimp_set_icon_state(d->w, dbox__fieldtoicon(f), 0, 0));
   }
+#else /* SKS_ACW */
+  winf_setfield(d->w, dbox_field_to_icon_handle(d, f), value);
+#endif /* SKS_ACW */
 }
 
 void dbox_getfield(dbox d, dbox_field f, char *buffer, int size)
 {
+#ifndef SKS_ACW
   wimp_icon *i = dbox__fieldtoiconptr(d, f);
   int j = 0;
   char *from;
@@ -348,6 +482,9 @@
     (void) memcpy(buffer, from, j);
   }
   buffer[j] = 0;
+#else /* SKS_ACW */
+  winf_getfield(d->w, dbox_field_to_icon_handle(d, f), buffer, size);
+#endif /* SKS_ACW */
   tracef1("GetField returns %s.\n", (int) buffer);
 }
 
@@ -361,6 +498,7 @@
 
 void dbox_setnumeric(dbox d, dbox_field f, int n)
 {
+#ifndef SKS_ACW
   char a[20];
   wimp_icon *i = dbox__fieldtoiconptr(d, f);
   dbox_fieldtype ftype = dbox__iconfieldtype(i);
@@ -379,10 +517,25 @@
       sprintf(a, "%i", n);
       dbox_setfield((dbox) d, f, a);
   }
+#else /* SKS_ACW */
+  wimp_icon *i = dbox__fieldtoiconptr(d, f);
+  dbox_fieldtype ftype = dbox__iconfieldtype(i);
+
+  switch(ftype) {
+  case dbox_FONOFF:
+  case dbox_FACTION:
+      winf_setonoff(d->w, dbox_field_to_icon_handle(d, f), (n != 0));
+      break;
+  default:
+      winf_setint(d->w, dbox_field_to_icon_handle(d, f), n);
+      break;
+  }
+#endif /* SKS_ACW */
 }
 
 int dbox_getnumeric(dbox d, dbox_field f)
 {
+#ifndef SKS_ACW
   char a[20];
   int n;
   int i;
@@ -420,6 +573,20 @@
     if (neg) {n = -n;}
     if (fail) {n = 0;}
   }
+#else /* SKS_ACW */
+  int n;
+  wimp_icon *iptr = dbox__fieldtoiconptr(d, f);
+
+  switch(dbox__iconfieldtype(iptr)) {
+  case dbox_FONOFF:
+  case dbox_FACTION:
+    n = winf_getonoff(d->w, dbox_field_to_icon_handle(d, f));
+    break;
+  default:
+    n = winf_getint(d->w, dbox_field_to_icon_handle(d, f), 0);
+    break;
+  }
+#endif /* SKS_ACW */
   return(n);
 }
 
@@ -469,6 +636,140 @@
   }
 }
 
+#ifdef SKS_ACW
+
+static void dbox__hitfield(dbox d, dbox_field hit_j, BOOL adjustclicked, BOOL notwimphit)
+{
+    wimp_w w;
+    wimp_icon *i;
+    wimp_i j, nicons;
+    wimp_i k, icon_to_select;
+    dbox_fieldtype f;
+    wimp_icon icon;
+    int esg;
+    wimp_i esg_first, esg_selected;
+    int woggle_count;
+    BOOL allow_invert = TRUE;
+    wimp_eventstr * e;
+
+    tracef2("[dbox_hitfield(&%p, %d)]\n", d, hit_j);
+
+    w      = d->w;
+    nicons = d->p_window->nicons;
+
+    i = dbox__iconhandletoptr(d, hit_j);
+    f = dbox__iconfieldtype(i);
+
+    /* lots of pratting about required here to cope with the inadequacies
+     * of el Window Manager. If a member of an non-menu type esg, then if right click, step
+     * selection, deselect all others anyway.
+    */
+    esg = i->flags & wimp_IESGMASK;
+    tracef2("[dbox_hitfield icon %d esg %8X]\n", hit_j, esg);
+    if(dbox__ibutflags(i) == MENU_IFLAGS)
+        {
+        /* give it a woggle now, restore state after */
+        icon_to_select = (wimp_i) hit_j;
+        for(woggle_count = 0; woggle_count < 5; ++woggle_count)
+            winf_invertfield(w, icon_to_select);
+        }
+    else if(esg)
+        {
+        /* first turn all selected items off, select just one at end */
+        esg_first = esg_selected = icon_to_select = (wimp_i) -1;
+        j = 0;
+        do  {
+            i = dbox__iconhandletoptr(d, j);
+            k = (wimp_i) j;
+            tracef2("[considering icon %d, esg %8X]\n", j, i->flags & wimp_IESGMASK);
+            if(esg == (i->flags & wimp_IESGMASK))
+                {
+                if(esg_first == (wimp_i) -1)
+                    esg_first = k; /* found the first member of this ESG */
+
+                (void) wimpt_complain(wimp_get_icon_info(w, k, &icon));
+                if(icon.flags & wimp_ISELECTED)
+                    {
+                    esg_selected = k;
+                    if(adjustclicked  ||  (k != (wimp_i) hit_j))
+                        {
+                        /* if found a selected one turn it off if adjustclicked or not the right one */
+                        (void) wimpt_complain(wimp_set_icon_state(w, esg_selected,
+                                                                  /* EOR */ wimp_ISELECTED,
+                                                                  /* BIC */ (wimp_iconflags) 0));
+                        }
+                    else
+                        {
+                        /* Select clicked on selected icon - keep selected */
+                        icon_to_select = (wimp_i) hit_j;
+                        allow_invert = FALSE;
+                        }
+                    }
+                else
+                    {
+                    if(adjustclicked)
+                        {
+                        if((esg_selected != (wimp_i) -1)  &&  (icon_to_select == (wimp_i) -1))
+                            /* this icon is the next one along from the one that was selected */
+                            icon_to_select = k;
+                        }
+                    else
+                        {
+                        if(k == (wimp_i) hit_j)
+                            /* Select clicked on unselected icon - will need inversion to select */
+                            icon_to_select = (wimp_i) hit_j;
+                        }
+                    }
+                }
+            }
+        while(++j < nicons);
+
+        tracef3("[esg_first %p, esg_selected %d, icon_to_select %p]\n", esg_first, esg_selected, icon_to_select);
+
+        if(icon_to_select == (wimp_i) -1)
+            {
+            icon_to_select = esg_first;
+
+            if(esg_selected == esg_first)
+                /* one member ESG, already deselected */
+                allow_invert = FALSE;
+            }
+
+        /* now select this one, all currently cleared */
+        }
+    else
+        {
+        /* toggle this one unless autorepeat type */
+        if(dbox__ibutflags(i) == AUTO_IFLAGS)
+            allow_invert = FALSE;
+
+        /* if hit by Neil, he will have selected it already */
+        if(!notwimphit)
+            allow_invert = FALSE; /* why oh why oh why */
+
+        icon_to_select = (wimp_i) hit_j;
+
+        }
+
+    if(allow_invert)
+        winf_invertfield(w, icon_to_select);
+
+    /* ensure hit looked like it came from a left button click */
+    e = wimpt_last_event();
+    if(e->e != wimp_EBUT)
+        {
+        e->e = wimp_EBUT;
+        e->data.but.m.bbits = wimp_BLEFT;
+        e->data.but.m.w     = w;
+        e->data.but.m.i     = icon_to_select;
+        }
+
+    /* finally call the responsible client with the field finally selected */
+    dbox__buttonclick(d, (dbox_field) icon_to_select);
+}
+
+#endif /* SKS_ACW */
+
 static BOOL dbox__hitbutton(dbox d, int button)
 /* A button is an action button or an on/off switch. "button" counts only
 such interesting buttons, button==0 -> the first one in the DBox. Find the
@@ -478,6 +779,7 @@
   wimp_icon *i;
   int j = 0; /* counts icons */
   dbox_fieldtype f;
+#ifndef SKS_ACW
   wimp_icon icon;
   BOOL icon_found = FALSE;
 
@@ -511,57 +813,125 @@
       /* not the right sort of icon: keep going. */
     }
   }
+#else /* SKS_ACW */
+  int esg, esg_seen_mask = 0;
+  BOOL icon_found = FALSE;
+
+  /* loop over icons until correct button found */
+  for (j=0; j<d->p_window->nicons; j++) {
+    i = dbox__iconhandletoptr(d, j);
+    f = dbox__iconfieldtype(i);
+    switch (f)
+    {
+    case dbox_FONOFF:
+    case dbox_FACTION:
+      /* on/off or action button */
+      esg = i->flags & wimp_IESGMASK;
+      if(esg)
+      {
+        /* only consider the first member of an ESG, ignore others for count */
+            esg = esg / wimp_IESG; /* number in 1..15 */
+        esg = 1 << esg;
+        if(esg_seen_mask & esg)
+        {
+          tracef2("[icon %d is a member of a noted ESG, mask %8X --- ignored]\n", j, esg);
+          continue; /* get another icon before considering button hit */
+        }
+        tracef2("[icon %d is the first member of an ESG, mask %8X --- counted]\n", j, esg);
+        esg_seen_mask |= esg;
+      }
+      else
+      {
+        tracef1("[icon %d is not a member of any ESG --- counted]\n", j);
+      }
+
+      if(button-- == 0)
+      {
+        dbox__hitfield(d, j, TRUE, TRUE);
+        return FALSE;
+      }
+
+      break;
+
+    default:
+      tracef1("[icon %d is not an action or an on/off button --- ignored]\n", j);
+      break;
+    }
+  }
+#endif /* SKS_ACW */
 
   return icon_found;
 }
 
-static void dbox__wimp_event_handler(wimp_eventstr *e, void *handle)
+
+#define DBOX_SHIFTED_FN_START 10
+
+static BOOL dbox__wimp_event_handler(wimp_eventstr *e, void *handle)
 {
   dbox d = (dbox) handle;
   wimp_caretstr c;
   wimp_icon *i;
   wimp_i j;
   char target;
+  BOOL setcaretpos = FALSE;
+  BOOL done;
+  int ch;
+
+  tracef2("[dbox__wimp_event_handler got event %s for dbox &%p]\n", report_wimp_event(e->e, &e->data), d);
 
   if (d->raweventproc != 0) {
-    BOOL done;
-    tracef0("client-installed raw event handler.\n");
+    tracef2("calling client-installed raw event handler(%s, &%p)\n",
+            report_procedure_name(report_proc_cast(d->raweventproc)),
+            d->raweventprochandle);
     d->eventdepth++;
     done = (d->raweventproc)(d, (void*) e, d->raweventprochandle);
     d->eventdepth--;
     if (d->disposepending && d->eventdepth == 0) {
       tracef0("delayed dispose of DBox.\n");
       dbox__dodispose(d);
-      return;
+      return(done);
     } else if (done) { /* this event has been processed. */
-      return;
+      return(done);
     }
   }
 
+  /* process some events */
+  done = TRUE;
+
   switch (e->e) {
   case  wimp_ECLOSE:
       dbox__buttonclick(d, dbox_CLOSE); /* special button code */
       break;
+
+#ifndef SKS_ACW
   case wimp_EOPEN:
       wimpt_noerr(wimp_open_wind(&e->data.o));
       break;
+#endif /* SKS_ACW */
+
   case wimp_EBUT:
-      if ((wimp_BMID & e->data.but.m.bbits) != 0) {
-        /* ignore it. */
+      if(((wimp_BMID | (wimp_BMID << 4) | (wimp_BMID << 8)) & e->data.but.m.bbits) != 0) {
+        /* ignore it */
         /* It will already have been intercepted (by Events) if there's
         a menu, otherwise we're not interested anyway. */
       } else if (e->data.but.m.i != (wimp_i) -1) {
         /* ignore clicks not on icons. */
         i = dbox__iconhandletoptr(d, e->data.but.m.i);
-        if (dbox__iconfieldtype(i) == dbox_FACTION) {
-          /* avoid spurious double-click from on/off button! */
-          dbox__buttonclick(d, e->data.but.m.i);
+        switch(dbox__iconfieldtype(i)) {
+        case dbox_FONOFF:
+        case dbox_FACTION:
+            dbox__hitfield(d, (dbox_field) e->data.but.m.i, e->data.but.m.bbits & wimp_BRIGHT, FALSE);
+            break;
+        default:
+            break;
         }
       }
       break;
+
   case wimp_EKEY:
       wimpt_noerr(wimp_get_caret_pos(&c));
-      switch (e->data.key.chcode) {
+      ch = e->data.key.chcode;
+      switch(e->data.key.chcode) {
       case akbd_Fn+1:
       case akbd_Fn+2:
       case akbd_Fn+3:
@@ -571,87 +941,152 @@
       case akbd_Fn+7:
       case akbd_Fn+8:
       case akbd_Fn+9:
-          /* if fnkey matches icon number, do action, else pass it on
-           * as a hotkey stroke
-           */
-          if (!dbox__hitbutton(d, e->data.key.chcode - (akbd_Fn+1)))
-            wimp_processkey(e->data.key.chcode);
+          dbox__hitbutton(d, ch - (akbd_Fn+1));
+          break;
+
+      case akbd_Fn10:
+  /*  case akbd_Fn11:     keep off F11 too!!!     *** */
+  /*  case akbd_Fn12:     keep off F12 !!!        *** */
+          dbox__hitbutton(d, ch - (akbd_Fn10) + 9);
+          break;
+
+#if defined(DBOX_SHIFTED_FN_START)
+      case akbd_Sh+akbd_Fn+1:
+      case akbd_Sh+akbd_Fn+2:
+      case akbd_Sh+akbd_Fn+3:
+      case akbd_Sh+akbd_Fn+4:
+      case akbd_Sh+akbd_Fn+5:
+      case akbd_Sh+akbd_Fn+6:
+      case akbd_Sh+akbd_Fn+7:
+      case akbd_Sh+akbd_Fn+8:
+      case akbd_Sh+akbd_Fn+9:
+          dbox__hitbutton(d, ch - (akbd_Sh+akbd_Fn+1) + DBOX_SHIFTED_FN_START);
+          break;
+
+      case akbd_Sh+akbd_Fn10:
+  /*  case akbd_Sh+akbd_Fn11:     keep off F11 too!!!     *** */
+  /*  case akbd_Sh+akbd_Fn12:     keep off F12 !!!        *** */
+          dbox__hitbutton(d, ch - (akbd_Sh+akbd_Fn10) + 9 + DBOX_SHIFTED_FN_START);
           break;
+#endif
 
       case 13: /* return key */
-          tracef1("Caret is in icon %i.\n", c.i);
-          c.i++;
-          if (c.i >= d->window.nicons ||
-              c.i < 0 ||
-              0==dbox__findicon(d, WRITABLE_IFLAGS, WRITABLE_IFLAGS, &c.i)
-               /* find a writable button */
-             )
+          tracef1("Caret is in icon %i\n", c.i);
+
+#if defined(PDACTION)
+          /* should be first action button! */
+          dbox__hitbutton(d, 0);
+#else /* PDACTION */
+          if(c.i != (wimp_i) -1)
           {
-            dbox__buttonclick(d, 0); /* should be first action button! */
-          } else {
-            c.height = -1;
-            c.index = dbox__fieldlength(d, c.i);
-            tracef2("Setting caret in icon %i index=%i.\n",
-              c.i, c.index);
-            wimpt_noerr(wimp_set_caret_pos(&c));
+            c.i = (wimp_i) ((wimp__i) c.i + 1);
+            if( ((wimp__i) c.i >= nicons)  ||
+                !dbox__findicon(d,
+                                WRITABLE_IFLAGS | wimp_INOSELECT,
+                                WRITABLE_IFLAGS | 0, /* desired state of just these flags */
+                                &c.i)
+                /* find a writeable button */
+              )
+            {
+              /* should be first action button! */
+              dbox__hitbutton(d, 0);
+            }
+            else
+            {
+              c.index = dbox__fieldlength(d, c.i);
+              setcaretpos = TRUE;
+            }
+          }
+          else
+          {
+            /* should be first action button! */
+            dbox_buttonclick(d, 0);
           }
+#endif /* PDACTION */
           break;
+
       case 27: /* ESC key */
           dbox__buttonclick(d, dbox_CLOSE);
           break;
-      case  398: /* DOWN key */
-          tracef1("Caret is in icon %i.\n", c.i);
-          if (c.i == (wimp_i) -1) {
+
+#ifndef SKS_ACW /* leave to Window Manager Kat validation processing now */
+#if defined(PDACTION)
+      case akbd_TabK:
+#endif /* PDACTION */
+      case akbd_DownK:
+          tracef1("Caret is in icon %i\n", c.i);
+
+#ifdef DBOX_MOTION_UPDATES
+          if(d->bits.motion_updates)
+              dbox__buttonclick(d, c.i);
+#endif
+          if(c.i == (wimp_i) -1) {
             /* do nothing */
           } else {
             c.i++;
-            if (c.i >= d->window.nicons ||
-                ! dbox__findicon(d, WRITABLE_IFLAGS, WRITABLE_IFLAGS, &c.i))
+            if( (c.i >= d->p_window->nicons)  ||
+                !dbox__findicon(d,
+                                (wimp_iconflags) (WRITABLE_IFLAGS | wimp_INOSELECT),
+                                (wimp_iconflags) (WRITABLE_IFLAGS),
+                                &c.i))
             {
               c.i = 0;
-              if (dbox__findicon(d, WRITABLE_IFLAGS, WRITABLE_IFLAGS, &c.i)) {
-                /* bound to find at least the one you started on. */
-              }
+              (void) dbox__findicon(d,
+                                    (wimp_iconflags) (WRITABLE_IFLAGS | wimp_INOSELECT),
+                                    (wimp_iconflags) (WRITABLE_IFLAGS),
+                                    &c.i);
+              /* bound to find at least the one you started on.*/
             }
-            c.height = -1;
+
             c.index = dbox__fieldlength(d, c.i);
-            tracef2("Setting caret in icon %i index=%i.\n",
-              c.i, c.index);
-            wimpt_noerr(wimp_set_caret_pos(&c));
+            setcaretpos = TRUE;
           }
           break;
-      case  399: /* UP key */
-          tracef1("Caret is in icon %i.\n", c.i);
+
+#if defined(PDACTION)
+      case akbd_TabK + akbd_Sh:
+#endif /* PDACTION */
+      case akbd_UpK:
+          tracef1("Caret is in icon %i\n", c.i);
+
+#ifdef DBOX_MOTION_UPDATES
+          if(d->bits.motion_updates)
+              dbox__buttonclick(d, c.i);
+#endif
           if (c.i == (wimp_i) -1) {
             /* do nothing */
           } else {
-            if (!dbox__findiconbefore(d,
-                  WRITABLE_IFLAGS, WRITABLE_IFLAGS, &c.i)) {
-              c.i = d->window.nicons;
-              if (dbox__findiconbefore(d,
-                    WRITABLE_IFLAGS, WRITABLE_IFLAGS, &c.i)) {
-                /* bound to find at least the one you started on. */
-              }
+            if(!dbox__findiconbefore(d,
+                                     (wimp_iconflags) (WRITABLE_IFLAGS | wimp_INOSELECT),
+                                     (wimp_iconflags) (WRITABLE_IFLAGS),
+                                     &c.i))
+            {
+              c.i = d->p_window->nicons;
+              (void) dbox__findiconbefore(d,
+                                          (wimp_iconflags) (WRITABLE_IFLAGS | wimp_INOSELECT),
+                                          (wimp_iconflags) (WRITABLE_IFLAGS),
+                                          &c.i);
+              /* bound to find at least the one you started on */
             }
-            c.height = -1;
+
             c.index = dbox__fieldlength(d, c.i);
-            tracef2("Setting caret in icon %i index=%i.\n",
-              c.i, c.index);
-            wimpt_noerr(wimp_set_caret_pos(&c));
+            setcaretpos = TRUE;
           }
           break;
+#endif /* SKS_ACW */
+
       default:
           /* If not to a field and this is a letter, try matching it
           with the first chars of action buttons in this DBox. */
-          if (e->data.key.chcode < 256)
-              e->data.key.chcode = toupper(e->data.key.chcode);
-          if (e->data.key.chcode < 256 && isalpha(e->data.key.chcode)) {
-            for (j=0; j<d->window.nicons; j++) {
-              tracef1("trying icon %i.\n", j);
+          if(!(ch & ~0xFF)  &&  isalpha(ch))
+          {
+            ch = toupper(ch); /* now buggered */
+            for(j = 0; j < d->p_window->nicons; ++j) {
+              tracef1("trying icon %i\n", j);
               i = dbox__iconhandletoptr(d, j);
               if ((i->flags & wimp_ITEXT) != 0
               && dbox__iconfieldtype(i) == dbox_FACTION) {
-                char *targetptr;
+                const char *targetptr;
                 BOOL found = FALSE;
 
                 if ((i->flags & wimp_INDIRECT) != 0) {
@@ -663,12 +1098,13 @@
                 while (1) {
                   target = *targetptr++;
                   if (target == 0) break;
-                  if (target == e->data.key.chcode) {
+                  if (target == ch) { /* NB ch is uppercased */
                     tracef2("clicking on %i, %i.\n", j, target);
-                    dbox__buttonclick(d, j);
+                    dbox__hitfield(d, j, FALSE, TRUE);
                     found = TRUE;
                     break;
                   }
+                  /* end if we didn't match the capital letter */
                   if (isupper(target)) break;
                 }
                 if (found) break;
@@ -680,17 +1116,33 @@
             tracef1("Key code %i ignored.\n", e->data.key.chcode);
             wimp_processkey(e->data.key.chcode);
           }
+          break;
       }
+
+      /* end of all EKEY type events */
+      if(setcaretpos)
+      {
+        c.height = -1; /* calc x,y,h from icon/index */
+        tracef2("Setting caret in icon %i, index = %i\n", c.i, c.index);
+        (void) wimpt_complain(wimp_set_caret_pos(&c));
+      }
+
       break;
+
   default:
       /* do nothing */
-      tracef3("DBoxes ignored event %i %i %i.\n",
-        e->e, e->data.o.w, e->data.menu[1]);
+      tracef1("[dbox__wimp_event_handler: ignored event %s]\n", report_wimp_event(e->e, &e->data));
+      done = FALSE;
+      break;
   }
+
+  return(done);
 }
 
 /* -------- New and Dispose. -------- */
 
+#ifndef SKS_ACW
+
 dbox dbox_new(char *name)
 {
   dbox d = dbox__fromtemplate(template_find(name));
@@ -734,7 +1186,50 @@
   return d;
 }
 
-static wimp_w dbox__submenu = 0;
+#else /* SKS_ACW */
+
+/* SKS Now ONLY for binary compatibility */
+
+dbox dbox_new(char * name)
+{
+  char * errorp;
+  return(dbox_new_new(name, &errorp));
+}
+
+static template *dbox__findtemplate(const char * name)
+{
+  template * templateHandle = template_find_new(name);
+  if(TRACE  &&  !templateHandle)
+    werr(FALSE, "Template '%s' not found", name);
+  return(templateHandle);
+}
+
+dbox dbox_new_new(const char * name, char ** errorp /*out*/)
+{
+  dbox d;
+
+  tracef1("dbox_new_new(%s)\n", name);
+
+  d = dbox__fromtemplate(dbox__findtemplate(name));
+
+  if (d == 0) {
+    *errorp = NULL;
+  }
+  else
+  { os_error *er;
+    er = winx_create_window((WimpWindowWithBitset *) d->p_window, (HOST_WND *) &d->w, dbox__wimp_event_handler, d);
+    if (er != 0) {
+      *errorp = er->errmess;
+      dbox__dispose(d);
+      return 0;
+    }
+  }
+
+  tracef1("dbox_new_new yields &%p\n", d);
+  return d;
+}
+
+#endif /* SKS_ACW */
 
 static void dbox__doshow(dbox d, BOOL isstatic)
 /* This is complicated by the following case: if the show is as a result
@@ -744,61 +1239,100 @@
   wimp_mousestr m;
   wimp_caretstr c;
   wimp_openstr o;
-  wimp_wstate s;
+  wimp_wstate ws;
   wimp_eventstr *e;
+  int submenu;
+  int xsize, ysize;
+
+  d->bits.sent_close = FALSE; /* clear this every time we show up */
 
   if (d->showing) return;
   d->showing = TRUE;
+#ifndef SKS_ACW
   win_activeinc();
+#endif
 
   e = wimpt_last_event();
-  if (e->e == wimp_ESEND && e->data.msg.hdr.action == wimp_MMENUWARN) {
-    /* this is a dbox that is actually part of the menu tree. */
-    tracef0("opening submenu dbox.\n");
-    dbox__submenu = d->w; /* there's only ever one. */
-    wimp_create_submenu(
-      (wimp_menustr*) d->w,
-      e->data.msg.data.words[1],
-      e->data.msg.data.words[2]);
-  } else {
-    o.w = d->w;
-    o.box = d->window.box;
+
+  submenu = ((e->e == wimp_ESEND)  &&  (e->data.msg.hdr.action == wimp_MMENUWARN));
+
+  if(submenu && !isstatic)
+  {
+    event_read_submenupos(&m.x, &m.y);
+    dbox_noted_position_trash();
+  }
+  else if(noted_position.noted)
+  {
+    m.x = noted_position.x;
+    m.y = noted_position.y;
+    noted_position.noted = FALSE;
+    /* but don't clear note_position! */
+  }
+  else
+  {
+    tracef0("Move dbox to near pointer\n");
+    (void) wimpt_complain(wimp_get_point_info(&m));
+    m.x -= 64 /*32*/; /* try to be a bit into it */
+    m.y += 64 /*32*/; /* SKS 23oct96 make distance consistent with Fireworkz dialogue boxes */
+
     if (d->posatcaret) {
       /* move to near the caret. */
-      if (wimpt_last_event_was_a_key()) {
-        tracef0("Move DBox to near caret.\n");
-        wimpt_noerr(wimp_get_caret_pos(&c));
-        if (c.w != (wimp_w) -1) {
-          wimpt_noerr(wimp_get_wind_state(c.w, &s));
-          c.x = c.x + (s.o.box.x0 - s.o.x);
-          c.y = c.y + (s.o.box.y1 - s.o.y);
+      if ((e->e == wimp_EKEY)) {
+        /* move to near the caret if it's in a window */
+        (void) wimpt_complain(wimp_get_caret_pos(&c));
+        if(c.w != (wimp_w) -1) {
+          tracef0("Move dbox to near caret.\n");
+          (void) wimpt_complain(wimp_get_wind_state(c.w, &ws));
+          c.x = c.x + (ws.o.box.x0 - ws.o.x);
+          c.y = c.y + (ws.o.box.y1 - ws.o.y);
+
+          m.x = c.x + 100; /* a little to the right */
+          m.y = c.y - 120; /* a little down */
         }
-        m.x = c.x + 100; /* a little to the right */
-        m.y = c.y - 120; /* a little down */
-      } else {
-        tracef0("Move DBox to near cursor.\n");
-        wimpt_noerr(wimp_get_point_info(&m));
-        m.x -= 48; /* try to be a bit into it. */
-        m.y += 48;
-      }
-      tracef2("put box at (%i,%i).\n", m.x, m.y);
-      o.box.y0 = m.y - (o.box.y1 - o.box.y0);
-      o.box.x1 = m.x + (o.box.x1 - o.box.x0);
-      o.box.y1 = m.y;
-      o.box.x0 = m.x;
-    }
-    o.x = d->window.scx;
-    o.y = d->window.scy;
-    o.behind = (wimp_w) -1;
+      }
+    }
+  }
 
-    if (isstatic) {
-      wimpt_noerr(wimp_open_wind(&o));
+  if(submenu && !isstatic)
+  {
+    /* this is a dbox that is actually part of the menu tree */
+    tracef0("opening dbox as top-level menu (submenu)");
+    (void) wimpt_complain(winx_create_submenu(d->w, m.x, m.y));
+  }
+  else
+  {
+    o.w = d->w;
+    o.box = d->p_window->box;
+    o.x = d->p_window->scx;
+    o.y = d->p_window->scy;
+    o.behind = -1;
+
+    xsize = o.box.x1 - o.box.x0;
+    ysize = o.box.y1 - o.box.y0;
+
+    tracef2("put box at (%i,%i).\n", m.x, m.y);
+    o.box.x0 = m.x;
+    o.box.y0 = m.y - ysize;
+    o.box.x1 = m.x + xsize;
+    o.box.y1 = m.y;
+
+    /* try not to overlap icon bar much (SKS 23oct96 from Fireworkz) */
+    if(96 > o.box.y0)
+    {
+      int d_y = 96 - o.box.y0;
+      o.box.y0 += d_y;
+      o.box.y1 += d_y;
+    }
+
+    if(isstatic) {
+      (void) wimpt_complain(winx_open_window((WimpOpenWindowBlock *) &o));
     } else {
-      dbox__submenu = d->w; /* there's only ever one. */
-      wimp_create_menu((wimp_menustr*) d->w, o.box.x0, o.box.y1);
+      /*dbox__submenu = d->w;*/ /* there's only ever one. */
+      tracef0("opening dbox as top-level menu (not submenu)");
+      (void) wimpt_complain(winx_create_menu(d->w, o.box.x0, o.box.y1));
     }
 
-    tracef0("Dialog box shown.\n");
+    tracef0("Dialog box shown\n");
   }
 }
 
@@ -810,13 +1344,15 @@
   dbox__doshow(d, TRUE);
 }
 
-void dbox_hide(dbox d)
+static void
+dbox__dohide(dbox d)
 {
-  tracef0("dbox_hide.\n");
+  tracef1("dbox_hide(&%p): ", d);
   if (! d->showing) {
-    tracef0("dbox_hide, not showing.\n");
+    tracef0("dbox_hide, not showing\n");
   } else {
     d->showing = FALSE;
+#ifndef SKS_ACW
     win_activedec();
     if (d->w == dbox__submenu) {
       wimp_wstate ws;
@@ -835,18 +1371,42 @@
       tracef0("hiding non-submenu dbox.\n");
       wimpt_noerr(wimp_close_wind(d->w));
     }
+#else /* SKS_ACW */
+      if(note_position) { /* stash the window position */
+        wimp_wstate ws;
+        (void) wimpt_complain(wimp_get_wind_state(d->w, &ws));
+        noted_position.x = (ws.o.box.x0 - ws.o.x);
+        noted_position.y = (ws.o.box.y1 - ws.o.y);
+        noted_position.noted = TRUE;
+      }
+      tracef0("hiding non-submenu dbox.\n");
+      (void) wimpt_complain(winx_close_window(d->w));
+#endif /* SKS_ACW */
   }
+
+  note_position = FALSE; /* can only be a one-shot operation */
+}
+
+extern void
+dbox_hide(dbox d)
+{
+  dbox__dohide(d);
 }
 
 void dbox_dispose(dbox *dd)
 {
   dbox d = *dd;
+  tracef2("dbox_dispose(&%p -> &%p): ", dd, d);
+  if (d == 0) return;
   if (d->eventdepth != 0) {
+    /* don't kill me yet */
+    tracef0("setting pending dispose as in event processor\n");
     d->disposepending = 1;
   } else {
-    if (d->showing) dbox_hide(d);
+    dbox_hide(d);
     dbox__dodispose(d);
   }
+  *dd = NULL;
 }
 
 /* -------- Event processing. -------- */
@@ -856,106 +1416,172 @@
 to see where in the text they've got to so far. dboxes with no fill-in fields
 do not even try to get the caret. */
 
-static dbox_field dbox_fillin_loop(dbox d)
+dbox_field dbox_fillin_loop(dbox d)
 {
-  wimp_eventstr e;
-  int harmless;
-  dbox_field result;
-  wimp_wstate ws;
+    wimp_eventstr e;
+    int harmless;
+    dbox_field result;
+    wimp_wstate ws;
+    wimp_caretstr caret;
+    wimp_w w = d->w;
+    wimp_i j = 0;
 
-  while (1) {
-    int null_at;
-    if (alarm_next(&null_at) && (event_getmask() & wimp_EMNULL) != 0)
-        wimpt_complain(wimp_pollidle(event_getmask() & ~wimp_EMNULL, &e, null_at));
-    else
-        wimpt_complain(wimp_poll(event_getmask(), &e));
+    tracef3("dbox_fillin(&%p (%d)): sp ~= &%p\n", d, w, &ws);
 
-    if (d->w == dbox__submenu) {
-      wimpt_noerr(wimp_get_wind_state(d->w, &ws));
-      if ((ws.flags & wimp_WOPEN) == 0) {
-        tracef0("dbox has been closed for us!.\n");
-        wimpt_fake_event(&e); /* stuff it back in the queue */
-        if (e.e == wimp_EREDRAW) event_process();
-        return dbox_CLOSE;
-      }
-    }
+    if(d->bits.sent_close)
+        return(dbox_CLOSE); /* keep returning this until the client listens */
 
-    switch (e.e) {
-    case wimp_ENULL:
-    case wimp_EREDRAW:
-    case wimp_EPTRENTER:
-    case wimp_EPTRLEAVE:
-    case wimp_ESCROLL:
-    case wimp_EOPEN:
-    case wimp_EMENU:
-    case wimp_ELOSECARET:
-    case wimp_EGAINCARET:
-    case wimp_EUSERDRAG:
-        harmless = TRUE;
-        break;
-    case wimp_ECLOSE:
-        harmless = e.data.o.w == d->w;
-        break;
-    case wimp_EKEY:
-        /* Intercept all key events. */
-        /* IDJ:11-Jun-91: stop useless dboxes
-           stealing key presses */
-        if (e.data.key.c.w != d->w && !dbox__has_action_button(d))
-        {
-           harmless = 0;
-        }
+    while (1) {
+        /* keep going round until he presses a button (one may already be queued on entry) */
+        if(d->fieldwaiting)
+            {
+            result = dbox_get(d);
+            break;
+            }
+
+        if(wimpt_complain(wimp_get_caret_pos(&caret))) { result = dbox_CLOSE; break; }
+
+        if(d->caret_set)
+            { /*EMPTY*/ /* SKS 20130523 don't repeat */
+            }
+        else if(caret.w != w) /* SKS: only set caret pos if not in here */
+            {
+            if(dbox__findicon(d,
+                              (wimp_iconflags) (WRITABLE_IFLAGS | wimp_INOSELECT),
+                              (wimp_iconflags) (WRITABLE_IFLAGS), /* desired state of these flags */
+                              &j))
+                {
+                tracef1("setting caret in icon %i\n", j);
+
+                caret.w      = w;
+                caret.i      = j;
+                caret.x      = 0;
+                caret.y      = 0;
+                caret.height = -1;      /* calc x,y,h from icon/index */
+                caret.index  = dbox__fieldlength(d, j);
+
+                (void) wimpt_complain(wimp_set_caret_pos(&caret));
+
+                d->caret_set = TRUE;
+                }
+            else
+                tracef0("no writeable icons in dbox\n");
+            }
         else
-        {
-           if (e.data.key.c.w != d->w) {
-             e.data.key.c.w = d->w;
-             e.data.key.c.i = (wimp_i) -1;
-           }
-           harmless = 1;
-        }
-        break;
-    case wimp_EBUT:
-        harmless = e.data.but.m.w == d->w;
-        break;
+            tracef0("caret already in dbox\n");
 
-    case wimp_ESEND:
-    case wimp_ESENDWANTACK:
-#ifdef TRACE
-        { int *i;
-          tracef0("event in dbox:");
-          for (i = (int*) &e; i != 12 + (int*) &e; i++) {
-            tracef1(" %x", *i);
-          }
-          tracef0(".\n");
+#if TRACE
+        {
+        int i;
+        tracef3("[dbox_fillin(&%p (%d)) (sp ~= &%p) doing wimpt_poll]\n", d, w, &i);
         }
 #endif
+
+        wimpt_poll(event_getmask(), &e);
+
+        tracef1("[dbox_fillin got event %s]\n",
+                report_wimp_event(e.e, &e.data));
+
+        if(winx_submenu_query_is_submenu(w))
+            {
+            /* Check to see if the window has been closed.
+             * If it has then we are in a menu tree, and the wimp
+             * is doing things behind our backs.
+            */
+            if(winx_submenu_query_closed())
+                {
+                tracef0("--- menu dbox has been closed for us!\n");
+
+                wimpt_fake_event(&e); /* stuff event back in the queue */
+
+                /* On a redraw event you may not perform operations such
+                 * as the deletion of windows before actually servicing the
+                 * redraw. This caused the mysterious bug in ArcEdit such
+                 * that certain dboxes of the menu tree caused a repaint
+                 * of the entire window.
+                */
+                if(e.e == wimp_EREDRAW)
+                    event_process();
+
+                (void) wimpt_complain(winx_close_window(w));
+
+                result = dbox_CLOSE;
+                break;
+                }
+            else
+                {
+                tracef0("menu dbox is still open\n");
+
+                (void) wimpt_complain(wimp_get_wind_state(w, &ws));
+
+                if(0 == (ws.flags & wimp_WTOP))
+                    {
+                    trace_0(TRACE_OUT | TRACE_ANY, "menu dbox not at front!!!\n");
+                    /*SKS 08jan97 winx_send_front_window_request(w, FALSE);*/
+                    }
+                }
+            }
+
         harmless = TRUE;
-        if (e.data.msg.hdr.action == wimp_MPREQUIT) harmless = FALSE;
-        break;
-    default:
-        harmless = 0;
-    }
-    if (harmless) {
 
-#if TRACE
-  if (e.e != wimp_ENULL) tracef1("harmless event %i.\n", e.e);
-#endif
+        switch(e.e)
+            {
+            case wimp_EKEY:
+                /* Pass key events to submenu dbox */
+                if(winx_submenu_query_is_submenu(w) &&  (e.data.key.c.w != w))
+                    {
+                    e.data.key.c.w = w;
+                    e.data.key.c.i = (wimp_i) -1;
+                    }
+                break;
+
+            case wimp_ESEND:
+            case wimp_ESENDWANTACK:
+                /* prequit message must be marked as harmful! */
+                harmless = (e.data.msg.hdr.action != wimp_MPREQUIT);
+                /* >>>> Hum: potentially not true, but this is what the
+                 * menu mechanism does...
+                */
+                break;
+
+            case wimp_ENULL:
+            case wimp_EOPEN:
+            case wimp_ECLOSE:
+            case wimp_EREDRAW:
+            case wimp_EPTRENTER:
+            case wimp_EPTRLEAVE:
+            case wimp_EBUT:
+            case wimp_ESCROLL:
+            case wimp_EMENU:
+            case wimp_ELOSECARET:
+            case wimp_EGAINCARET:
+            case wimp_EUSERDRAG:
+            default:
+                break;
+            }
 
-      wimpt_fake_event(&e);
-      event_process();
-    }
-    else {
-      tracef1("event %i causes DBoxes.FillIn to give up.\n", e.e);
-      wimpt_fake_event(&e); /* stuff it back in the queue */
-      result = dbox_CLOSE;
-      break;
-    }
-    /* And keep going round until he presses a button. */
-    if (d -> fieldwaiting) {result = dbox_get((dbox) d); break;}
-  } /* loop */
+        wimpt_fake_event(&e); /* stuff it back in the queue in any case */
 
-  return(result);
+        if(!harmless)
+            {
+            tracef0("harmful event causes dbox_fillin to give up\n");
+            result = dbox_CLOSE;
+            break;
+            }
+
+        tracef0("process harmless event\n");
+        event_process();
+        /* and loop until we get a result */
+        }
+
+    if(result == dbox_CLOSE)
+        d->bits.sent_close = 1;
+
+    return(result);
 }
 
+#ifndef SKS_ACW
+
 dbox_field dbox_fillin(dbox d)
 {
   wimp_i j = 0;
@@ -996,6 +1622,16 @@
    return dbox_fillin_loop(d);
 }
 
+#else /* SKS_ACW */
+
+dbox_field dbox_fillin(dbox d)
+{
+  return dbox_fillin_loop(d);
+}
+
+#endif /* SKS_ACW */
+
+#ifndef SKS_ACW
 
 #ifndef UROM
 dbox_field dbox_popup(char *name, char *message)
@@ -1012,6 +1648,8 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 BOOL dbox_persist(void) {
   wimp_mousestr m;
   wimpt_noerr(wimp_get_point_info(&m));
@@ -1033,7 +1671,76 @@
 void dbox_init(void)
 {
 
+#ifndef SKS_ACW
   if (template_loaded() == FALSE)
       werr(0, msgs_lookup(MSGS_dbox2));
+#endif /* SKS_ACW */
 }
+
+#ifdef SKS_ACW
+
+/*
+SKS added for PipeDream
+*/
+
+extern BOOL
+dbox_adjusthit(dbox_field * fp, dbox_field a, dbox_field b, BOOL adjustclicked)
+{
+    BOOL res;
+    dbox_field f;
+
+    f = *fp;
+
+    res = ((f == a)  ||  (f == b));
+
+    if(res  &&  adjustclicked)
+        *fp = f ^ a ^ b;
+
+    return(res);
+}
+
+extern void
+dbox_motion_updates(dbox d)
+{
+    d->bits.motion_updates = 1;
+}
+
+extern void
+dbox_note_position_on_completion(BOOL f)
+{
+    note_position = f;
+}
+
+extern void
+dbox_noted_position_restore(void)
+{
+    noted_position = saved_position;
+}
+
+extern void
+dbox_noted_position_save(void)
+{
+    saved_position = noted_position;
+}
+
+extern void
+dbox_noted_position_trash(void)
+{
+    note_position = FALSE;
+
+    noted_position.noted = FALSE;
+}
+
+extern void
+dbox_raw_eventhandlers(dbox d, dbox_raw_handler_proc * handlerp, void ** handlep)
+{
+    if(handlerp)
+        *handlerp = d->raweventproc;
+
+    if(handlep)
+        *handlep  = d->raweventprochandle;
+}
+
+#endif /* SKS_ACW */
+
 /* end */
