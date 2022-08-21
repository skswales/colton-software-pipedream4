--- _src	2020-06-16 16:13:32.420000000 +0100
+++ _dst	2020-06-17 23:34:06.830000000 +0100
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
+#define RISC_OS_RESOURCE_SUBDIR "RISC_OS"
+#endif
+
+#ifndef SKS_ACW
 
 BOOL res_findname(const char *leafname, char *buf /*out*/)
 {
@@ -88,6 +95,29 @@
   return TRUE;
 }
 
+#else /* SKS_ACW */
+
+/* simple replacement function which only looks on the path, which must therefore be set */
+
+int res_findname(const char *leafname, char *buf /*out*/)
+{
+  _kernel_osfile_block fileblk;
+  int res;
+
+  res_prefixnamewithpath(leafname, buf);
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
@@ -100,6 +130,9 @@
   {
     strcpy(buf, programname);
     strcat(buf, ":");
+#ifdef RISC_OS_RESOURCE_SUBDIR
+    strcat(buf, RISC_OS_RESOURCE_SUBDIR ".");
+#endif
     strcat(buf, leafname);
   }
   return TRUE;
@@ -115,11 +148,17 @@
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
 
@@ -142,5 +181,6 @@
 }
 #endif
 
+#endif /* SKS_ACW */
 
 /* end of c.res */
