--- _src	2009-05-31 18:58:59.000000000 +0100
+++ _dst	2016-09-16 14:50:26.960000000 +0100
@@ -35,6 +35,10 @@
  *
  */
 
+#include "include.h" /* for SKS_ACW */
+
+#undef OS_ReadVarVal /* SKS otherwise clashes below */
+
 #include <string.h>
 #include <stdio.h>
 
@@ -81,7 +85,7 @@
 static int xferrecv_msgid ;
 
 static int scrap_ref = 0 ;
-static int xferrecv__fileissafe ;
+static int xferrecv__fileissafe = TRUE; /* SKS for PipeDream */
 
 int xferrecv_checkinsert(char **filename)
 {
@@ -174,7 +178,7 @@
   tracef2("xferrecv_checkimport returning filetype %x from %d\n",
            e->data.msg.data.datasave.type,xferrecv_sendertask) ;
 
-  return  xferrecv_ack.data.datasaveok.type = e->data.msg.data.datasave.type ;
+  return  xferrecv_ack.data.datasaveok.type; /* SKS superfluous assignment removed */
  }
  else return -1 ;
 }
@@ -186,7 +190,7 @@
  int size = xferrecv_ack.data.ramfetch.nbytes ;
  int action = xferrecv_ack.hdr.action ;
 
- tracef2("xferrecv_sendRAMFETCH with buffer %x, size %d :",
+ tracef2("xferrecv_sendRAMFETCH with buffer %x, size %d : ",
           (int) xferrecv_buffer,xferrecv_buffersize) ;
 
  xferrecv_ack.hdr.action = wimp_MRAMFETCH ;
@@ -209,7 +213,7 @@
  handle = handle ;
 
 #if TRACE
- tracef("xferrecv_unknown_events %d %d %d %d %d\n",e->e,
+ tracef5("xferrecv_unknown_events %d %d %d %d %d\n",e->e,
          e->data.msg.hdr.action,e->data.msg.hdr.my_ref,
          e->data.msg.hdr.your_ref,xferrecv_msgid) ;
 #endif
@@ -266,7 +270,7 @@
    os_swix(OS_ReadVarVal+os_X, &r) ;
 
    if (r.r[2]==0)
-     werr(0,msgs_lookup(MSGS_xferrecv1)) ;
+     werr(0,msgs_lookup(MSGS_xferrecv1));
    else
    {
     strcpy(xferrecv_ack.data.datasaveok.name, "<Wimp$Scrap>") ;
@@ -278,7 +282,7 @@
   else
   {
    tracef0("tell the user the protocol fell down\n") ;
-   werr(0, msgs_lookup(MSGS_xferrecv2)) ;
+   werr(0, msgs_lookup(MSGS_xferrecv2));
   }
   xferrecv_state = xferrecv_state_Broken ;
  }
@@ -289,6 +293,7 @@
 
 int xferrecv_doimport(char *buf, int size, xferrecv_buffer_processor p)
 {
+ xferrecv__fileissafe = FALSE ;
 
 /* Receives data into the buffer; calls the buffer processor if the buffer
    given becomes full. Returns TRUE if the transaction completed sucessfully.
@@ -300,9 +305,11 @@
  xferrecv_processbuff = p ;
  xferrecv_buffer = buf ;
  xferrecv_buffersize = size ;
- xferrecv_sendRAMFETCH() ;
+
  xferrecv_state = xferrecv_state_Ask ;
- xferrecv__fileissafe = FALSE ;
+
+ /* send me some data */
+ xferrecv_sendRAMFETCH() ;
 
  do
  {
@@ -319,4 +326,58 @@
  return xferrecv__fileissafe ;
 }
 
-/* end */
+#ifdef SKS_ACW
+
+/*
+SKS for PipeDream
+*/
+
+extern int
+xferrecv_import_via_file(const char * filename)
+{
+    if(NULL == filename)
+    {
+        os_regset r ;
+        tracef0("xferrecv_import_via_file asking for Wimp$Scrap transfer\n") ;
+
+        xferrecv__fileissafe = 0;
+
+        /* first check that variable exists */
+        r.r[0] = (int) "Wimp$Scrap" ;
+        r.r[1] = NULL ;
+        r.r[2] = -1 ;
+        r.r[3] = 0 ;
+        r.r[4] = 0 ;          /* don't use 3 cos buffer size unsigned for macro expansion (fixed in 4.15) */
+        os_swix(OS_ReadVarVal+os_X, &r) ;
+
+        if (r.r[2]==0)
+          werr(0,msgs_lookup(MSGS_xferrecv1));
+        else
+        {
+         /* rest of ack has been set up by checkimport */
+         strcpy(xferrecv_ack.data.datasaveok.name, "<Wimp$Scrap>") ;
+         xferrecv_ack.data.datasaveok.estsize = -1 ; /* file is not safe with us */
+         (void) wimp_sendmessage(wimp_ESEND, &xferrecv_ack, xferrecv_sendertask) ;
+         /* now message is sent, log the ref */
+         scrap_ref = xferrecv_ack.hdr.my_ref ;
+        }
+
+        return(-1);
+    }
+
+    tracef1("[xferrecv_import_via_file(%s) entered]\n", filename);
+
+    xferrecv__fileissafe = 1;
+    scrap_ref = 0;
+
+    /* rest of ack has been set up by checkimport */
+    strcpy(xferrecv_ack.data.datasaveok.name, filename);
+    /* file **will** be safe with us */
+    (void) wimp_sendmessage(wimp_ESEND, &xferrecv_ack, xferrecv_sendertask);
+
+    return(-1);
+}
+
+#endif /* SKS_ACW */
+
+/* end of xferrecv.c */
