--- _src	2022-10-26 16:47:18.790000000 +0000
+++ _dst	2022-10-31 17:46:49.430000000 +0000
@@ -31,22 +31,79 @@
  * History: IDJ: 07-Feb-92: prepared for source release
  */
 
-#define BOOL int
-#define TRUE 1
-#define FALSE 0
+#include "include.h" /* for SKS_ACW */
+
+// SKS_ACW #define BOOL int
+// SKS_ACW #define TRUE 1
+// SKS_ACW #define FALSE 0
 
 #include <stdlib.h>
 #include <string.h>
 #include <stdarg.h>
 
 #include "os.h"
-#include "trace.h"
+/*#include "trace.h"*/
 #ifndef __wimp_h
 #define __WIMP_COMPILE
 #include "wimp.h"
 #endif
 
 /*                          W I M P    S W I 's                            */
+#ifdef SKS_ACW
+/* Always done in XWimp form */
+#define Initialise          (0x000400C0 | os_X)
+#define CreateWindow        (0x000400C1 | os_X)
+#define CreateIcon          (0x000400C2 | os_X)
+#define DeleteWindow        (0x000400C3 | os_X)
+#define DeleteIcon          (0x000400C4 | os_X)
+#define OpenWindow          (0x000400C5 | os_X)
+#define CloseWindow         (0x000400C6 | os_X)
+#define Poll                (0x000400C7 | os_X)
+#define RedrawWindow        (0x000400C8 | os_X)
+#define UpdateWindow        (0x000400C9 | os_X)
+#define GetRectangle        (0x000400CA | os_X)
+#define GetWindowState      (0x000400CB | os_X)
+#define GetWindowInfo       (0x000400CC | os_X)
+#define SetIconState        (0x000400CD | os_X)
+#define GetIconState        (0x000400CE | os_X)
+#define GetPointerInfo      (0x000400CF | os_X)
+#define DragBox             (0x000400D0 | os_X)
+#define ForceRedraw         (0x000400D1 | os_X)
+#define SetCaretPosition    (0x000400D2 | os_X)
+#define GetCaretPosition    (0x000400D3 | os_X)
+#define CreateMenu          (0x000400D4 | os_X)
+#define DecodeMenu          (0x000400D5 | os_X)
+#define WhichIcon           (0x000400D6 | os_X)
+#define SetExtent           (0x000400D7 | os_X)
+#define SetPointerShape     (0x000400D8 | os_X)
+#define OpenTemplate        (0x000400D9 | os_X)
+#define CloseTemplate       (0x000400DA | os_X)
+#define LoadTemplate        (0x000400DB | os_X)
+#define ProcessKey          (0x000400DC | os_X)
+#define CloseDown           (0x000400DD | os_X)
+#define StartTask           (0x000400DE | os_X)
+#define ReportError         (0x000400DF | os_X)
+#define GetWindowOutline    (0x000400E0 | os_X)
+#define PollIdle            (0x000400E1 | os_X)
+#define PlotIcon            (0x000400E2 | os_X)
+#define SetMode             (0x000400E3 | os_X)
+#define SetPalette          (0x000400E4 | os_X)
+#define ReadPalette         (0x000400E5 | os_X)
+#define SetColour           (0x000400E6 | os_X)
+#define SendMessage         (0x000400E7 | os_X)
+#define CreateSubMenu       (0x000400E8 | os_X)
+#define SpriteOp            (0x000400E9 | os_X)
+#define BaseOfSprites       (0x000400EA | os_X)
+#define BlockCopy           (0x000400EB | os_X)
+#define SlotSize            (0x000400EC | os_X)
+#define ReadPixTrans        (0x000400ED | os_X)
+#define ClaimFreeMemory     (0x000400EE | os_X)
+#define CommandWindow       (0x000400EF | os_X)
+#define TextColour          (0x000400F0 | os_X)
+#define TransferBlock       (0x000400F1 | os_X)
+#define ReadSysInfo         (0x000400F2 | os_X)
+#define SetFontColours      (0x000400F3 | os_X)
+#else
 #define Initialise          0x000400C0
 #define CreateWindow        0x000400C1
 #define CreateIcon          0x000400C2
@@ -99,6 +156,9 @@
 #define TransferBlock       (0x000400c0+49)
 #define ReadSysInfo         (0x000400c0+50)
 #define SetFontColours      (0x000400c0+51)
+#endif /* SKS_ACW */
+
+#ifndef SKS_ACW
 
 #pragma -s1
 
@@ -118,7 +178,7 @@
 #pragma -s0
 
 #ifndef UROM
-os_error *wimp_taskinit (char *name, int *version, wimp_t *t, wimp_msgaction *messages)
+os_error *wimp_taskinit(char *name, int *version, wimp_t *t, wimp_msgaction *messages)
 {
   os_regset r;
   os_error *e;
@@ -142,10 +202,15 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 #pragma -s1
 
 os_error * wimp_create_wind(wimp_wind * w, wimp_w * result)
 {
+#if defined(SKS_ACW)
+  return(tbl_wimp_create_window((WimpWindow *) w, (int *) result));
+#else
   os_regset r;
   os_error *e;
 
@@ -156,10 +221,14 @@
   *result = r.r[0];
 
   return(e);
+#endif /* SKS_ACW */
 }
 
 os_error * wimp_create_icon(wimp_icreate * i, wimp_i * result)
 {
+#if defined(SKS_ACW)
+  return(tbl_wimp_create_icon(0, i, (int *) result));
+#else
   os_regset r;
   os_error *e;
 
@@ -170,10 +239,16 @@
   *result = r.r[0];
 
   return(e);
+#endif /* SKS_ACW */
 }
 
 os_error * wimp_delete_wind(wimp_w w)
 {
+#if defined(SKS_ACW)
+    WimpDeleteWindowBlock block;
+    block.window_handle = w;
+    return(tbl_wimp_delete_window(&block));
+#else
     os_regset r;
     os_error *e;
 
@@ -182,10 +257,17 @@
     e = os_swix(DeleteWindow, &r);
 
     return(e);
+#endif /* SKS_ACW */
 }
 
 os_error * wimp_delete_icon(wimp_w w, wimp_i i)
 {
+#if defined(SKS_ACW)
+    WimpDeleteIconBlock block;
+    block.window_handle = w;
+    block.icon_handle = i;
+    return(tbl_wimp_delete_icon(&block));
+#else
     os_regset r;
     os_error *e;
     int j[2];
@@ -193,11 +275,13 @@
     j[0] = (int)w;
     j[1] = (int)i;
 
+    r.r[0] = 0;
     r.r[1] = (int) j;
 
     e = os_swix(DeleteIcon, &r);
 
     return(e);
+#endif /* SKS_ACW */
 }
 
 os_error * wimp_open_wind(wimp_openstr * o)
@@ -205,6 +289,7 @@
     os_regset r;
     os_error *e;
 
+    r.r[0] = 0;
     r.r[1] = (int)o;
 
     e = os_swix(OpenWindow, &r);
@@ -218,6 +303,7 @@
     os_regset r;
     os_error *e;
 
+    r.r[0] = 0;
     r.r[1] = (int)&w;
 
     e = os_swix(CloseWindow, &r);
@@ -235,6 +321,7 @@
 
     e = os_swix(RedrawWindow, &r);
 
+    if(!e)
     *result = r.r[0];
 
     return(e);
@@ -250,6 +337,7 @@
 
     e = os_swix(UpdateWindow, &r);
 
+    if(!e)
     *result = r.r[0];
 
     return(e);
@@ -265,6 +353,7 @@
 
     e = os_swix(GetRectangle, &r);
 
+    if(!e)
     *result = r.r[0];
 
     return(e);
@@ -278,6 +367,7 @@
 
     result->o.w = w;
 
+    r.r[0] = 0;
     r.r[1] = (int)result;
 
     e = os_swix(GetWindowState, &r);
@@ -291,6 +381,7 @@
     os_regset r;
     os_error *e;
 
+    r.r[0] = 0;
     r.r[1] = (int)result;
     e = os_swix(GetWindowInfo, &r);
 
@@ -318,6 +409,7 @@
     b.flags_v = value;
     b.flags_m = mask;
 
+    r.r[0] = 0;
     r.r[1] = (int)&b;
 
     e = os_swix(SetIconState, &r);
@@ -346,6 +438,7 @@
 
   e = os_swix(GetIconState, &r);
 
+  if(!e)
   *result = b.icon_s;
 
   return(e);
@@ -367,6 +460,7 @@
 
   e = os_swix(GetPointerInfo, &r);
 
+  if(!e)
   *result = m.m;
   return(e);
 }
@@ -377,6 +471,7 @@
   os_regset r;
   os_error *e;
 
+  r.r[0] = 0;
   r.r[1] = (int)d;
 
   e = os_swix(DragBox, &r);
@@ -385,6 +480,8 @@
 }
 
 
+#ifndef SKS_ACW
+
 os_error * wimp_force_redraw(wimp_redrawstr * r)
 {
   os_error *e;
@@ -394,6 +491,8 @@
   return(e);
 }
 
+#endif /* SKS_ACW */
+
 
 os_error * wimp_set_caret_pos(wimp_caretstr * c)
 {
@@ -410,6 +509,7 @@
   os_regset r;
   os_error *e;
 
+  r.r[0] = 0;
   r.r[1] = (int)c;
 
   e = os_swix(GetCaretPosition, &r);
@@ -423,6 +523,7 @@
   os_regset r;
   os_error *e;
 
+  r.r[0] = 0;
   r.r[1] = (int)m;
   r.r[2] = x;
   r.r[3] = y;
@@ -497,6 +598,7 @@
   os_regset r;
   os_error *e;
 
+  r.r[0] = 0;
   r.r[1] = (int)name;
 
   e = os_swix(OpenTemplate, &r);
@@ -527,7 +629,9 @@
 
 os_error *wimp_processkey(int chcode)
 {
-  return os_swi1(ProcessKey, chcode);
+    os_regset r;
+    r.r[0] = chcode;
+    return(os_swix(ProcessKey, &r));
 }
 
 #ifndef UROM
@@ -556,6 +660,8 @@
     return e;
 }
 
+#if !defined(NORCROFT_INLINE_SWIX) /* SKS_ACW */
+
 os_error *wimp_starttask(char *clicmd)
 {
   os_regset r;
@@ -566,11 +672,16 @@
   return e;
 }
 
+#endif /* NORCROFT_INLINE_SWIX */ /* SKS_ACW */
+
+#ifndef SKS_ACW
+
 os_error *wimp_getwindowoutline(wimp_redrawstr *re)
 {
   os_regset r;
   os_error *e;
 
+  r.r[0] = 0;
   r.r[1] = (int) re;
   e = os_swix(GetWindowOutline, &r);
   return e;
@@ -581,6 +692,7 @@
   os_regset r;
   os_error *e;
 
+  r.r[0] = 0;
   r.r[1] = (int) i;
   e = os_swix(PlotIcon, &r);
   return e;
@@ -598,6 +710,8 @@
 }
 #endif
 
+#endif /* SKS_ACW */
+
 os_error *wimp_readpalette(wimp_palettestr* p)
 {
   os_regset r;
@@ -610,6 +724,8 @@
   return e;
 }
 
+#ifndef SKS_ACW
+
 #ifndef UROM
 os_error *wimp_setpalette(wimp_palettestr* p)
 {
@@ -673,15 +789,68 @@
   return os_swi3(os_X | ReportError, (int) er, flags, (int) name);
 }
 
+#endif /* SKS_ACW */
+
 os_error *wimp_sendmessage(wimp_etype code, wimp_msgstr* m, wimp_t dest)
 {
-  return os_swi3(os_X | SendMessage, code, (int) m, dest);
+    os_regset r;
+    os_error *e;
+
+    tracef3("sending event %s message %s to task &%p: ", report_wimp_event_code(code),
+            (code >= wimp_ESEND) ? report_wimp_message(m, TRUE) : report_wimp_event(code, m),
+            dest);
+
+    if(code >= wimp_ESEND)
+    {
+        /* round up to word size for clients */
+        if(m->hdr.size & 3)
+            m->hdr.size = (m->hdr.size + 3) & ~3;
+    }
+
+    r.r[0] = code;
+    r.r[1] = (int) m;
+    r.r[2] = dest;
+
+    e = os_swix(SendMessage, &r);
+
+#if TRACE
+    if(!e)
+        tracef2("got my_ref %d, my task id &%p\n", m->hdr.my_ref, m->hdr.task);
+#endif
+
+    return(e);
 }
 
 os_error *wimp_sendwmessage(
   wimp_etype code, wimp_msgstr* m, wimp_w w, wimp_i i)
 {
-  return os_swi4(os_X | SendMessage, code, (int) m, w, i);
+    os_regset r;
+    os_error *e;
+
+    tracef4("sending event %s message %s to window %d icon %d: ", report_wimp_event_code(code),
+            (code >= wimp_ESEND) ? report_wimp_message(m, TRUE) : report_wimp_event(code, m),
+            (int) w, (int) i);
+
+    if(code >= wimp_ESEND)
+    {
+        /* round up to word size for clients */
+        if(m->hdr.size & 3)
+            m->hdr.size = (m->hdr.size + 3) & ~3;
+    }
+
+    r.r[0] = code;
+    r.r[1] = (int) m;
+    r.r[2] = w;
+    r.r[3] = i;
+
+    e = os_swix(SendMessage, &r);
+
+#if TRACE
+    if(!e)
+        tracef2("got my_ref %d, my task id &%p\n", m->hdr.my_ref, m->hdr.task);
+#endif
+
+    return(e);
 }
 
 os_error *wimp_create_submenu(wimp_menustr *sub, int x, int y)
@@ -696,6 +865,8 @@
   return e;
 }
 
+#ifndef SKS_ACW /* see riscasm */
+
 os_error *wimp_slotsize(int *currentslot /*inout*/,
                         int *nextslot /*inout*/,
                         int *freepool /*out*/) {
@@ -705,6 +876,8 @@
   return e;
 }
 
+#endif /* SKS_ACW */
+
 os_error *wimp_transferblock(
   wimp_t sourcetask,
   char *sourcebuf,
@@ -735,11 +908,19 @@
  return os_swix(SpriteOp, r) ;
 }
 
+#ifndef SKS_ACW
+
 os_error *wimp_setfontcolours(int foreground, int background)
 {
+#if defined(NORCROFT_INLINE_SWIX)
+    return(_swix(Wimp_SetFontColours, _INR(0, 2), 0, background, foreground));
+#else
   return os_swi3(os_X | SetFontColours, 0, background, foreground);
+#endif
 }
 
+#endif /* SKS_ACW */
+
 os_error *wimp_readpixtrans(sprite_area *area, sprite_id *id,
                          sprite_factors *factors, sprite_pixtrans *pixtrans)
 {
