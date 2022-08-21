--- _src	2009-05-31 18:58:59 +0100
+++ _dst	2013-09-04 17:05:03 +0100
@@ -59,7 +59,9 @@
 
 
 
+#ifndef SKS_ACW
 static int flex__initialised = 0;
+#endif /* SKS_ACW */
 
 
 /* This implementation goes above the original value of GetEnv,
@@ -74,6 +76,8 @@
 } flex__rec;
 
 
+#ifndef SKS_ACW
+
 static void flex__fail(int i)
 {
   werr(TRUE, msgs_lookup(MSGS_flex1));
@@ -91,12 +95,19 @@
     werr(TRUE, msgs_lookup(MSGS_flex3));
 }
 
+#endif /* SKS_ACW */
+
 /* macro to avoid stack usage */
 #define roundup(n)  (0xfffffffc & (n + 3))
 
+#ifndef SKS_ACW
+
 static char *flex__base;        /* lowest flex__rec - only for budging. */
 static char *flex__freep;       /* free flex memory */
 static char *flex__lim;         /* limit of flex memory */
+
+#endif /* SKS_ACW */
+
 /* From base upwards, it's divided into store blocks of
   a flex__rec
   the space
@@ -104,6 +115,8 @@
 */
 
 
+#ifndef SKS_ACW
+
 static void flex__wimpslot(char **top)
 {
   /* read/write the top of available memory. *top == 0 -> just read. */
@@ -154,7 +167,9 @@
   tracef1("flex__wimpslot out: %i.\n", slot);
 }
 
+#endif /* SKS_ACW */
 
+#ifndef SKS_ACW
 
 static BOOL flex__more(int n)
 {
@@ -196,10 +211,62 @@
   return (n <= 0 || flex__more(n));
 }
 
+#else /* SKS_ACW */
+
+static BOOL flex__ensure(int required)
+{
+    int more = required - (flex_.limit - flex_.freep);
+
+    if(more <= 0)
+        return(TRUE);
+
+    /*nd 15jul96 try the allocation anyway and see if it fails - faster and works correctly with Virtualise */
+    {
+    int top = (int) flex_.limit;
+    top += more;
+    if(NULL == flex__area_change(&top))
+    {
+        flex_.limit = (char *) top;
+        tracef1("flex__ensure: flex_.limit out: &%p\n", flex_.limit);
+    }
+    } /*block*/
+
+    more = required - (flex_.limit - flex_.freep);
+
+    if(more <= 0)
+        return(TRUE);
+
+    return(FALSE);
+}
+
+static void flex__give(void)
+{
+    /* Gives away memory, lowering flex_.limit, if possible. */
+    int mask = (flex_pagesize - 1); /* pagesize is a power of 2 */
+    int lim = (mask + (int) flex_.limit) & ~mask;
+    int fre = (mask + (int) flex_.freep) & ~mask;
+
+    tracef2("flex__give() flex_.limit: &%p, flex_.freep: &%p\n", flex_.limit, flex_.freep);
+
+    if((lim <= (fre + flex_pagesize /*SKS 04oct95 add a little hysteresis*/)) || flex_.shrink_forbidden)
+        return;
+
+    {
+    int top = (int) flex_.freep;
+    (void) flex__area_change(&top);
+    flex_.limit = (char *) top;
+    tracef1("flex__give: flex_.limit out: &%p\n", flex_.limit);
+    } /* block */
+}
+
+#endif /* SKS_ACW */
+
 BOOL flex_alloc(flex_ptr anchor, int n)
 {
   flex__rec *p;
 
+#ifndef SKS_ACW
+
   tracef2("flex_alloc %x %i.\n", (int) anchor, n);
 
   flex__check();
@@ -216,72 +283,144 @@
   p->anchor = anchor;
   p->size = n;
   *anchor = p + 1; /* sizeof(flex__rec), that is */
+
+#else /* SKS_ACW */
+
+    int required = sizeof(flex__rec) + roundup(n);
+
+    tracef2("flex_alloc(&%p, %d)\n", anchor, n);
+
+    if((n < 0)  ||  !flex__ensure(required))
+    {
+        *anchor = NULL;
+        trace_1(TRACE_OUT, "[flex_alloc(%d) yields NULL\n", n);
+        return(FALSE);
+    }
+
+    /* allocate at end of memory */
+    p = (flex__rec *) flex_.freep;
+
+    flex_.freep = flex_.freep + required;
+
+    p->anchor = anchor;
+    p->size   = n;              /* store requested amount, not allocated amount */
+
+    *anchor   = flex_innards(p);     /* point to punter's part of allocated object */
+
+    tracef1("flex_alloc yields &%p\n", *anchor);
+
+#endif /* SKS_ACW */
+
   return TRUE;
 }
 
 #if TRACE
 
+#ifndef SKS_ACW
 static char *flex__start ;
+#endif /* SKS_ACW */
 
 /* show all flex pointers for debugging purposes */
 void flex_display(void)
 {
  flex__rec *p = (flex__rec *) flex__start ;
 
- tracef3("*****flex display: %x %x %x\n",
-          (int) flex__start, (int) flex__freep, (int) flex__lim) ;
+ tracef3("flex__display(): flex__start &%p flex__lim &%p flex__freep &%p\n",
+         flex__start, flex__lim, flex__freep);
  while (1)
  {
-  if ((int) p >= (int) flex__freep) break;
+  if (p >= (flex__rec*) flex__freep) break;
 
-  tracef("flex block @ %x->%x->%x",
-        (int)p, (int)(p->anchor), (int)(*(p->anchor))) ;
+  tracef(TRACE_RISCOS_HOST, "flex block &%p->&%p->&%p, size %d",
+         p, p->anchor, *(p->anchor), p->size);
 
-  if (*(p->anchor) != p + 1) tracef("<<< bad block!");
+  if (*(p->anchor) != flex_innards(p)) tracef0(" <<< bad block!");
 
-  tracef("\n") ;
-  p = (flex__rec*) (((char*) (p + 1)) + roundup(p->size));
+  tracef0("\n") ;
+  p = flex_next_block(p);
  }
 }
 
 #endif
 
-static void flex__reanchor(flex__rec *p, int by)
+static void flex__reanchor(flex__rec *startp, int by)
 {
+  flex__rec *p = startp;
+
+  tracef3("flex_reanchor(&%p, by = %d): flex__freep = &%p\n", p, by, flex__freep);
+
   /* Move all the anchors from p upwards. This is in anticipation
   of that block of the heap being shifted. */
 
   while (1) {
-    if ((int) p >= (int) flex__freep) break;
-   tracef1("flex__reanchor %x\n",(int) p) ;
-    if (*(p->anchor) != p + 1) flex__fail(6);
-    *(p->anchor) = ((char*) (p + 1)) + by;
-    p = (flex__rec*) (((char*) (p + 1)) + roundup(p->size));
+    if (p >= (flex__rec*) flex__freep) break;
+
+    if(TRACE  &&  by)
+        tracef3("reanchoring object &%p (&%p) to &%p  ", flex_innards(p), p->anchor, flex_innards(p) + by);
+
+    /* check current registration */
+    if(TRACE)
+      if(*(p->anchor) != flex_innards(p))
+        werr(TRUE, "flex__reanchor: p->anchor &%p *(p->anchor) &%p != object &%p (motion %d)",
+                    p->anchor, *(p->anchor), flex_innards(p), by);
+
+    /* point anchor to where block will be moved */
+    *(p->anchor) = flex_innards(p) + by;
+
+    /* SKS - does anchor needs moving (is it above the reanchor start and in the flex area?) */
+    if(((char *) p->anchor >= (char *) startp)  &&  ((char *) p->anchor < flex__freep))
+    {
+      tracef3("moving anchor for object &%p from &%p to &%p  ", flex_innards(p), p->anchor, (char *) p->anchor + by);
+      p->anchor = (flex_ptr) ((char *) p->anchor + by);
+    }
+
+    p = flex_next_block(p);
   }
+
+  tracef0("\n");
 }
 
 void flex_free(flex_ptr anchor)
 {
-  flex__rec *p = ((flex__rec*) *anchor) - 1;
-  int roundsize = roundup(p->size);
-  flex__rec *next = (flex__rec*) (((char*) (p + 1)) + roundsize);
+  flex__rec *p = (flex__rec *) *anchor;
+  flex__rec *next;
+  int nbytes_above;
+  int blksize;
 
-  tracef1("flex_free %i.\n", (int) anchor);
+  tracef3("flex_free(&%p -> &%p (size %d))\n", anchor, p, flex_size(anchor));
   flex__check();
 
+  if(!p--)
+    return;
+
+  if(((char *) p < flex_.start) || ((char *) (p + 1) > flex__freep))
+  {
+    werr(TRUE, "flex_free: block (%p->%p) is not in flex store (%p:%p)", anchor, *anchor, flex_.start, flex__freep);
+  }
   if (p->anchor != anchor)
   {
-    flex__fail(0);
+    werr(TRUE, "flex_free: anchor (%p->%p) does not match that stored in block (%p->%p)", anchor, *anchor, p, p->anchor);
   }
 
-  flex__reanchor(next, - (sizeof(flex__rec) + roundsize));
+  next = flex_next_block(p);
+  nbytes_above = flex__freep - (char *) next;
+  blksize = sizeof(flex__rec) + roundup(p->size);
+
+  if(nbytes_above)
+  {
+    flex__reanchor(next, -blksize);
+
+    memmove(
+      p,
+      next,
+      nbytes_above);
+  }
 
-  memmove(
-     p,
-     next,
-     flex__freep - (char*) next);
+  flex__freep -= blksize;
 
-  flex__freep -= sizeof(flex__rec) + roundsize;
+  if(TRACE  &&  nbytes_above)
+    /* a quick check after all that */
+    flex__reanchor(p, 0);
 
   flex__give();
 
@@ -290,16 +429,24 @@
 
 int flex_size(flex_ptr anchor)
 {
-  flex__rec *p = ((flex__rec*) *anchor) - 1;
+  flex__rec *p = (flex__rec*) *anchor;
+
+  if(!p--)
+      return(0);
+
   flex__check();
+  if(((char *) p < flex_.start) || ((char *) (p + 1) > flex__freep))
+  {
+    werr(TRUE, "flex_size: block (%p->%p) is not in flex store (%p:%p)", anchor, *anchor, flex_.start, flex__freep);
+  }
   if (p->anchor != anchor)
   {
-    flex__fail(4);
+    werr(TRUE, "flex_size: anchor (%p->%p) does not match that stored in block (%p->%p)", anchor, *anchor, p, p->anchor);
   }
   return(p->size);
 }
 
-int flex_extend(flex_ptr anchor, int newsize)
+BOOL flex_extend(flex_ptr anchor, int newsize)
 {
   flex__rec *p = ((flex__rec*) *anchor) - 1;
   flex__check();
@@ -311,21 +458,21 @@
   flex__rec *p;
   flex__rec *next;
 
-  tracef3("flex_midextend %i at=%i by=%i.\n", (int) anchor, at, by);
+  tracef4("flex_midextend(&%p -> &%p, at = %d, by = %d)\n", anchor, ((flex__rec*) *anchor) - 1, at, by);
   flex__check();
 
   p = ((flex__rec*) *anchor) - 1;
-  if (p->anchor != anchor)
+  if(((char *) p < flex_.start) || ((char *) (p + 1) > flex__freep))
   {
-    flex__fail(1);
+    werr(TRUE, "flex_midextend: block (%p->%p) is not in flex store (%p:%p)", anchor, *anchor, flex_.start, flex__freep);
   }
-  if (at > p->size)
+  if (p->anchor != anchor)
   {
-    flex__fail(2);
+    werr(TRUE, "flex_midextend: anchor (%p->%p) does not match that stored in block (%p->%p)", anchor, *anchor, p, p->anchor);
   }
-  if (by < 0 && (-by) > at)
+  if((U32) at > (U32) p->size)
   {
-    flex__fail(3);
+    werr(TRUE, "flex_midextend: at %d > size stored in block (%p:%d)", at, p, p->size);
   }
   if (by == 0)
   {
@@ -337,28 +484,32 @@
     int growth = roundup(p->size + by) - roundup(p->size);
     /* Amount by which the block will actually grow. */
 
-    if (! flex__ensure(growth))
+    if(growth) /* subsequent block motion might not be needed for small extensions */
     {
-      return FALSE;
-    }
+      if (! flex__ensure(growth))
+      {
+        return FALSE;
+      }
 
-    next = (flex__rec*) (((char*) (p + 1)) + roundup(p->size));
-    /* The move has to happen in two parts because the moving
-    of objects above is word-aligned, while the extension within
-    the object may not be. */
+      next = flex_next_block(p);
 
-    flex__reanchor(next, growth);
+      /* The move has to happen in two parts because the moving
+      of objects above is word-aligned, while the extension within
+      the object may not be. */
 
-    memmove(
-      ((char*) next) + roundup(growth),
-      next,
-      flex__freep - (char*) next);
+      flex__reanchor(next, growth);
+
+      memmove(
+        ((char*) next) + roundup(growth),
+        next,
+        flex__freep - (char*) next);
 
-    flex__freep += growth;
+      flex__freep += growth;
+    }
 
     memmove(
-      ((char*) (p + 1)) + at + by,
-      ((char*) (p + 1)) + at,
+      flex_innards(p) + at + by,
+      flex_innards(p) + at,
       p->size - at);
     p->size += by;
 
@@ -368,18 +519,24 @@
     /* The block shrinks. */
     int shrinkage;
 
-    next = (flex__rec*) (((char*) (p + 1)) + roundup(p->size));
+    next = flex_next_block(p);
 
     by = -by; /* a positive value now */
     shrinkage = roundup(p->size) - roundup(p->size - by);
       /* a positive value */
 
+    if((U32) by > (U32) at)
+      werr(TRUE, "flex_midextend: by %d > at %d", by, at);
+
     memmove(
-      ((char*) (p + 1)) + at - by,
-      ((char*) (p + 1)) + at,
+      flex_innards(p) + at - by,
+      flex_innards(p) + at,
       p->size - at);
     p->size -= by;
 
+    if(!shrinkage) /* subsequent block motion might not be needed for small shrinkages */
+      return TRUE;
+
     flex__reanchor(next, - shrinkage);
 
     memmove(
@@ -392,10 +549,17 @@
     flex__give();
 
   }
+
+#if TRACE
+  /* a quick check after all that */
+  flex__reanchor(flex_next_block(p), 0);
+#endif
   return TRUE;
 }
 
 
+#ifndef SKS_ACW
+
 /* stack checking off */
 #pragma -s1
 
@@ -561,5 +725,7 @@
 
 }
 
+#endif /* SKS_ACW */
+
 /* end */
 
