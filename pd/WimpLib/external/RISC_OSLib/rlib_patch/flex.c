--- _src	2019-07-29 20:14:14.000000000 +0100
+++ _dst	2019-07-30 11:45:38.670000000 +0100
@@ -45,6 +45,7 @@
  *   17-Sep-97: Defered compaction support added.
  */
 
+#ifndef SKS_ACW
 #define BOOL int
 #define TRUE 1
 #define FALSE 0
@@ -55,8 +56,11 @@
 #include <stdio.h>
 #include "kernel.h"
 #include "swis.h"
+#endif /* SKS_ACW */
 
+#ifndef SKS_ACW
 #include "opts.h"
+#endif /* SKS_ACW */
 #include "flex.h"
 #include "swiextra.h"
 
@@ -73,6 +77,14 @@
 static char *program_name = 0;
 static int  *error_fd = 0;
 
+#ifdef SKS_ACW
+
+#define MSGS_flex1 "flex1:" "Flex memory error"
+#define MSGS_flex2 "flex2:" "Not enough memory, or not within *desktop world"
+#define MSGS_flex3 "flex3:" "Flex not initialised"
+
+#else /* SKS_ACW */
+
 #define MSGS_flex1 "flex1", "Flex memory error"
 #define MSGS_flex2 "flex2", "Not enough memory, or not within *desktop world"
 #define MSGS_flex3 "flex3", "Flex not initialised"
@@ -117,6 +129,7 @@
         return (char *)r.r[2];
 }
 
+#endif /* SKS_ACW */
 
 /* This implementation goes above the original value of GetEnv,
 to memory specifically requested from the Wimp (about which the
@@ -130,6 +143,8 @@
                         /* then the actual store follows. */
 } flex__rec;
 
+#ifndef SKS_ACW
+
 static void flex__fail(int i)
 {
   werr(TRUE, msgs_lookup(MSGS_flex1));
@@ -144,16 +159,21 @@
     werr(TRUE, msgs_lookup(MSGS_flex3));
 }
 
+#endif /* SKS_ACW */
 
 /* IDJ macro to avoid stack usage */
 #define roundup(n)  (0xfffffffc & (n + 3))
 
+#ifndef SKS_ACW
+
 static char       * flex__base;           /* lowest flex__rec - only for budging, marking of freed     */
                                           /* blocks and debug output.                                  */
 static int          flex__area_num;       /* number of allocated dynamic area (0 => using appspace).   */
 static char       * flex__freep;          /* free flex memory.                                         */
 static char       * flex__lim;            /* limit of flex memory.                                     */
 
+#endif /* SKS_ACW */
+
 static unsigned int flex_to_compact;      /* offset from base of flex block to first free block.       */
                                           /* or NO_COMPACTION_NEEDED if there are no free blocks.      */
 static char         flex_needscompaction; /* = 1 if compaction is deferred until allocations or        */
@@ -204,6 +224,16 @@
   char *prev = flex__lim;
   int got = 0;
 
+#ifdef SKS_ACW
+  /* Always grow slot/area by a multiple of this granularity */
+  /* NB This granularity need not be the system page size */
+  n = flex_granularity_ceil(n);
+#endif /* SKS_ACW */
+
+#if defined(REPORT_FLEX)
+  reportf("flex__more(): flex__lim == %p, grow by %d", report_ptr_cast(flex__lim), n);
+#endif
+
   if (flex__area_num)
   {
      if (_swix(OS_ChangeDynamicArea, _INR(0,1)|_OUT(1), flex__area_num, n, &got))
@@ -217,6 +247,10 @@
      flex__wimpslot(&flex__lim);
   }
 
+#if defined(REPORT_FLEX)
+  reportf("flex__more(%d): flex__lim := %p", n, report_ptr_cast(flex__lim));
+#endif
+
   if (flex__lim < prev + n)
   {
    flex__lim = prev;             /* restore starting state:
@@ -236,17 +270,50 @@
 {
   /* Gives away memory, lowering flex__lim, if possible. */
 
+#ifdef SKS_ACW
+  int n = flex__lim - flex__freep;
+
+  /* Always shrink slot/area by a multiple of this granularity */
+  n = flex_granularity_floor(n);
+
+#if defined(REPORT_FLEX)
+  reportf("flex_give(): flex__lim = %p, flex__freep = %p, shrink by %d, granularity = %d",
+          report_ptr_cast(flex__lim), report_ptr_cast(flex__freep), n, flex_granularity);
+#endif
+
+  if(n < flex_granularity)
+    return;
+
+  if(flex_.shrink_forbidden)
+  {
+    reportf("flex__give(): shrink_forbidden (leaving %d free)", flex_storefree());
+    return;
+  }
+#endif /* SKS_ACW */
+
   if (flex__area_num)
   {
+#ifdef SKS_ACW
+    flex__lim -= n;
+#else
     int n = flex__lim - flex__freep;
+#endif /* SKS_ACW */
     _swix(OS_ChangeDynamicArea, _INR(0,1)|_OUT(1), flex__area_num, -n, &n);
     flex__lim -= n;
   }
   else
   {
+#ifdef SKS_ACW
+    flex__lim -= n;
+#else
     flex__lim = flex__freep;
+#endif /* SKS_ACW */
     flex__wimpslot(&flex__lim);
   }
+
+#if defined(REPORT_FLEX)
+  reportf("flex_give(): flex__lim := %p", (void *) flex__lim);
+#endif
 }
 
 static BOOL flex__ensure(int n)
@@ -325,6 +392,7 @@
           if (debugfileptr) fprintf(debugfileptr, "Failed in flex_reanchor\n");
         #endif
         flex__fail(6);
+        return; /* SKS_ACW */
       }
       *(p->anchor) = ((char*) (p + 1)) + by;
     }
@@ -362,6 +430,8 @@
       if (debugfileptr) fprintf(debugfileptr, "Failed in flex_free\n");
     #endif
     flex__fail(0);
+    *anchor = NULL; /* set anchor to null */ /* SKS_ACW */
+    return; /* SKS_ACW */
   }
 
   if (flex_needscompaction)
@@ -507,6 +577,7 @@
       if (debugfileptr) fprintf(debugfileptr, "Failed in flex_size\n");
     #endif
     flex__fail(4);
+    return(0); /* SKS_ACW */
   }
 
   return(p->size);
@@ -566,6 +637,7 @@
       if (debugfileptr) fprintf(debugfileptr, "Failed in flex_midextend (1)\n");
     #endif
     flex__fail(1);
+    return FALSE; /* SKS_ACW */
   }
 
   if (at > p->size)
@@ -574,6 +646,7 @@
       if (debugfileptr) fprintf(debugfileptr, "Failed in flex_midextend (2)\n");
     #endif
     flex__fail(2);
+    return FALSE; /* SKS_ACW */
   }
 
   if (by < 0 && (-by) > at)
@@ -582,6 +655,7 @@
       if (debugfileptr) fprintf(debugfileptr, "Failed in flex_midextend (3)\n");
     #endif
     flex__fail(3);
+    return FALSE; /* SKS_ACW */
   }
 
   if (by == 0)
@@ -670,10 +744,13 @@
 
   if (p->anchor != from_anchor)
   {
-    flex__fail(1);
     #ifdef TRACE
       if (debugfileptr) fprintf(debugfileptr, "Failed in flex_reanchor\n");
     #endif
+    flex__fail(1);
+    *to_anchor = NULL; /* SKS_ACW */
+    *from_anchor = NULL; /* SKS_ACW */
+    return FALSE; /* SKS_ACW */
   }
 
   p->anchor = to_anchor;
@@ -930,8 +1007,15 @@
 
   flex__lim = (char*) -1;
 
+#if defined(REPORT_FLEX)
+  reportf("flex_init(dynamic_size:%d)", dynamic_size);
+#endif
+
   if (dynamic_size)
   {
+#ifdef SKS_ACW
+  dynamic_size = flex_granularity_ceil(dynamic_size);
+#endif
   err = _swix(OS_DynamicArea,
               _INR(0,8)|_OUT(1)|_OUT(3),
               OS_DynamicArea_Create,
@@ -979,6 +1063,8 @@
   return flex__area_num;
 }
 
+#ifndef SKS_ACW
+
 /*************************************************/
 /* flex_save_heap_info()                         */
 /*                                               */
@@ -1024,6 +1110,8 @@
   }
 }
 
+#endif /* SKS_ACW */
+
 /*************************************************/
 /* flex_set_deferred_compaction()                */
 /*                                               */
