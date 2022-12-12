--- _src	2022-10-26 16:47:18.800000000 +0000
+++ _dst	2022-10-31 14:11:48.130000000 +0000
@@ -69,13 +69,22 @@
 
 os_error * wimpt_poll(wimp_emask mask, wimp_eventstr * result)
 {
+#ifdef SKS_ACW
+  tracef3("\n[wimpt_poll(&%X, &%p): fake_waiting = %s]\n", mask, result, report_boolstring(wimpt__fake_waiting));
+  g_host_must_die = FALSE;
+#endif /* SKS_ACW */
+
   if (wimpt__fake_waiting != 0) {
     *result = wimpt__fake_event;
     wimpt__fake_waiting = 0;
+#ifndef SKS_ACW
     wimpt__last_event = wimpt__fake_event;
+#endif /* SKS_ACW */
     return(0);
   } else {
     os_error *r;
+
+#ifndef SKS_ACW
     int next_alarm_time;
     if (alarm_next(&next_alarm_time) != 0 && ((event_getmask() & wimp_EMNULL))!=0)
     {
@@ -84,11 +93,36 @@
       r = wimp_pollidle(mask & ~wimp_EMNULL, result, next_alarm_time);
     }
     else
+#else /* SKS_ACW */
+    if(wimptx__atexitproc) /* SKS pragmatic fix - make general in due course */
+      wimptx__atexitproc(result);
+#endif /* SKS_ACW */
+
+    if(win_idle_event_claimer() != win_IDLE_OFF)
+    {
+      /* ensure we get null events */
+      mask = (wimp_emask) (mask & ~Wimp_Poll_NullMask);
+      tracef1("[wimpt_poll must get idle requests: mask := %X]\n", mask);
+    }
+
+    for(;;)
     {
       tracef0("Polling busy\n");
-      tracef2("Mask = %d   %d\n", mask, event_getmask());
-      r = wimp_poll(mask, result);
+      tracef2("[calling wimp_poll_coltsoft(&%X, &%p)]\n", mask, result);
+      if(NULL == (r = wimp_poll_coltsoft(mask, &result->data, NULL, (int *) &result->e)))
+          break;
+
+      tracef0("--- ERROR returned from wimp_poll()!!! (Ta Neil)\n");
+      (void) wimpt_complain(r);
     }
+
+#ifdef SKS_ACW
+    /*if(wimptx__atentryproc)*/ /* SKS pragmatic fix - make general in due course */
+        /*wimptx__atentryproc(result);*/
+
+    tracef1("[wimpt_poll returns real event %s]\n", report_wimp_event(result->e, &result->data));
+#endif /* SKS_ACW */
+
     wimpt__last_event = *result;
     return(r);
   }
@@ -116,11 +150,22 @@
 
 /* -------- Control of graphics environment -------- */
 
+#ifndef SKS_ACW
+
 static int wimpt__mode = 12;
 static int wimpt__dx;
 static int wimpt__dy;
 static int wimpt__bpp;
 
+#else /* SKS_ACW */
+
+int wimpt__mode  = 12; /* expose this set for fast access via macros */
+int wimpt__dx    = 2;
+int wimpt__dy    = 4;
+int wimpt__bpp   = 1;
+
+#endif /* SKS_ACW */
+
 static int wimpt__read_screen_mode(void)
 {
   int x, y;
@@ -143,24 +188,48 @@
   if (old != wimpt__mode) changed = TRUE;
 
   old = wimpt__dx;
+#ifndef SKS_ACW
   wimpt__dx = 1 << bbc_vduvar(bbc_XEigFactor);
+#else /* SKS_ACW - cache EIG and size too */
+  wimptx__XEigFactor  = bbc_vduvar(bbc_XEigFactor);
+  wimpt__dx = 1 << wimptx__XEigFactor;
+  wimptx__display_size_cx = (bbc_vduvar(bbc_XWindLimit) + 1) << wimptx__XEigFactor;
+#endif /* SKS_ACW */
   if (old != wimpt__dx) changed = TRUE;
 
   old = wimpt__dy;
+#ifndef SKS_ACW
   wimpt__dy = 1 << bbc_vduvar(bbc_YEigFactor);
+#else /* SKS_ACW - cache EIG and size too */
+  wimptx__YEigFactor  = bbc_vduvar(bbc_YEigFactor);
+  wimpt__dy = 1 << wimptx__YEigFactor;
+  wimptx__display_size_cy = (bbc_vduvar(bbc_YWindLimit) + 1) << wimptx__YEigFactor;
+#endif /* SKS_ACW */
   if (old != wimpt__dy) changed = TRUE;
 
   old = wimpt__bpp;
   wimpt__bpp = 1 << bbc_vduvar(bbc_Log2BPP);
   if (old != wimpt__bpp) changed = TRUE;
 
+#ifdef SKS_ACW
+  wimptx__title_height   = 40;
+  wimptx__hscroll_height = 40;
+  wimptx__vscroll_width  = 40;
+  if(0) /* <<< test needed here */
+  {
+    wimptx__title_height   += wimpt__dy;
+    wimptx__hscroll_height += wimpt__dy;
+    wimptx__vscroll_width  += wimpt__dx;
+  }
+#endif /* SKS_ACW */
+
   return changed;
   /* This will now return TRUE if any interesting aspects of the
   window appearance have changed. */
 }
 
 #ifndef UROM
-void wimpt_forceredraw()
+void wimpt_forceredraw(void)
 {
   wimp_redrawstr r;
   r.w = (wimp_w) -1;
@@ -170,10 +239,21 @@
              (1 << bbc_vduvar(bbc_XEigFactor));
   r.box.y1 = (1 + bbc_vduvar(bbc_YWindLimit)) *
              (1 << bbc_vduvar(bbc_YEigFactor));
+#ifdef SKS_ACW
+  (void) tbl_wimp_force_redraw(r.w,
+                               r.box.x0, r.box.y0,
+                               r.box.x1, r.box.y1);
+#else
   (void) wimp_force_redraw(&r);
+#endif /* SKS_ACW */
 }
 #endif
 
+#undef wimpt_mode
+#undef wimpt_dx
+#undef wimpt_dy
+#undef wimpt_bpp
+
 int wimpt_mode(void)
 {
   return(wimpt__mode);
@@ -208,6 +288,8 @@
   return programname;
 }
 
+#ifndef SKS_ACW
+
 void wimpt_reporterror(os_error *e, wimp_errflags f)
 {
   if (!programname)
@@ -221,9 +303,13 @@
   return e;
 }
 
+#endif /* SKS_ACW */
+
 typedef void SignalHandler(int);
 
+#ifndef SKS_ACW
 static SignalHandler *oldhandler;
+#endif /* SKS_ACW */
 
 static void escape_handler(int sig)
 {
@@ -231,6 +317,7 @@
   (void) signal(SIGINT, &escape_handler);
 }
 
+#ifndef SKS_ACW
 
 static void handler(int signal)
 {
@@ -251,6 +338,8 @@
     exit(1);
 }
 
+#endif /* SKS_ACW */
+
 static int wimpversion = 0;
 
 
@@ -278,6 +367,11 @@
   signal(SIGINT, &escape_handler);
   signal(SIGSEGV, &handler);
   signal(SIGTERM, &handler);
+#ifdef SKS_ACW
+  signal(SIGSTAK, &wimptx_internal_stack_overflow_handler);
+  signal(SIGUSR1, &handler);
+  signal(SIGUSR2, &handler);
+#endif /* SKS_ACW */
   signal(SIGOSERROR, &errhandler);
   signal_handlers_installed = 1;
 }
@@ -312,7 +406,9 @@
 
   wimpt_checkmode();
   atexit(wimpt__exit);
+#ifndef SKS_ACW
   if (!win_init()) werr(TRUE, msgs_lookup(MSGS_wimpt3));
+#endif /* SKS_ACW */
   return wimpversion;
 }
 
