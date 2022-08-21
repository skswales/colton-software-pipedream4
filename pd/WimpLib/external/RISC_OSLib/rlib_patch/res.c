--- _src	2019-07-29 19:20:28.000000000 +0100
+++ _dst	2019-07-30 11:45:39.030000000 +0100
@@ -38,6 +38,8 @@
 #define TRUE 1
 #define FALSE 0
 
+#include "include.h" /* for SKS_ACW */
+
 #include <stdio.h>
 #include <string.h>
 
@@ -56,6 +58,11 @@
   programname = a;
 }
 
+#ifdef SKS_ACW /* SKS - PipeDream's RISC_OSLib-type resources now live one level down */
+/*#define RISC_OS_RESOURCE_SUBDIR "RISC_OS"*/ /* Not needed now RISC_OSLib-type resources live in PipeDreamRes: */
+#endif
+
+#ifndef SKS_ACW
 
 BOOL res_findname(const char *leafname, char *buf /*out*/)
 {
@@ -88,6 +95,37 @@
   return TRUE;
 }
 
+#else /* SKS_ACW */
+
+/* simple replacement function */
+
+int res_findname(const char *leafname, char *buf /*out*/)
+{
+  char var_name[64];
+  _kernel_osfile_block fileblk;
+  int res;
+
+  (void) snprintf(var_name, elemof32(var_name),
+           "%s$Path",
+           programname);
+
+  if(NULL == _kernel_getenv(var_name, buf, 256 /*assumed*/))
+      res_prefixnamewithpath(leafname, buf);
+  else
+      res_prefixnamewithdir(leafname, buf);
+
+  if(_kernel_osfile(5, buf, &fileblk) != 1)
+      res = -1; /* without error report */
+  else
+      res = fileblk.start; /* file length */
+
+  reportf("res_findname(%u:%s): length=%u\n", strlen(buf), buf, (size_t) res);
+
+  return(res);
+}
+
+#endif /* SKS_ACW */
+
 BOOL res_prefixnamewithpath(const char *leafname, char *buf /*out*/)
 {
 #ifndef UROM
@@ -100,6 +138,9 @@
   {
     strcpy(buf, programname);
     strcat(buf, ":");
+#ifdef RISC_OS_RESOURCE_SUBDIR
+    strcat(buf, RISC_OS_RESOURCE_SUBDIR ".");
+#endif
     strcat(buf, leafname);
   }
   return TRUE;
@@ -115,11 +156,17 @@
   else
 #endif
   {
+#ifdef RISC_OS_RESOURCE_SUBDIR
+    sprintf(buf, "<%s$Dir>." RISC_OS_RESOURCE_SUBDIR ".%s", programname, leafname);
+#else
     sprintf(buf, "<%s$Dir>.%s", programname, leafname);
+#endif
   }
   return TRUE;
 }
 
+#ifndef SKS_ACW
+
 #ifndef UROM
 FILE *res_openfile(const char *leafname, const char *mode)
 
@@ -142,5 +189,6 @@
 }
 #endif
 
+#endif /* SKS_ACW */
 
 /* end of c.res */
