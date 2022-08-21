--- _src	2021-12-31 14:59:30.430000000 +0000
+++ _dst	2022-08-18 16:55:46.840000000 +0000
@@ -32,6 +32,14 @@
  *
  */
 
+/*
+ *   03-Apr-89 SKS made window allocation stuff dynamic
+ *   07-Jun-91 SKS added pane handling facilities
+ *   27-Mar-92 SKS after PD 4.12 changed BAD_WIMP_Ws to 0
+*/
+
+#include "include.h" /* for SKS_ACW */
+
 #define BOOL int
 #define TRUE 1
 #define FALSE 0
@@ -53,6 +61,7 @@
 
 /* -------- Keeping Track of All Windows. -------- */
 
+#ifndef WIN_WINDOW_LIST
 /* At the moment, a simple linear array of all windows. */
 
 typedef struct {
@@ -67,9 +76,13 @@
 
 static win__str *win__allwindows; /*[MAXWINDOWS]*/
 static int win__lim = 0; /* first unused index of win__allwindows */
+#else
+#define DUD (-1)
+#endif /* WIN_WINDOW_LIST */
 
 static void win__register(wimp_w w, win_event_handler proc, void *handle)
 {
+#ifndef WIN_WINDOW_LIST
   int i;
   for (i=0;; i++) {
     if (i == win__lim)
@@ -96,10 +109,14 @@
   win__allwindows[i].proc = proc;
   win__allwindows[i].handle = handle;
   win__allwindows[i].menuh = 0;
+#else
+  winx__register_new(w, (winx_new_event_handler) proc, handle, 0 /* for old binaries */);
+#endif /* WIN_WINDOW_LIST */
 }
 
 static void win__discard(wimp_w w)
 {
+#ifndef WIN_WINDOW_LIST
   int i;
   for (i=0; i<win__lim; i++) {
     if (win__allwindows[i].w == w) {
@@ -111,16 +128,41 @@
     /* decrease the limit if you can. */
     win__lim--;
   };
+#else
+  win__str *pp = (win__str *) &win__window_list;
+  win__str *cp;
+
+  while ((cp = pp->link) != NULL) {
+    if (cp->window_handle != w) {
+      pp = cp;
+      continue;
+    }
+
+    myassert1x(cp->children == NULL, "win__deregister(&%p): window still has children", w);
+    pp->link = cp->link;
+    free(cp);
+    return;
+  }
+#endif /* WIN_WINDOW_LIST */
 }
 
 static win__str *win__find(wimp_w w)
 {
+#ifndef WIN_WINDOW_LIST
   int i;
   for (i=0; i<win__lim; i++) {
     if (win__allwindows[i].w == w) {
       return(&win__allwindows[i]);
     };
   };
+#else
+  win__str *p = (win__str *) &win__window_list;
+  while ((p = p->link) != NULL) {
+    if (p->window_handle == w) {
+       return(p);
+    }
+  }
+#endif /* WIN_WINDOW_LIST */
   return(0);
 }
 
@@ -143,8 +185,8 @@
  void *handle ;
 } unknown_previewer ;
 
-static wimp_w win__idle = DUD;
-#define win__unknown_flag (('U'<<24)+('N'<<16)+('K'<<8)+'N')
+static wimp_w win__idle = win_IDLE_OFF;
+#define win__unknown_flag ((wimp_w) (('U'<<24)+('N'<<16)+('K'<<8)+'N'))
 
 static wimp_w win__unknown = DUD;
 static unknown_previewer *win__unknown_previewer_list = 0 ;
@@ -193,7 +235,7 @@
 }
 
 void win_remove_unknown_event_processor(win_unknown_event_processor p,
-                                        void *h)
+                                        void * h)
 {
    unknown_previewer *b, **pb;
 
@@ -232,7 +274,30 @@
   wimp_w w;
   win__str *p;
 
-  switch (e->e) {
+  if(e->e != Wimp_ERedrawWindow)
+  {
+    /* note that the Window Manager cannot cope with any window opening etc issued
+     * between the receipt of the Wimp_ERedrawWindow and the SWI Wimp_RedrawWindow
+    */
+    if(winx_statics.submenu_window_handle != 0)
+    {
+      if(winx_submenu_query_closed())
+      {
+        /* Window Manager must have gone and closed the window the sly rat */
+        /* so send our client a close request NOW */
+        wimp_eventstr ev;
+
+        ev.e = wimp_ECLOSE;
+        ev.data.close.w = winx_statics.submenu_window_handle;
+
+        winx_statics.submenu_window_handle = 0;
+
+        (void) win_processevent(&ev);
+      }
+    }
+  }
+
+  switch(e->e) {
 
     case wimp_ENULL:
       w = win__idle;
@@ -240,11 +305,18 @@
 
     case wimp_EUSERDRAG:
       w = win__unknown_flag ;
+      if(winx_statics.drag_window_handle != 0)
+      { /* user drag comes but once per winx_drag_box */
+        w = winx_statics.drag_window_handle;
+        winx_statics.drag_window_handle = 0;
+      }
       break;
 
     case wimp_EREDRAW: case wimp_ECLOSE: case wimp_EOPEN:
     case wimp_EPTRLEAVE: case wimp_EPTRENTER: case wimp_EKEY:
     case wimp_ESCROLL:
+    case wimp_EGAINCARET:
+    case wimp_ELOSECARET:
       w = e->data.o.w;
       break;
 
@@ -257,14 +329,10 @@
     case wimp_ESENDWANTACK:
     /* Some standard messages we understand, and give to the right guy. */
       switch (e->data.msg.hdr.action) {
-        case wimp_MCLOSEDOWN:
-          tracef0("closedown message: for the moment, just do it.\n");
-          exit(0);
-
         case wimp_MDATALOAD:
         case wimp_MDATASAVE:
           tracef1("data %s message arriving.\n",
-             (int) (e->data.msg.hdr.action==wimp_MDATASAVE ? "save" : "load"));
+             (int) (e->data.msg.hdr.action == wimp_MDATASAVE ? "save" : "load"));
 
           /* Note that we're assuming the window handle's the same position in
              both messages */
@@ -283,18 +351,24 @@
           if (w < 0) w = win_ICONBARLOAD;
           break;
 
+        case wimp_MWINDOWINFO:
+          w = e->data.msg.data.windowinfo.w;
+          break;
+
         default:
-            tracef1("unknown message type %i arriving.\n", e->data.msg.hdr.action);
+            tracef1("unknown message arriving: %s\n", report_wimp_message(&e->data.msg, FALSE));
             w = win__unknown_flag;
             if (w < 0) w = win_ICONBARLOAD;
+            break;
       }
       break;
 
     default:
       w = win__unknown_flag;
+      break;
   }
 
-  if (w==win__unknown_flag || win__find(w) == 0)
+  if (w==win__unknown_flag)
   {
    unknown_previewer *pr ;
    for (pr = win__unknown_previewer_list; pr != 0; pr = pr->link)
@@ -305,15 +379,21 @@
    w = win__unknown ;
   }
 
-  p = (w == DUD ? 0 : win__find(w));
+  p = ((w == DUD) ? 0 : win__find(w));
   if (p != 0) {
-    p->proc(e, p->handle);
-    return TRUE;
+      BOOL processed = (* p->proc) (e, p->handle); /* may be garbage result if called an old void-return binary */
+
+      if(!p->new_proc)
+        processed = TRUE;
+
+      return(processed);
   } else {
     return FALSE;
   }
 }
 
+#ifndef SKS_ACW
+
 /* -------- Termination. -------- */
 
 static int win__active = 0;
@@ -378,41 +458,64 @@
   return(FALSE);
 }
 
+#endif /* SKS_ACW */
+
 /* ----------- setting a window title ------------ */
 
 void win_settitle(wimp_w w, char *newtitle)
 {
   wimp_redrawstr r;
   wimp_winfo *winfo;
+  char *str;
+  size_t count;
 
-  if((winfo = malloc(sizeof(wimp_wind) + 200*sizeof(wimp_icon))) == 0)
-    werr(TRUE, msgs_lookup(MSGS_win2));
+  /* SKS - no malloc() needed */
+#define MAX_N_ICONS 200
+  char winfo_buf[sizeof(wimp_winfo) + MAX_N_ICONS * sizeof(wimp_icon)]; /*BPDM!*/
+    winfo = (wimp_winfo *) &winfo_buf;
 
   /* --- get the window's details --- */
     winfo->w = w;
-    wimpt_noerr(wimp_get_wind_info(winfo));
+    if(NULL != wimp_get_wind_info(winfo))
+        return;
+
+    myassert1x(winfo->info.nicons <= MAX_N_ICONS, "win_settitle: reading info for %d icons corrupted stack", winfo->info.nicons);
 
   /* --- put the new title string in the title icon's buffer --- */
-    strcpy(winfo->info.title.indirecttext.buffer, newtitle);
+    if((winfo->info.titleflags & wimp_INDIRECT) == 0)
+    {
+        myassert0x(0, "win_settitle: non-indirected title bar icon");
+        return;
+    }
+    str   = winfo->info.title.indirecttext.buffer;
+    count = winfo->info.title.indirecttext.bufflen;
+    *str  = CH_NULL;
+    strncat(str, newtitle, count - 1);
 
   /* --- invalidate the title bar in absolute coords --- */
     r.w = (wimp_w) -1;    /* absolute screen coords */
     r.box = winfo->info.box;
-    r.box.y1 += 36;            /* tweak */
-    r.box.y0 = r.box.y1 - 36;
-    wimpt_noerr(wimp_force_redraw(&r));
+    if(winfo->info.flags & wimp_WOPEN) /* only if open! */
+    {
+        const int dy = wimpt_dy();
+        r.box.y0 = r.box.y0 + dy; /* title bar starts one raster up */
+        r.box.y1 = r.box.y0 + wimptx_title_height() - 2*dy;
+        (void) wimpt_complain(wimp_force_redraw(&r));
+    }
 
   /* --- free space used to window info --- */
-    free(winfo);
+  /* SKS - no free() needed */
 }
 
 BOOL win_init(void)
 {
+#ifndef WIN_WINDOW_LIST
     if (win__allwindows == 0)
     {
         return((win__allwindows = malloc(MAXWINDOWS*sizeof(win__str)))!=0);
     }
     else
+#endif
         return TRUE;
 }
 
