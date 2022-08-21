--- _src	2019-07-29 19:42:18.000000000 +0100
+++ _dst	2019-07-30 11:45:39.040000000 +0100
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
