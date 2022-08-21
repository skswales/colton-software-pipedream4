--- _src	2021-12-31 14:59:30.150000000 +0000
+++ _dst	2022-08-18 16:55:46.800000000 +0000
@@ -33,6 +33,8 @@
  */
 
 
+#include "include.h" /* for SKS_ACW */
+
 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
@@ -61,7 +63,11 @@
 
 static int sprite_load(char *name)
 {
+#ifndef SKS_ACW
     char file_name[40]; /* long enough for <ProgramName$Dir>.SpritesXX */
+#else /* SKS_ACW */
+    char file_name[256]; /* long enough for <ProgramName$Dir>.SpritesXX */
+#endif /* SKS_ACW */
     res_findname(name, file_name);
 
 #ifdef SPRITES11
