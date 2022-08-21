--- _src	2021-12-31 14:59:30.010000000 +0000
+++ _dst	2022-08-18 16:55:46.590000000 +0000
@@ -73,21 +73,26 @@
   menu m;
   event_menu_maker maker;
   event_menu_proc event;
+  event_menu_ws_proc event_ws_proc;
   void *handle;
 } mstr;
 
-static BOOL event__attach(event_w w,
-                          menu m, event_menu_maker menumaker,
-                          event_menu_proc eventproc, void *handle)
+static BOOL event__attach(_HwndRef_ HOST_WND w /*window_handle*/,
+                          int i /*icon_handle*/,
+                          menu m,
+                          event_menu_maker menumaker,
+                          event_menu_proc eventproc,
+                          event_menu_ws_proc event_ws_proc,
+                          void *handle)
 {
-  mstr *p = win_getmenuh(w);
+  mstr *p = winx_menu_get_handle(w, i);
   if (m == 0 && menumaker == 0)
   {
     /* cancelling */
     if (p != 0)
     { /* something to cancel */
       free(p);
-      win_setmenuh(w, 0);
+      winx_menu_set_handle(w, i, NULL);
     }
   }
   else
@@ -97,12 +102,15 @@
     p->m = m;
     p->maker = menumaker;
     p->event = eventproc;
+    p->event_ws_proc = event_ws_proc;
     p->handle = handle;
-    win_setmenuh(w, p);
+    winx_menu_set_handle(w, i, p);
   }
   return TRUE;
 }
 
+#ifndef SKS_ACW
+
 BOOL event_attachmenu(event_w w, menu m, event_menu_proc eventproc, void* handle)
 {
   return event__attach(w, m, 0, eventproc, handle);
@@ -113,8 +121,39 @@
   return event__attach(w, 0, menumaker, eventproc, handle);
 }
 
+#else /* SKS_ACW */
+
+BOOL event_attachmenu(event_w w, menu m, event_menu_proc eventproc, void *handle)
+{
+  return event__attach(w, -1, m, NULL, eventproc, NULL, handle);
+}
+
+BOOL event_attachmenumaker(event_w w, event_menu_maker menumaker, event_menu_proc eventproc, void *handle)
+{
+  return event__attach(w, -1, NULL, menumaker, eventproc, NULL, handle);
+}
+
+BOOL
+event_attachmenumaker_to_w_i(
+    _HwndRef_   HOST_WND window_handle,
+    _InVal_     int icon_handle,
+    event_menu_maker menumaker,
+    event_menu_ws_proc event_ws_proc,
+    void * handle)
+{
+    return event__attach((event_w) window_handle, icon_handle, NULL, menumaker, NULL, event_ws_proc, handle);
+}
+
+#endif /* SKS_ACW */
+
 /* -------- Processing Events. -------- */
 
+#ifdef SKS_ACW
+
+static menu event__current_menu;
+
+#else /* NOT SKS_ACW */
+
 static wimp_w event__current_menu_window = 0;
   /* 0 if no menu currently visible */
 static menu event__current_menu;
@@ -124,89 +163,243 @@
 static BOOL event__last_was_menu_hit = FALSE;
 static BOOL event__recreation;
 
-static BOOL event__process(wimp_eventstr *e)
+#endif /* SKS_ACW */
+
+static BOOL event__recreation;
+
+#ifdef SKS_ACW
+
+static void event__createmenu(BOOL recreate)
+{
+    mstr *          p = NULL;
+    wimp_menuhdr *  m;
+    wimp_menuitem * mi;
+
+    if(event_statics.menuclick.window_handle != win_ICONBAR)
+        p = winx_menu_get_handle(event_statics.menuclick.window_handle, event_statics.menuclick.icon_handle);
+
+    if(!p)
+        p = win_getmenuh(event_statics.menuclick.window_handle);
+
+    if(p)
+        {
+        event__recreation = recreate;
+        event__current_menu = p->m;
+        if(p->m)
+            m = (wimp_menuhdr *) menu_syshandle(p->m);
+        else if(!TRACE  ||  p->maker)
+            {
+            event__current_menu = p->maker(p->handle);
+            m = (wimp_menuhdr *) menu_syshandle(event__current_menu);
+            }
+        else
+            {
+            werr(FALSE, "registered menu with no means of creation!!!\n");
+            m = NULL;
+            }
+
+        /* allow icon bar recreates not to restore menu position */
+        if((event_statics.menuclick.window_handle == win_ICONBAR)  &&  !recreate)
+            {
+            /* move icon bar menus up to standard position. */
+            mi = (wimp_menuitem *) (m + 1);
+
+            event_statics.menuclick.y = 96;
+
+            tracef0("positioning icon bar menu.\n");
+            do  {
+                event_statics.menuclick.y += m->height + m->gap;
+                }
+            while(!((mi++)->flags & wimp_MLAST));
+            }
+
+        (void) wimpt_complain(event_create_menu((wimp_menustr *) m, event_statics.menuclick.x - 64, event_statics.menuclick.y));
+        }
+    else
+        tracef0("no registered menu\n");
+}
+
+/*
+SKS: process event: returns true if idle
+*/
+
+#define event__process event_do_process
+
+#endif /* SKS_ACW */
+
+extern BOOL
+event__process(wimp_eventstr * e)
 {
+  wimp_msgstr *m = &e->data.msg;
+  wimp_mousestr ms;
+  char hit[20];
+  int i;
+  BOOL submenu_fake_hit = FALSE;
+
+  tracef1("event__process(%s)\n", report_wimp_event(e->e, &e->data));
+
   /* Look for submenu requests, and if found turn them into menu hits. */
   /* People wishing to respond can pick up the original from wimpt. */
   if (e->e == wimp_ESEND && e->data.msg.hdr.action == wimp_MMENUWARN)
   {
-    int i;
+    /* A hit on the submenu pointer may be interpreted as slightly different
+     * from actually clicking on the menu entry with the arrow. The most
+     * important example of this is the standard "save" menu item, where clicking
+     * on "Save" causes a save immediately while following the arrow gets
+     * the standard save dbox up.
+    */
+    tracef0("hit on submenu => pointer, fake menu hit\n");
+
+    /* cache submenu opening info for use later on this event */
+    event_statics.submenu.m = m->data.menuwarning.submenu.m;
+    event_statics.submenu.x = m->data.menuwarning.x;
+    event_statics.submenu.y = m->data.menuwarning.y;
+
     e->e = wimp_EMENU;
     i = 0;
-    while ((e->data.menu[i] = e->data.msg.data.words[i+3]) != -1) i++;
-    e->data.menu[i++] = 0;
-    e->data.menu[i++] = -1;
+    do  {
+        e->data.menu[i] = m->data.menuwarning.menu[i];
+    } while(e->data.menu[i++] != -1);
+
+    submenu_fake_hit = TRUE;
   }
 
   /* Look for menu events */
-  if (e->e == wimp_EBUT && (wimp_BMID & e->data.but.m.bbits) != 0)
+  if (e->e == Wimp_EMouseClick)
   {
-    /* set up a menu! */
-    mstr *p;
-    if (e->data.but.m.w <= -1) e->data.but.m.w = win_ICONBAR;
-    p = win_getmenuh(e->data.but.m.w);
-    if (p != 0)
+    HOST_WND window_handle = e->data.but.m.w;
+    int icon_handle = e->data.but.m.i;
+    mstr * p = NULL;
+
+    if(window_handle < 0)
+        window_handle = win_ICONBAR;
+
+    /* menu on an icon can give a drag as the pointer leaves the window!
+     * so Mr. Paranoid masks out all unreasonable button hits
+    */
+    if(e->data.but.m.bbits & (wimp_BLEFT | wimp_BMID /*SKS 29sep96 | wimp_BRIGHT*/))
     {
-      wimp_menuhdr *m;
-      event__current_menu_window = e->data.but.m.w;
-      event__current_menu = p->m;
-      event__current_destroy_after = FALSE;
-      if (p->m != 0)
-      {
-        m = (wimp_menuhdr*) menu_syshandle(p->m);
-      }
-      else if (p->maker != 0)
+        if(e->data.but.m.bbits & wimp_BMID)
+        {
+            /* don't use Menu to get menus for registered to icons; fake work area */
+            icon_handle = -1;
+
+            if(!p)
+                p = win_getmenuh(window_handle);
+        }
+        else if(icon_handle != -1)
+            /* look for menu registered to that icon */
+            p = winx_menu_get_handle(window_handle, icon_handle);
+    }
+
+    if(p)
+    {
+      event_statics.menuclick.window_handle = window_handle;
+      event_statics.menuclick.icon_handle = icon_handle;
+      event_statics.menuclick.x = e->data.but.m.x;
+      event_statics.menuclick.y = e->data.but.m.y;
+      event_statics.menuclick.valid = TRUE;
+      event__createmenu(FALSE);
+      event_statics.menuclick.valid = FALSE;
+      tracef0("menu created\n");
+      return(FALSE);
+    }
+  }
+
+
+  if((e->e == wimp_EMENU)  &&  event__current_menu != 0)
+  {
+    /* Menu hit */
+    mstr * p = NULL;
+
+    if(event_statics.menuclick.window_handle != win_ICONBAR)
+       p = winx_menu_get_handle(event_statics.menuclick.window_handle, event_statics.menuclick.icon_handle);
+
+    if(!p)
+       p = win_getmenuh(event_statics.menuclick.window_handle);
+
+    tracef0("menu hit ");
+
+    if(p)
+    {
+      if(!submenu_fake_hit)
       {
-        event__current_destroy_after = FALSE;
-        event__recreation = 0 ;
-        event__current_menu = p->maker(p->handle);
-        m = (wimp_menuhdr*) menu_syshandle(event__current_menu);
+        if(wimpt_complain(wimp_get_point_info(&ms))) { ms.bbits = (wimp_bbits) 0; }
+        event_statics.recreatepending = ((ms.bbits & wimp_BRIGHT) == wimp_BRIGHT);
       }
       else
       {
-        m = (wimp_menuhdr *) -1;
+        /* say the submenu opening cache is valid */
+        event_statics.submenu.valid = TRUE;
+        event_statics.recreatepending = FALSE;
       }
-      if (event__current_menu_window == win_ICONBAR)
+
+      /* form array of menu hits ending in 0 */
+      i = 0;
+      do  {
+          hit[i] = e->data.menu[i] + 1; /* convert hit on 0 to 1 etc. */
+          tracef1("[%d]", hit[i]);
+      } while(e->data.menu[i++] != -1);
+
+      tracef1(", Adjust = %s\n", report_boolstring(event_statics.recreatepending));
+
+      /* allow access to initial click cache during handler */
+      event_statics.menuclick.valid = TRUE;
+
+      if(p->event_ws_proc)
       {
-        /* move icon bar menus up a bit. */
-        e->data.but.m.x -= 16 /* m->width/2 */;
-        e->data.but.m.y = 96;
+        if(! (* p->event_ws_proc) (p->handle, hit, submenu_fake_hit))
         {
-          wimp_menuitem *mi = (wimp_menuitem*) (m + 1);
-          while (1) {
-            e->data.but.m.y += m->height + m->gap;
-            if ((mi->flags & wimp_MLAST) != 0) break;
-            mi++;
+          if(submenu_fake_hit)
+          { /* handle unprocessed submenu open events */
+            int x, y;
+            event_read_submenupos(&x, &y);
+            (void) wimpt_complain(event_create_submenu(event_read_submenudata(), x, y));
           }
         }
       }
-      event__menux = e->data.but.m.x;
-      event__menuy = e->data.but.m.y;
-      wimpt_complain(wimp_create_menu((wimp_menustr*) m, e->data.but.m.x - 48, e->data.but.m.y));
-      return FALSE;
-    }
-  }
-  else if (e->e == wimp_EMENU && event__current_menu != 0)
-  {
-    mstr *p;
-    p = win_getmenuh(event__current_menu_window);
-    if (p == 0)
-    {
-      /* if the menu registration has been cancelled,
-      we hand the event on. */
+      else
+      {
+        (* p->event) (p->handle, hit);
+
+        if(submenu_fake_hit)
+        { /* handle unprocessed submenu open events */
+          int x, y;
+          event_read_submenupos(&x, &y);
+          (void) wimpt_complain(event_create_submenu(event_read_submenudata(), x, y));
+        }
+      }
+
+      /* submenu opening cache no longer valid */
+      event_statics.submenu.valid = FALSE;
+
+      if(event_statics.recreatepending)
+      {
+        /* Twas an ADJ-hit on a menu item.
+         * The menu should be recreated.
+        */
+        tracef0("menu hit caused by Adjust - recreating menu\n");
+        event_statics.menuclick.valid = TRUE;
+        event__createmenu(TRUE);
+      }
+#if TRACE
+      else if(submenu_fake_hit)
+        tracef0("menu hit was faked\n");
+      else
+        tracef0("menu hit caused by Select - let tree collapse\n");
+#endif
+
+      /* initial click cache no longer valid */
+      event_statics.menuclick.valid = FALSE;
+
+      return(FALSE);
     }
     else
-    {
-      int i;
-      char a[20];
-      for (i = 0; e->data.menu[i] != -1; i++) {a[i] = 1 + e->data.menu[i];}
-      a[i] = 0;
-      p->event(p->handle, a);
-      event__last_was_menu_hit = TRUE; /* menu has still to be deleted. */
-      return FALSE;
-    }
+      tracef0("- no registered handler\n");
   }
 
+
+#ifndef SKS_ACW
   if (e->e == wimp_ENULL)
   {
     int dummy_time;
@@ -221,12 +414,14 @@
     if ((event_getmask() & wimp_EMNULL) != 0)
        return TRUE;
   }
+#endif /* SKS_ACW */
 
   /* now process the event */
   if (win_processevent(e))
   {
     /* all is well, it was claimed */
   }
+#ifndef SKS_ACW
   else if (e->e == wimp_ENULL)
   {
     /* machine idle: say so */
@@ -237,12 +432,20 @@
     /* Assume it's a menu being moved */
     wimpt_complain(wimp_open_wind(&e->data.o));
   }
+#else /* SKS_ACW */
+  else
+  {
+    return(event__default_process(e->e, (WimpPollBlock *) &e->data));
+  }
+#endif /* SKS_ACW */
 
   return FALSE;
 }
 
 static void event__poll(wimp_emask mask, wimp_eventstr *result)
 {
+#ifndef SKS_ACW
+
   if (event__last_was_menu_hit && wimpt_last_event()->e == wimp_EMENU)
   {
     wimp_mousestr m;
@@ -273,6 +476,8 @@
       }
     }
   }
+#endif /* SKS_ACW */
+
   tracef0("doing poll.\n");
   wimpt_complain(wimpt_poll(mask, result));
   tracef0("poll done.\n");
@@ -281,7 +486,9 @@
 void event_process(void)
 {
   tracef0("event_process.\n");
+#ifndef SKS_ACW
   if (win_activeno() == 0) exit(0); /* stop program */
+#endif
   {
     wimp_eventstr e;
     event__poll(event_getmask(), &e);
@@ -289,6 +496,8 @@
   }
 }
 
+#ifndef SKS_ACW
+
 #ifndef UROM
 BOOL event_anywindows()
 {
@@ -296,8 +505,15 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 void event_clear_current_menu(void) {
+#ifdef SKS_ACW
+  /* NB not wimpt_noerr(), and use our new function */
+  (void) wimpt_complain(event_create_menu((wimp_menustr *) -1, 0, 0));
+#else /* NOT SKS_ACW */
   wimpt_noerr(wimp_create_menu((wimp_menustr*) -1, 0, 0));
+#endif /* SKS_ACW */
 }
 
 BOOL event_is_menu_being_recreated(void)
