--- _src	2003-01-16 13:43:31 +0100
+++ _dst	2013-09-06 13:18:24 +0100
@@ -76,21 +76,26 @@
   menu m;
   event_menu_maker maker;
   event_menu_proc event;
+  event_menu_ws_proc event_ws_proc;
   void *handle;
 } mstr;
 
 static BOOL event__attach(event_w w,
-                          menu m, event_menu_maker menumaker,
-                          event_menu_proc eventproc, void *handle)
+                          wimp_i i,
+                          menu m,
+                          event_menu_maker menumaker,
+                          event_menu_proc eventproc,
+                          event_menu_ws_proc event_ws_proc,
+                          void *handle)
 {
-  mstr *p = win_getmenuh(w);
+  mstr *p = win_getmenuhi(w, i);
   if (m == 0 && menumaker == 0)
   {
     /* cancelling */
     if (p != 0)
     { /* something to cancel */
       free(p);
-      win_setmenuh(w, 0);
+      win_setmenuhi(w, i, NULL);
     }
   }
   else
@@ -100,12 +105,15 @@
     p->m = m;
     p->maker = menumaker;
     p->event = eventproc;
+    p->event_ws_proc = event_ws_proc;
     p->handle = handle;
-    win_setmenuh(w, p);
+    win_setmenuhi(w, i, p);
   }
   return TRUE;
 }
 
+#ifndef SKS_ACW
+
 BOOL event_attachmenu(event_w w, menu m, event_menu_proc eventproc, void* handle)
 {
   return event__attach(w, m, 0, eventproc, handle);
@@ -116,8 +124,64 @@
   return event__attach(w, 0, menumaker, eventproc, handle);
 }
 
+#else /* SKS_ACW */
+
+BOOL event_attachmenu(event_w w, menu m, event_menu_proc eventproc, void *handle)
+{
+  return event__attach(w, (wimp_i) -1, m, NULL, eventproc, NULL, handle);
+}
+
+BOOL event_attachmenumaker(event_w w, event_menu_maker menumaker, event_menu_proc eventproc, void *handle)
+{
+  return event__attach(w, (wimp_i) -1, NULL, menumaker, eventproc, NULL, handle);
+}
+
+BOOL
+event_attachmenumaker_to_w_i(
+    wimp_w w,
+    wimp_i i,
+    event_menu_maker menumaker,
+    event_menu_ws_proc event_ws_proc,
+    void * handle)
+{
+    return event__attach(w, i, NULL, menumaker, NULL, event_ws_proc, handle);
+}
+
+#endif /* SKS_ACW */
+
 /* -------- Processing Events. -------- */
 
+#ifdef SKS_ACW
+
+static menu event__current_menu;
+
+static struct _event_statics
+{
+    BOOL       recreatepending;
+
+    struct _event_menu_click_cache_data
+        {
+        BOOL   valid;
+        int    x;
+        int    y;
+        wimp_w w;
+        wimp_i i;
+        }
+    menuclick;
+
+    struct _event_submenu_opening_cache_data
+        {
+        BOOL          valid;
+        wimp_menuptr  m;
+        int           x;
+        int           y;
+        }
+    submenu;
+}
+event_;
+
+#else /* NOT SKS_ACW */
+
 static wimp_w event__current_menu_window = 0;
   /* 0 if no menu currently visible */
 static menu event__current_menu;
@@ -127,89 +191,243 @@
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
 {
+    mstr *          p = NULL;
+    wimp_menuhdr *  m;
+    wimp_menuitem * mi;
+
+    if(event_.menuclick.w != win_ICONBAR)
+        p = win_getmenuhi(event_.menuclick.w, event_.menuclick.i);
+
+    if(!p)
+        p = win_getmenuh(event_.menuclick.w);
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
+        if((event_.menuclick.w == win_ICONBAR)  &&  !recreate)
+            {
+            /* move icon bar menus up to standard position. */
+            mi = (wimp_menuitem *) (m + 1);
+
+            event_.menuclick.y = 96;
+
+            tracef0("positioning icon bar menu.\n");
+            do  {
+                event_.menuclick.y += m->height + m->gap;
+                }
+            while(!((mi++)->flags & wimp_MLAST));
+            }
+
+        wimpt_complain(event_create_menu((wimp_menustr *) m, event_.menuclick.x - 64, event_.menuclick.y));
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
+{
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
+    event_.submenu.m = m->data.menuwarning.submenu.m;
+    event_.submenu.x = m->data.menuwarning.x;
+    event_.submenu.y = m->data.menuwarning.y;
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
+  if (e->e == wimp_EBUT)
   {
-    /* set up a menu! */
-    mstr *p;
-    if (e->data.but.m.w <= -1) e->data.but.m.w = win_ICONBAR;
-    p = win_getmenuh(e->data.but.m.w);
-    if (p != 0)
+    wimp_w w = e->data.but.m.w;
+    wimp_i i = e->data.but.m.i;
+    mstr * p = NULL;
+
+    if((int) w < 0)
+        w = win_ICONBAR;
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
+            /* don't use MENU to get menus for registered to icons; fake work area */
+            i = (wimp_i) -1;
+
+            if(!p)
+                p = win_getmenuh(w);
+        }
+        else if(i != (wimp_i) -1)
+            /* look for menu registered to that icon */
+            p = win_getmenuhi(w, i);
+    }
+
+    if(p)
+    {
+      event_.menuclick.w = w;
+      event_.menuclick.i = i;
+      event_.menuclick.x = e->data.but.m.x;
+      event_.menuclick.y = e->data.but.m.y;
+      event_.menuclick.valid = TRUE;
+      event__createmenu(FALSE);
+      event_.menuclick.valid = FALSE;
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
+    if(event_.menuclick.w != win_ICONBAR)
+       p = win_getmenuhi(event_.menuclick.w, event_.menuclick.i);
+
+    if(!p)
+       p = win_getmenuh(event_.menuclick.w);
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
+        wimpt_safe(wimp_get_point_info(&ms));
+        event_.recreatepending = ((ms.bbits & wimp_BRIGHT) == wimp_BRIGHT);
       }
       else
       {
-        m = (wimp_menuhdr *) -1;
+        /* say the submenu opening cache is valid */
+        event_.submenu.valid = TRUE;
+        event_.recreatepending = FALSE;
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
+      tracef1(", ADJUST = %s\n", trace_boolstring(event_.recreatepending));
+
+      /* allow access to initial click cache during handler */
+      event_.menuclick.valid = TRUE;
+
+      if(p->event_ws_proc)
       {
-        /* move icon bar menus up a bit. */
-        e->data.but.m.x -= 16 /* m->width/2 */;
-        e->data.but.m.y = 96;
-        {
-          wimp_menuitem *mi = (wimp_menuitem*) (m + 1);
-          while (1) {
-            e->data.but.m.y += m->height + m->gap;
-            if ((mi->flags & wimp_MLAST) != 0) break;
-            mi++;
+        if(! (* p->event_ws_proc) (p->handle, hit, submenu_fake_hit))
+        {
+          if(submenu_fake_hit)
+          { /* handle unprocessed submenu open events */
+            int x, y;
+            event_read_submenupos(&x, &y);
+            wimpt_complain(event_create_submenu(event_read_submenudata(), x, y));
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
+          wimpt_complain(event_create_submenu(event_read_submenudata(), x, y));
+        }
+      }
+
+      /* submenu opening cache no longer valid */
+      event_.submenu.valid = FALSE;
+
+      if(event_.recreatepending)
+      {
+        /* Twas an ADJ-hit on a menu item.
+         * The menu should be recreated.
+        */
+        tracef0("menu hit caused by ADJUST - recreating menu\n");
+        event_.menuclick.valid = TRUE;
+        event__createmenu(TRUE);
+      }
+#if TRACE
+      else if(submenu_fake_hit)
+        tracef0("menu hit was faked\n");
+      else
+        tracef0("menu hit caused by SELECT - let tree collapse\n");
+#endif
+
+      /* initial click cache no longer valid */
+      event_.menuclick.valid = FALSE;
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
@@ -224,12 +442,14 @@
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
@@ -240,12 +460,20 @@
     /* Assume it's a menu being moved */
     wimpt_complain(wimp_open_wind(&e->data.o));
   }
+#else /* SKS_ACW */
+  else
+  {
+    return(event__default_process(e));
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
@@ -276,6 +504,8 @@
       }
     }
   }
+#endif /* SKS_ACW */
+
   tracef0("doing poll.\n");
   wimpt_complain(wimpt_poll(mask, result));
   tracef0("poll done.\n");
@@ -284,7 +514,9 @@
 void event_process(void)
 {
   tracef0("event_process.\n");
+#ifndef SKS_ACW
   if (win_activeno() == 0) exit(0); /* stop program */
+#endif
   {
     wimp_eventstr e;
     event__poll(event_getmask(), &e);
@@ -292,6 +524,8 @@
   }
 }
 
+#ifndef SKS_ACW
+
 #ifndef UROM
 BOOL event_anywindows()
 {
@@ -299,8 +533,15 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 void event_clear_current_menu(void) {
+#ifdef SKS_ACW
+  /* NB not wimpt_noerr(), and use our new function */
+  wimpt_complain(event_create_menu((wimp_menustr *) -1, 0, 0));
+#else /* NOT SKS_ACW */
   wimpt_noerr(wimp_create_menu((wimp_menustr*) -1, 0, 0));
+#endif /* SKS_ACW */
 }
 
 BOOL event_is_menu_being_recreated(void)
