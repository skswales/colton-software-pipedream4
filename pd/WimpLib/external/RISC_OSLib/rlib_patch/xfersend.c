--- _src	2009-05-31 18:58:59 +0100
+++ _dst	2013-08-31 18:30:02 +0100
@@ -35,6 +35,8 @@
  *
  */
 
+#include "include.h" /* for SKS_ACW */
+
 #define BOOL int
 #define TRUE 1
 #define FALSE 0
@@ -62,6 +64,22 @@
 #include "dragasprit.h"
 #include "VerIntern/messages.h"
 
+#ifdef SKS_ACW
+static struct _xfersend_statics
+{
+    BOOL                     ukproc;
+    dbox_raw_handler_proc    oldraw_handler_proc;
+    void *                   oldhandle;
+
+    dbox                     d;
+    xfersend_clickproc       clickproc;
+    void *                   clickhandle;
+
+    dbox_field               cancel_button;
+}
+xfersend_;
+#endif /* SKS_ACW */
+
 static int rcvbufsize;
 static int xfersend__msgid = 0;           /* my_ref of last DataSave message */
 static xfersend_saveproc xfersend__saveproc;
@@ -72,7 +90,9 @@
 static int xfersend__estsize = 0;
 static wimp_t xfersend__receiver;
 static BOOL xfersend__fileissafe;
+#ifndef SKS_ACW
 static char *xfersend__filename;  /*[256]*/
+#endif
 
 static int Unused; /*future expansion?*/
 
@@ -111,9 +131,43 @@
 }
 #endif
 
+/******************************************************************************
+*
+* determine whether a filename is 'rooted' sufficiently
+* to open not relative to cwd or path
+*
+******************************************************************************/
+
+static BOOL
+xfersend__file_is_rooted(
+    PC_U8 filename)
+{
+    int res;
+
+    res = (strpbrk(filename, ":$&%@\\") != NULL);
+
+    return(res);
+}
+
+/* nd 23-07-1996 added veneer for DragASprite and Wimp_StartDrag */
+static void nd_dragstop(void);
+
+static BOOL xfersend__wimp_EBUT(dbox d, wimp_eventstr *e);
+
+static BOOL xfersend__wimpevents(dbox d, void *ev, void *handle);
+
 static BOOL xfersend__unknowns (wimp_eventstr *e, void *handle)
 {
-  tracef1 ("xfersend raw event %i.\n", e->e);
+  tracef1 ("xfersend__unknowns got event %s\n", report_wimp_event(e->e, &e->data));
+  return(xfersend__wimpevents((dbox) handle, (void *) e, NULL));
+}
+
+static BOOL xfersend__wimpevents(dbox d, void *ev, void *handle)
+{
+  wimp_eventstr eventcopy = * (wimp_eventstr *) ev; /* copy event */
+  wimp_eventstr * e = &eventcopy;
+
+  tracef1 ("xfersend__wimpevents got event %s\n", report_wimp_event(e->e, &e->data));
   handle = handle;
 
   switch (e->e)
@@ -122,10 +176,19 @@
     { /* finish my drag */
       tracef0 ("drag event received.\n");
       #if USE_DRAGASPRITE
+#ifndef SKS_ACW
         if (xfersend__using_dragasprite)
           dragasprite_stop ();
+#else /* SKS_ACW */
+      nd_dragstop();
+#endif /* SKS_ACW */
       #endif
       wimp_get_point_info (&xfersend__mousestr);
+      if (xfersend__mousestr.w == dbox_syshandle(xfersend_.d))
+      {
+        tracef0 ("drag to same window: has no effect in PipeDream.\n");
+        return TRUE;
+      }
       if (xfersend__mousestr.w != -1)
       {
         wimp_msgstr msg;
@@ -146,8 +209,8 @@
           int i, tail, namelen;
           char name[256];
 
-          if (xfersend__filename == 0) xfersend__filename = malloc (256);
-          strncpy (name,xfersend__filename,  256);
+          dbox_getfield(d, (dbox_field) xfersend_FName, name, sizeof(name));
+
           tail = strlen (name); /* point at the zero */
           while (tail > 0 && name[tail-1] != '.' && name[tail-1] != ':')
                tail--;
@@ -175,8 +238,8 @@
 
     case wimp_ESEND:
     case wimp_ESENDWANTACK:
-      tracef3 ("xfersend msg %x received: %i %i.\n",
-          e->data.msg.hdr.action,e->data.msg.hdr.your_ref,xfersend__msgid);
+      tracef3 ("xfersend msg %s received: your_ref %i, msgid %i\n",
+          report_wimp_message(e, FALSE), e->data.msg.hdr.your_ref,xfersend__msgid);
 
       if (e->data.msg.hdr.your_ref == xfersend__msgid)
         switch (e->data.msg.hdr.action)
@@ -205,6 +268,9 @@
 
               if (xfersend__sendproc (xfersend__savehandle, &rcvbufsize))
               {
+                /* get dbox closed */
+                win_send_close(dbox_syshandle(d));
+
                 /* See sendbuf for all the real work for this case... */
 
                 tracef0 ("The send succeeded; send final RAMTRANSMIT.\n");
@@ -234,24 +300,30 @@
 
           case wimp_MPrintTypeOdd: /*was dropped on a printer application with
               queue empty - print immediately*/
-            if (xfersend__printproc != NULL)
+             if (xfersend__printproc != NULL)
             {
               wimp_t xfersend__printer;
-
-              win_remove_unknown_event_processor (&xfersend__unknowns, 0);
+              int res;
 
               tracef0 ("immediate print request acceptable\n");
               xfersend__fileissafe = FALSE;
 
-              /*Print the file now.*/
-              (void) (*xfersend__printproc) (e->data.msg.data.print.name,
+              xfersend__printer = e->data.msg.hdr.task;
+              xfersend__msg = e->data.msg;
+              xfersend__msg.hdr.your_ref = e->data.msg.hdr.my_ref;
+
+              /* get dbox closed */
+              win_send_close(dbox_syshandle(d));
+
+              res = (*xfersend__printproc) (e->data.msg.data.print.name,
                   xfersend__savehandle);
                   /*reports errors, if any*/
 
-              xfersend__printer = e->data.msg.hdr.task;
-              xfersend__msg = e->data.msg;
-              xfersend__msg.hdr.your_ref = xfersend__msg.hdr.my_ref;
-              xfersend__msg.hdr.action = wimp_MPrintTypeKnown;
+              xfersend__msg.hdr.action = (res >= 0) ? wimp_MDATALOAD : wimp_MWillPrint;
+
+              xfersend__msg.data.print.type = res;
+              /* in case it's been saved */
+
               wimpt_noerr (wimp_sendmessage (wimp_ESEND, &xfersend__msg,
                   xfersend__printer));
               if (xfersend__close) xfersend__winclose ();
@@ -264,6 +336,7 @@
             return TRUE;
           break;
 
+#ifdef UNUSED_AS_YET
           case wimp_MPrintError:
             if (e->data.msg.hdr.size > 24)
               werr(FALSE, &e->data.msg.data.chars[4]);
@@ -271,6 +344,7 @@
             if (xfersend__close) xfersend__winclose ();
             return TRUE;
           break;
+#endif
 
           case wimp_MDATASAVEOK:
             tracef4 ("datasaveok %i %i %i %i.\n",
@@ -286,25 +360,31 @@
             tracef1 ("it's the datasaveok, to file '%s'.\n",
                 (int) &e->data.msg.data.datasaveok.name[0]);
 
-            win_remove_unknown_event_processor (&xfersend__unknowns, 0);
+            win_remove_unknown_event_processor (xfersend__unknowns, d);
+            xfersend_.ukproc = FALSE;
 
             tracef1 ("save to filename '%s'.\n",
                 (int) &e->data.msg.data.datasaveok.name[0]);
 
-            xfersend__fileissafe = e->data.msg.data.datasaveok.estsize >= 0;
+            xfersend__fileissafe = e->data.msg.data.datasaveok.estsize > 0;
+
+            xfersend__msg = e->data.msg;
+                  /* sets hdr.size, data.w,i,x,y, size, name */
+            xfersend__receiver = e->data.msg.hdr.task;
+            xfersend__msg.hdr.your_ref = e->data.msg.hdr.my_ref;
+            xfersend__msg.hdr.action = wimp_MDATALOAD;
 
             if (xfersend__saveproc != NULL &&
                 xfersend__saveproc (&e->data.msg.data.datasaveok.name[0],
                 xfersend__savehandle))
             { tracef0 ("the save succeeded: send dataload\n");
 
-              xfersend__msg = e->data.msg;
-                  /* sets hdr.size, data.w,i,x,y, size, name */
-              xfersend__msg.hdr.your_ref = e->data.msg.hdr.my_ref;
-              xfersend__msg.hdr.action = wimp_MDATALOAD;
+              /* get dbox closed */
+              win_send_close(dbox_syshandle(d));
+
               xfersend__msg.data.dataload.type = xfersend__filetype;
               wimpt_noerr (wimp_sendmessage (wimp_ESENDWANTACK, &xfersend__msg,
-                  e->data.msg.hdr.task));
+                  xfersend__receiver));
             }
             else
               /* he has already reported the error: nothing more to do. */
@@ -324,6 +404,7 @@
       {
         tracef0 ("no printer manager - printing direct\n");
 
+#ifdef UNUSED_AS_YET
         if (xfersend__printproc != NULL)
           (void) (*xfersend__printproc) (e->data.msg.data.print.name,
               xfersend__savehandle);
@@ -332,9 +413,31 @@
           tracef0 ("no printing function supplied\n");
 
         return TRUE;
+#endif
       }
-    break;
+
+      if (e->data.msg.hdr.action == wimp_MDATALOAD &&
+          e->data.msg.hdr.my_ref == xfersend__msgid)
+      {
+        /* It looks as if he hasn't acknowledged my DATALOAD acknowledge:
+           thus it may be a loose scrap file, and must be deleted. */
+        _kernel_swi_regs rs;
+        tracef0 ("he hasn't ack'd our data load of temp file, so delete the file.\n");
+        rs.r[0] = OSFile_Delete; /* doesn't complain if not there */
+        rs.r[1] = (int) xfersend__msg.data.dataload.name;
+        (void) _kernel_swi(OS_File, &rs, &rs);
+        werr(FALSE, msgs_lookup("xfersend1" /*":Bad data transfer, receiver dead"*/));
+        return(TRUE);
+      }
+      break;
+
+    case wimp_EBUT:
+      return xfersend__wimp_EBUT(d, e);
   }
+
+  if(xfersend_.oldraw_handler_proc)
+    return((* xfersend_.oldraw_handler_proc) (d, ev, xfersend_.oldhandle));
+
   return FALSE;
 }
 
@@ -426,6 +529,105 @@
  return sendbuf__state != 2;  /* OK unless state = broken */
 }
 
+/* make and set a file icon of the appropriate type */
+
+static void xfersend__fileicon(dbox d)
+{
+  wimp_w w = dbox_syshandle(d);
+  fileicon(w, xfersend_FIcon, xfersend__filetype);
+  wimpt_safe(wimp_set_icon_state(w, xfersend_FIcon, 0, 0));
+}
+
+BOOL xfersend_x(
+  int                filetype,
+  char *             name,
+  int                estsize,
+  xfersend_saveproc  saveproc,
+  xfersend_sendproc  sendproc,
+  xfersend_printproc printproc,
+  void *             savehandle,
+  dbox               d,
+  xfersend_clickproc clickproc,
+  void *             clickhandle)
+{
+  dbox_field f;
+  char filename[216];
+
+  xfersend__filetype      = filetype;
+  xfersend__estsize       = estsize;
+  xfersend__saveproc      = saveproc;
+  xfersend__savehandle    = savehandle;
+  xfersend__sendproc      = sendproc;
+  xfersend__printproc     = printproc;
+  xfersend_.d             = d;
+  xfersend_.clickproc     = clickproc;
+  xfersend_.clickhandle   = clickhandle;
+
+  /* set up icons in dbox */
+  dbox_setfield(d, (dbox_field) xfersend_FName, name);
+  xfersend__fileicon(d);
+
+  dbox_raw_eventhandlers(d, &xfersend_.oldraw_handler_proc, &xfersend_.oldhandle);
+
+  dbox_raw_eventhandler(d, xfersend__wimpevents, NULL);
+
+  while ((f = dbox_fillin(d)) != dbox_CLOSE)
+  {
+    if (f == dbox_OK)
+    {
+      dbox_getfield(d, (dbox_field) xfersend_FName, filename, sizeof(filename));
+
+      /* complain if not rooted */
+      if (!xfersend__file_is_rooted(filename))
+      {
+        werr(FALSE, msgs_lookup("saveas1" /*":To save, drag the icon to a directory viewer"*/));
+        continue;
+      }
+
+      xfersend__fileissafe = TRUE;        /* bog standard save to real file */
+
+      if (xfersend__saveproc (filename, xfersend__savehandle)  &&
+          !dbox_persist())
+      {
+        tracef0 ("file saved ok but not persisting - terminate xfersend loop\n");
+        break;
+      }
+    }
+    else if (f == xfersend_.cancel_button)
+    {
+      tracef0 ("click on cancel button terminates xfersend loop\n");
+      break;
+    }
+    else if (xfersend_.clickproc)
+    {
+      filetype = xfersend__filetype;
+      xfersend_.clickproc (d, f, &xfersend__filetype, xfersend_.clickhandle);
+      if(filetype != xfersend__filetype)
+        xfersend__fileicon(d);
+    }
+    else
+    {
+      tracef0 ("strange click terminates xfersend loop\n");
+      break;
+    }
+  }
+
+  tracef0 ("xfersend loop done\n");
+
+  xfersend_.cancel_button = dbox_OK; /* reset to default in any case */
+
+  /* Might have escaped the loop above still owning unknown events. */
+  if (xfersend_.ukproc != NULL)
+  {
+    xfersend_.ukproc = FALSE;
+    win_remove_unknown_event_processor(xfersend__unknowns, d);
+  }
+
+  return (f == dbox_OK);
+}
+
+#ifndef SKS_ACW
+
 BOOL xfersend (int filetype, char *filename, int estsize,
    xfersend_saveproc saver, xfersend_sendproc sender, xfersend_printproc printer,
    wimp_eventstr *e, void *handle)
@@ -636,6 +838,8 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 BOOL xfersend_file_is_safe(void)
 {
   return xfersend__fileissafe;
@@ -657,4 +861,95 @@
   return xfersend__msgid;        /* my_ref of last DataSave message */
 }
 
+extern void xfersend_set_cancel_button(dbox_field f)
+{
+  xfersend_.cancel_button = f; 
+}
+
+/* Starts a drag using DragASprite */
+
+static int dragging_using_dragasprite = FALSE;
+
+static os_error *nd_dragstart(wimp_dragstr dr, int filetype)
+{
+    os_regset r;
+    os_error * e;
+    char temp[32];
+
+    r.r[0]=filetype;
+    r.r[1]=(int) temp+4;
+    r.r[2]=sizeof(temp)-5;
+    os_swix(OS_ConvertHex4, &r);
+    memcpy(temp, "file_", 5);
+
+    r.r[0]=0x85;
+    r.r[1]=1;
+    r.r[2]=(int) temp;
+    r.r[3]=(int) &(dr.box);
+    r.r[4]=0;
+    e=os_swix(DragASprite_Start, &r);
+
+    if(!e)
+        dragging_using_dragasprite = TRUE;
+
+    return(e);
+}
+
+/* Finishes a DragASprite call */
+
+static void nd_dragstop(void)
+{
+    if(dragging_using_dragasprite)
+    {
+        dragging_using_dragasprite = FALSE;
+
+        os_swi0(DragASprite_Stop);
+    }
+}
+
+static BOOL xfersend__wimp_EBUT(dbox d, wimp_eventstr *e)
+{
+  tracef1 ("xfersend EBUT &%x\n", e->data.but.m.bbits);
+
+  if( (e->data.but.m.bbits == wimp_BDRAGLEFT) &&
+      (e->data.but.m.i     == xfersend_FIcon) )
+  {
+    wimp_dragstr dr;
+    wimp_wstate wstate;
+    wimp_icon icon;
+    wimp_w w;
+
+    win_add_unknown_event_processor(xfersend__unknowns, d);
+
+    w = dbox_syshandle(d);
+
+    tracef0 ("Initiate a drag.\n");
+
+    xfersend_.ukproc = TRUE;
+
+    wimpt_safe(wimp_get_wind_state(w, &wstate));
+    wimpt_safe(wimp_get_icon_info(w, xfersend_FIcon, &icon));
+    dr.window = w; /* not relevant */
+    dr.type   = wimp_USER_FIXED;
+    dr.box.x0 = wstate.o.box.x0 - wstate.o.x + icon.box.x0;
+    dr.box.y0 = wstate.o.box.y1 - wstate.o.y + icon.box.y0;
+    dr.box.x1 = wstate.o.box.x0 - wstate.o.x + icon.box.x1;
+    dr.box.y1 = wstate.o.box.y1 - wstate.o.y + icon.box.y1;
+    dr.parent.x0 = 0;
+    dr.parent.y0 = 0;
+    dr.parent.x1 = bbc_vduvar(bbc_XWindLimit) << bbc_vduvar(bbc_XEigFactor);
+    dr.parent.y1 = bbc_vduvar(bbc_YWindLimit) << bbc_vduvar(bbc_YEigFactor);
+
+    /* nd 19-07-1996 Added in support for DragASprite */
+    if(!xfersend__using_dragasprite() || (NULL != nd_dragstart(dr, xfersend__filetype)))
+        wimpt_safe(wimp_drag_box(&dr));
+
+    return TRUE;
+  }
+  else
+    tracef0 ("not a drag on the file icon - ignored\n");
+
+  return FALSE;
+}
+
 /* end xfersend.c */
