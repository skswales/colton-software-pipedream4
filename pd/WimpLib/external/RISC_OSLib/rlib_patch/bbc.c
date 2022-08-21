--- _src	2019-07-29 19:42:18.000000000 +0100
+++ _dst	2019-07-30 11:45:38.690000000 +0100
@@ -32,6 +32,8 @@
  * History: IDJ: 05-Feb-92: prepared for source release
  */
 
+#include "include.h" /* for SKS_ACW */
+
 #define BOOL int
 #define TRUE 1
 #define FALSE 0
@@ -78,6 +80,12 @@
 
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
@@ -130,6 +138,8 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 int bbc_modevar (int mode, int varno)
 
 { int flags, result;
@@ -220,23 +230,45 @@
 
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
@@ -248,15 +280,32 @@
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
@@ -549,6 +598,8 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 /* Return a character code from an input stream or the keyboard. */
 int bbc_inkey(int n)
 {
