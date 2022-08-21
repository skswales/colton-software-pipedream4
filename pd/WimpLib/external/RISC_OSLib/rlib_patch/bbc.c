--- _src	2010-11-30 10:29:08 +0100
+++ _dst	2013-09-02 16:10:43 +0100
@@ -35,6 +35,8 @@
  * History: IDJ: 05-Feb-92: prepared for source release
  */
 
+#include "include.h" /* for SKS_ACW */
+
 #define BOOL int
 #define TRUE 1
 #define FALSE 0
@@ -81,6 +83,12 @@
 
 #pragma -s1
 
+#ifdef SKS_ACW
+
+/* Many of these functions are not used by PipeDream */
+
+#else /* NOT SKS_ACW */
+
 #ifndef UROM
 /* Set screen mode. */
 os_error *bbc_mode(int n)
@@ -133,6 +141,8 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 int bbc_modevar (int mode, int varno)
 
 { int flags, result;
@@ -223,23 +233,45 @@
 
 /* ---------- Graphics ----------- */
 
+#ifndef SKS_ACW
+
 /* Clear graphics window. */
 os_error *bbc_clg(void)
 {
    return (bbc_vdu(bbc_ClearGraph));
 }
 
+#endif /* SKS_ACW */
+
 /* Set up graphics window. */
 os_error *bbc_gwindow(int a, int b, int c, int d)
 {
+#ifdef SKS_ACW
+    /* SKS get round VDU funnel/multiple SWI overhead */
+    char buffer[9 /*length of VDU 24 sequence*/];
+    char * p_u8 = buffer;
+    *p_u8++ = 24;
+    *p_u8++ = (a);
+    *p_u8++ = (a >> 8);
+    *p_u8++ = (b);
+    *p_u8++ = (b >> 8);
+    *p_u8++ = (c);
+    *p_u8++ = (c >> 8);
+    *p_u8++ = (d);
+    *p_u8++ = (d >> 8);
+    return(os_writeN(buffer, sizeof(buffer)));
+#else /* NOT SKS_ACW */
    os_error *e = bbc_vdu(bbc_DefGraphWindow);
    if (!e) e = bbc_vduw(a);
    if (!e) e = bbc_vduw(b);
    if (!e) e = bbc_vduw(c);
    if (!e) e = bbc_vduw(d);
    return(e);
+#endif /* SKS_ACW */
 }
 
+#ifndef SKS_ACW
+
 #ifndef UROM
 /* Move the graphics origin to the given absolute coordinates. */
 os_error *bbc_origin(int x, int y)
@@ -251,15 +283,32 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 /* Set graphics foreground/background colour and action. */
 os_error *bbc_gcol(int a, int b)
 {
+#ifdef SKS_ACW
+    /* SKS get round VDU funnel/multiple SWI overhead */
+    char buffer[3 /*length of VDU 18 sequence*/];
+    char * p_u8 = buffer;
+    *p_u8++ = 18;
+    *p_u8++ = (a);
+    *p_u8++ = (b);
+    return(os_writeN(buffer, sizeof(buffer)));
+#else /* NOT SKS_ACW */
    os_error *e = bbc_vdu(bbc_DefGraphColour);
    if (!e) e = bbc_vdu(a);
    if (!e) e = bbc_vdu(b);
    return(e);
+#endif /* SKS_ACW */
 }
 
+#ifdef SKS_ACW
+
+/* see cs-bbcx.h for bbc_move() / bbc_draw() / bbc_drawby() macros that use os_plot() */
+
+#else /* NOT SKS_ACW */
 
 /* Perform an operating system plot operation. Plot number, x, y. */
 os_error *bbc_plot(int n, int x, int y)
@@ -552,6 +601,8 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 /* Return a character code from an input stream or the keyboard. */
 int bbc_inkey(int n)
 {
