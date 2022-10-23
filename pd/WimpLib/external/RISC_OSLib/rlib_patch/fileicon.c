--- _src	2021-12-31 14:59:30.010000000 +0000
+++ _dst	2022-08-18 17:22:35.940000000 +0000
@@ -42,7 +42,7 @@
 #include "wimpt.h"
 #include "sprite.h"
 #include "trace.h"
-#include "fileicon.h"
+#include "cs-fileicon.h"
 #include "werr.h"
 
 void fileicon(wimp_w w, wimp_i ii, int filetype)
@@ -58,27 +58,7 @@
   /* the icon must initially be an indirect text icon. */
   i.i.flags &= ~wimp_ITEXT;        /* set not text */
   i.i.flags |= wimp_ISPRITE + wimp_INDIRECT;       /* set sprite */
-
-  if (filetype == 0x1000)
-     sprintf(i.i.data.indirectsprite.name, "Directory");
-  else if (filetype == 0x2000)
-     sprintf(i.i.data.indirectsprite.name, "Application");
-  else
-     sprintf(i.i.data.indirectsprite.name, "file_%03x", filetype);
-
-  /* now to check if the sprite exists. */
-  /* do a sprite_select on the Wimp sprite pool */
-
-  if (wimp_spriteop(24,i.i.data.indirectsprite.name) == 0)
-  {
-     /* the sprite exists: all is well */
-  }
-  else
-  {
-     /* the sprite does not exist: print general don't-know icon. */
-     sprintf(i.i.data.indirectsprite.name, "file_xxx");
-  }
-
+  fileicon_name(i.i.data.indirectsprite.name, filetype);
   i.i.data.indirectsprite.spritearea = (void*) 1;
   tracef1("sprite name is %s.\n", (int) i.i.data.indirectsprite.name);
   wimp_create_icon(&i, &iii); /* will recreate with same number. */
