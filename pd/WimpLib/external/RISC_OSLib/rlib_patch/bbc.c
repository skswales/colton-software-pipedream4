--- _src	2020-07-01 11:43:05.680000000 +0100
+++ _dst	2020-07-01 11:46:45.360000000 +0100
@@ -32,6 +32,8 @@
  * History: IDJ: 05-Feb-92: prepared for source release
  */
 
+#include "include.h" /* for SKS_ACW */
+
 #define BOOL int
 #define TRUE 1
 #define FALSE 0
@@ -75,6 +77,12 @@
 
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
@@ -127,6 +135,8 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 int bbc_modevar (int mode, int varno)
 
 { int flags, result;
@@ -217,6 +227,8 @@
 
 /* ---------- Graphics ----------- */
 
+#ifndef SKS_ACW
+
 /* Clear graphics window. */
 os_error *bbc_clg(void)
 {
@@ -245,15 +257,32 @@
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
@@ -546,6 +575,8 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 /* Return a character code from an input stream or the keyboard. */
 int bbc_inkey(int n)
 {
