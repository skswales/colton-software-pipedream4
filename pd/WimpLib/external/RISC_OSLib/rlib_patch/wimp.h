--- _src	2011-11-13 19:13:59.000000000 +0000
+++ _dst	2015-11-18 16:29:49.260000000 +0000
@@ -221,7 +221,11 @@
 typedef struct {
   wimp_w w;               /* window handle */
   wimp_box box;           /* position on screen of visible work area */
+#if !defined(SKS_ACW) || defined(COMPILING_WIMPLIB)
   int x, y;               /* 'real' coordinates of visible work area */
+#else /* SKS_ACW */
+  int scx, scy;           /* 'real' coordinates of visible work area (scroll offsets) */
+#endif /* SKS_ACW */
   wimp_w behind;          /* handle of window to go behind (-1 = top,
                            * -2 = bottom) */
 } wimp_openstr;
@@ -314,6 +318,9 @@
   wimp_MDEVICECLAIM  = 11, /* Broadcast before an application can claim parallel port, RS232 port etc. */
   wimp_MDEVICEINUSE  = 12, /* Reply if another application is already using the device */
   wimp_MDATASAVED    = 13, /* A file previously saved has become 'safe' */
+#ifdef SKS_ACW
+  wimp_MShutDown     = 14,
+#endif /* SKS_ACW */
 
   wimp_FilerOpenDir  = 0x0400,
   wimp_FilerCloseDir = 0x0401,
@@ -349,10 +356,14 @@
   wimp_MICONIZEAT    = 0x400d0,
   wimp_MTOGGLEBACKDROP         = 0x400d1,
   wimp_MSCREENEDGENOTIFICATION = 0x400d2,
-  
+
   wimp_MHELPREQUEST  = 0x502,         /* interactive help request */
   wimp_MHELPREPLY    = 0x503,         /* interactive help message */
 
+#if defined(SKS_ACW)
+  wimp_MPD_DDE       = 0x600,
+#endif
+
   /* Messages for dialogue with printer applications */
 
   wimp_MPrintFile       = 0x80140,    /* Printer app's first response to */
@@ -376,7 +387,6 @@
 } wimp_msghdr;
 /* size is the size of the whole msgstr, see below. */
 
-
 typedef struct {
   wimp_w w;               /* window in which save occurs. */
   wimp_i i;               /* icon there */
@@ -425,8 +435,18 @@
   int nbyteswritten;         /* number of bytes written */
 } wimp_msgramtransmit;
 
+#ifdef SKS_ACW
+typedef struct {
+  int flags;                 /* where the help is required */
+} wimp_msgprequitrequest;
+
+#define wimp_MPREQUIT_flags_killthistask 0x01
+#define wimp_MPREQUIT_flags int
+#endif /* SKS_ACW */
+
 typedef struct {           /* Save state for restart */
   int filehandle;          /* RISC OS file handle (not a C one!) */
+  int flags;
 } wimp_msgsavedesk;
 
 typedef struct {
@@ -440,7 +460,11 @@
 } wimp_msghelprequest;
 
 typedef struct {
+#ifndef SKS_ACW
   char text[200];        /* the helpful string */
+#else /* SKS_ACW */
+  char text[236];        /* the helpful string */
+#endif /* SKS_ACW */
 } wimp_msghelpreply;
 
 typedef struct {         /* structure used in all print messages */
@@ -449,6 +473,74 @@
   char name[256-44] ;    /* filename */
 } wimp_msgprint ;
 
+#ifdef SKS_ACW /* Need copy up here ... */
+typedef struct wimp_menustr *wimp_menuptr;
+/* Only for the circular reference in menuitem/str. */
+#endif /* SKS_ACW */
+
+typedef struct {
+  union {
+    wimp_menuptr m;
+    wimp_w w;
+  } submenu;
+  int x;                      /* where to open the submenu */
+  int y;
+  int menu[10];               /* list of menu selection indices, -1 at end */
+} wimp_msgmenuwarning;
+
+typedef struct {
+  void *CAO;
+  int size;
+  char taskname[236 - 2*4];
+} wimp_msgtaskinit;
+
+typedef struct {
+  int newcurrent;
+  int newnext;
+} wimp_msgslotsize;
+
+typedef struct {
+  wimp_w w;
+  wimp_t window_owner;
+  char title[20];
+} wimp_msgiconize;
+
+typedef struct {
+  wimp_w w;
+  int reserved_0;
+  char sprite[8]; /* doesn't include ic_ */
+  char title[20];
+} wimp_msgwindowinfo;
+
+typedef struct {
+  wimp_w w;
+} wimp_msgwindowclosed;
+
+typedef struct {
+    os_error oserror;
+} wimp_msgprinterror;
+
+#ifdef SKS_ACW /* SKS for PipeDream */
+
+typedef union {
+    char chars[236];
+    int words[59];             /* max data size. */
+    wimp_msgdatasave    datasave;
+    wimp_msgdatasaveok  datasaveok;
+    wimp_msgdataload    dataload;
+    wimp_msgdataopen    dataopen;
+    wimp_msgramfetch    ramfetch;
+    wimp_msgramtransmit ramtransmit;
+    wimp_msghelprequest helprequest;
+    wimp_msghelpreply   helpreply;
+    wimp_msgprint       print;
+    wimp_msgsavedesk    savedesk;
+    wimp_msgdevice      device;
+
+} wimp_msgstr_data1;
+
+#endif /* SKS_ACW */
+
 typedef struct {          /* message block */
   wimp_msghdr hdr;
   union {
@@ -465,12 +557,38 @@
     wimp_msgprint       print;
     wimp_msgsavedesk    savedesk;
     wimp_msgdevice      device;
+#ifdef SKS_ACW
+    wimp_msgpd_dde          pd_dde;
+    wimp_msgwindowinfo      windowinfo;
+    wimp_msgwindowclosed    windowclosed;
+    wimp_msgprequitrequest  prequitrequest;
+    wimp_msgtaskinit        taskinit;
+    wimp_msgmenuwarning     menuwarning;
+    wimp_msgiconize         iconize;
+    wimp_msgdatasave        printsave;
+    wimp_msgprinterror      printerror;
+#endif /* SKS_ACW */
   } data;
 } wimp_msgstr;
 
-typedef union {
+typedef union _wimp_eventdata {
     wimp_openstr o;         /* for redraw, close, enter, leave events */
-    struct {
+
+#ifdef SKS_ACW
+    struct _wimp_eventdata_redraw {     /* SKS for redraw events */
+        wimp_w          w;
+    } redraw;
+
+    struct _wimp_eventdata_close {      /* SKS for close events */
+        wimp_w          w;
+    } close;
+
+    struct _wimp_eventdata_pointer {    /* SKS for pointer enter, leave events */
+        wimp_w          w;
+    } pointer;
+#endif /* SKS_ACW */
+
+    struct _wimp_eventdata_but {        /* SKS tag for PipeDream */
       wimp_mousestr m;
       wimp_bbits b;} but;   /* for button change event */
     wimp_box dragbox;       /* for user drag box event */
@@ -508,8 +626,10 @@
 /* use wimp_INOSELECT to shade the item as unselectable,
 and the button type to mark it as writable. */
 
+#ifndef SKS_ACW
 typedef struct wimp_menustr *wimp_menuptr;
 /* Only for the circular reference in menuitem/str. */
+#endif /* SKS_ACW */
 
 typedef struct {
   wimp_menuflags flags;         /* menu entry flags */
@@ -831,6 +951,32 @@
    It is the application's responsibility to set the tag correctly.
  */
 
+#ifdef SKS_ACW
+
+/* SKS 10 Feb 1989 Corrected wimp_openstr scx,scy
+ * SKS 14 Jan 1998 Corrected wimp_IESGMASK definition
+ * SKS 17 Oct 1999 Corrected datasave leaf[] size for RISC OS 4
+ */
+
+#define wimp_IBUTMASK (0xF * wimp_IBTYPE)
+#define wimp_IESGMASK (0x1F * wimp_IESG)
+#define wimp_IFOREMASK 0x0F000000
+#define wimp_IBACKMASK 0xF0000000U
+
+/*
+sizes of wimp controlled areas of screen - slightly mode dependent
+*/
+
+#define wimp_iconbar_height 96
+
+#define wimp_win_title_height(dy) (40 + (dy))
+
+#define NULL_WIMP_W  ((wimp_w) 0)
+
+os_error *wimp_reporterror_rf(const os_error *, wimp_errflags, const char *name, wimp_errflags *flags_out, const char *message);
+
+#endif /* SKS_ACW */
+
 # endif
 
 /* end wimp.h */
