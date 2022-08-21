--- _src	2009-05-31 18:58:59.000000000 +0100
+++ _dst	2014-10-28 16:35:10.000000000 +0100
@@ -72,13 +72,22 @@
 
 os_error * wimpt_poll(wimp_emask mask, wimp_eventstr * result)
 {
+#ifdef SKS_ACW
+  tracef3("\n[wimpt_poll(&%X, &%p): fake_waiting = %s]\n", mask, result, trace_boolstring(wimpt__fake_waiting));
+  wimpt__must_die = 0;
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
@@ -87,11 +96,36 @@
       r = wimp_pollidle(mask & ~wimp_EMNULL, result, next_alarm_time);
     }
     else
+#else /* SKS_ACW */
+    if(wimpt__atexitproc) /* SKS pragmatic fix - make general in due course */
+      wimpt__atexitproc(result);
+#endif /* SKS_ACW */
+
+    if(win_idle_event_claimer() != win_IDLE_OFF)
+    {
+      /* ensure we get null events */
+      mask = (wimp_emask) (mask & ~wimp_EMNULL);
+      tracef1("[wimpt_poll must get idle requests: mask := %X]\n", mask);
+    }
+
+    for(;;)
     {
       tracef0("Polling busy\n");
-      tracef2("Mask = %d   %d\n", mask, event_getmask());
-      r = wimp_poll(mask, result);
+      tracef2("[calling wimp_Poll_SFM(&%X, &%p)]\n", mask, result);
+      if(NULL == (r = wimp_Poll_SFM(mask, result)))
+          break;
+
+      tracef0("--- ERROR returned from wimp_poll()!!! (Ta Neil)\n");
+      wimpt_complain(r);
     }
+
+#ifdef SKS_ACW
+    if(wimpt__atentryproc) /* SKS pragmatic fix - make general in due course */
+        wimpt__atentryproc(result);
+
+    tracef1("[wimpt_poll returns real event %s]\n", report_wimp_event(result->e, &result->data));
+#endif /* SKS_ACW */
+
     wimpt__last_event = *result;
     return(r);
   }
@@ -119,11 +153,22 @@
 
 /* -------- Control of graphics environment -------- */
 
+#ifndef SKS_ACW
+
 static int wimpt__mode = 12;
 static int wimpt__dx;
 static int wimpt__dy;
 static int wimpt__bpp;
 
+#else /* SKS_ACW */
+
+int wimpt__mode  = 12;
+int wimpt__dx    = 2;
+int wimpt__dy    = 4;
+int wimpt__bpp   = 1;
+
+#endif /* SKS_ACW */
+
 static int wimpt__read_screen_mode(void)
 {
   int x, y;
@@ -146,17 +191,41 @@
   if (old != wimpt__mode) changed = TRUE;
 
   old = wimpt__dx;
+#ifndef SKS_ACW
   wimpt__dx = 1 << bbc_vduvar(bbc_XEigFactor);
+#else /* SKS_ACW - cache eig and size too */
+  wimpt__xeig  = bbc_vduvar(bbc_XEigFactor);
+  wimpt__dx    = 1 << wimpt__xeig;
+  wimpt__xsize = (bbc_vduvar(bbc_XWindLimit) + 1) << wimpt__xeig;
+#endif /* SKS_ACW */
   if (old != wimpt__dx) changed = TRUE;
 
   old = wimpt__dy;
+#ifndef SKS_ACW
   wimpt__dy = 1 << bbc_vduvar(bbc_YEigFactor);
+#else /* SKS_ACW - cache eig and size too */
+  wimpt__yeig  = bbc_vduvar(bbc_YEigFactor);
+  wimpt__dy    = 1 << wimpt__yeig;
+  wimpt__ysize = (bbc_vduvar(bbc_YWindLimit) + 1) << wimpt__yeig;
+#endif /* SKS_ACW */
   if (old != wimpt__dy) changed = TRUE;
 
   old = wimpt__bpp;
   wimpt__bpp = 1 << bbc_vduvar(bbc_Log2BPP);
   if (old != wimpt__bpp) changed = TRUE;
 
+#ifdef SKS_ACW
+  wimpt__title_height   = 40;
+  wimpt__hscroll_height = 40;
+  wimpt__vscroll_width  = 40;
+  if(0) /* <<< test needed here */
+  {
+    wimpt__title_height   += wimpt__dy;
+    wimpt__hscroll_height += wimpt__dy;
+    wimpt__vscroll_width  += wimpt__dx;
+  }
+#endif /* SKS_ACW */
+
   return changed;
   /* This will now return TRUE if any interesting aspects of the
   window appearance have changed. */
@@ -177,6 +246,11 @@
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
@@ -226,7 +300,9 @@
 
 typedef void SignalHandler(int);
 
+#ifndef SKS_ACW
 static SignalHandler *oldhandler;
+#endif /* SKS_ACW */
 
 static void escape_handler(int sig)
 {
@@ -234,6 +310,7 @@
   (void) signal(SIGINT, &escape_handler);
 }
 
+#ifndef SKS_ACW
 
 static void handler(int signal)
 {
@@ -254,6 +331,131 @@
     exit(1);
 }
 
+#else /* SKS_ACW */
+
+/* if message system ok then all well and good, but there is a chance of fulkup */
+
+static jmp_buf * program_safepoint = NULL;
+
+extern void
+wimpt_set_safepoint(jmp_buf * safe)
+{
+    program_safepoint = safe;
+}
+
+#pragma no_check_stack
+
+static void
+wimpt_jmp_safepoint(int signum)
+{
+    if(program_safepoint)
+        longjmp(*program_safepoint, signum);
+}
+
+static void stack_overflow_handler(int signum)
+{
+    static const _kernel_oserror er = { 1,
+    "Stack overflow. "
+    "Click Continue to exit, losing data, Cancel to attempt to resume execution."
+    };
+
+    wimp_errflags flags = (wimp_errflags) (wimp_EOK | wimp_ECANCEL);
+    wimp_errflags flags_out;
+
+    /*if(wimpt_os_version_query() >= RISC_OS_3_5)*/
+    {
+        flags = (wimp_errflags) (flags | wimp_EUSECATEGORY); /* new-style */
+        flags = (wimp_errflags) (flags | wimp_ECAT_STOP); /* error */
+    }
+
+    flags_out = wimp_ReportError_wrapped(&er, flags, programname, "error", (const char *) 1 /*Wimp Sprite Area*/, NULL);
+    if(0 != (wimp_ECANCEL & flags_out))
+    {
+       /* give it your best shot else we come back and die soon */
+        wimpt_jmp_safepoint(SIGSTAK);
+    }
+
+    exit(EXIT_FAILURE);
+}
+
+#pragma check_stack
+
+/* keep defaults for these in case of msgs death */
+
+static const char sudden_death_str[] =
+    "wimpt2:%s has gone wrong (%s). "
+    "Click Continue to exit, losing data";
+
+static const char pending_death_str[] =
+    "wimptT42:%s has gone wrong (%s). "
+    "Click Continue to exit, losing data, Cancel to attempt to resume execution.";
+
+static void handler(int signum)
+{
+  os_error er;
+  char causebuffer[64];
+  const char * cause;
+  int must_die, jump_back;
+  wimp_errflags flags, flags_out;
+
+  switch(signum)
+  {
+    case SIGOSERROR:
+      cause = _kernel_last_oserror()->errmess;
+      break;
+
+    default:
+      (void) snprintf(causebuffer, elemof32(causebuffer), "wimptT%d", signum);
+      if((cause = msgs_lookup(causebuffer)) == causebuffer)
+          {
+          (void) snprintf(causebuffer, elemof32(causebuffer), msgs_lookup("wimpt1:type=%d"), signum);
+          cause = causebuffer;
+          }
+      break;
+  }
+
+  if(!program_safepoint)
+    wimpt__must_die = 1;
+
+  must_die = wimpt__must_die;
+  wimpt__must_die = 1; /* trap errors in lookup/sprintf etc */
+
+  er.errnum = signum;
+
+  (void) snprintf(er.errmess, elemof32(er.errmess),
+          msgs_lookup(de_const_cast(char *, must_die ? sudden_death_str : pending_death_str)),
+          wimpt_programname(),
+          cause);
+
+  flags = (wimp_errflags) (must_die ? wimp_EOK : wimp_EOK | wimp_ECANCEL);
+
+  if(wimpt_os_version_query() >= RISC_OS_3_5)
+  {
+    flags = (wimp_errflags) (flags | wimp_EUSECATEGORY); /* new-style */
+    flags = (wimp_errflags) (flags | wimp_ECAT_STOP); /* error */
+  }
+
+  flags_out = wimp_ReportError_wrapped(&er, flags, wimpt_programname(), wimpt_get_spritename(), (const char *) 1 /*Wimp Sprite Area*/, NULL);
+  jump_back = (0 != (wimp_ECANCEL & flags_out));
+
+  if(jump_back)
+    {
+    /* reinstall ourselves, as SIG_DFL may have been restored by the system
+     * as defined by the ANSI spec!
+     */
+    (void) signal(signum, &handler);
+
+    /* give it your best shot else we come back and die soon */
+    wimpt_jmp_safepoint(signum);
+    }
+
+  exit(EXIT_FAILURE);
+}
+
+#define errhandler handler
+
+#endif /* SKS_ACW */
+
 static int wimpversion = 0;
 
 
@@ -281,6 +483,11 @@
   signal(SIGINT, &escape_handler);
   signal(SIGSEGV, &handler);
   signal(SIGTERM, &handler);
+#ifdef SKS_ACW
+  signal(SIGSTAK, &stack_overflow_handler);
+  signal(SIGUSR1, &handler);
+  signal(SIGUSR2, &handler);
+#endif /* SKS_ACW */
   signal(SIGOSERROR, &errhandler);
   signal_handlers_installed = 1;
 }
@@ -315,7 +522,9 @@
 
   wimpt_checkmode();
   atexit(wimpt__exit);
+#ifndef SKS_ACW
   if (!win_init()) werr(TRUE, msgs_lookup(MSGS_wimpt3));
+#endif /* SKS_ACW */
   return wimpversion;
 }
 
