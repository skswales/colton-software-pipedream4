--- _src	2022-10-26 16:47:18.220000000 +0000
+++ _dst	2022-10-27 18:43:18.810000000 +0000
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
@@ -144,10 +154,13 @@
    int result;
    vars[0] = varno;
    vars[1] = -1; /* terminator. */
-   return os_swi2 (os_X | OS_ReadVduVariables, (int) &vars[0], (int) &result) != NULL?
+   return os_swix2(OS_ReadVduVariables, (int) &vars[0], (int) &result) != NULL?
          -1: result; /*relies on -1 never being valid*/
+   /* SKS_ACW was os_swi2 (os_X | ) */
 }
 
+#ifndef SKS_ACW
+
 os_error *bbc_vduvars(int *vars, int *values)
 {
    return(os_swi2(os_X | OS_ReadVduVariables, (int) vars, (int) values));
@@ -245,15 +258,32 @@
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
+    /* char * p_u8 = buffer; */
+    buffer[0] = bbc_DefGraphColour /*18*/;
+    buffer[1] = a;
+    buffer[2] = b;
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
@@ -556,6 +586,8 @@
   return x;
 }
 
+#endif /* SKS_ACW */
+
 #pragma -s0
 
 /* end of c.bbc */
