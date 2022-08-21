--- _src	2019-07-29 19:42:18.000000000 +0100
+++ _dst	2019-07-30 11:45:39.010000000 +0100
@@ -31,6 +31,10 @@
  * History: IDJ: 06-Feb-92: prepared for source release
  */
 
+/*  SKS big hack 15-May-89 to not explode with fatal errors */
+
+#include "include.h" /* for SKS_ACW */
+
 #define BOOL int
 #define TRUE 1
 #define FALSE 0
@@ -91,21 +95,28 @@
   }
 }
 
-static void menu__preparecopy(menu from, menu to)
+static BOOL menu__preparecopy(menu from, menu to)
 {
   /* Allocate space in the destination to suit the copy. */
   to->m = malloc(sizeof(wimp_menuhdr) + from->nitems * sizeof(wimp_menuitem));
   if (to->m == 0) {
-    werr(TRUE, msgs_lookup(MSGS_menu1));
+#ifdef SKS_ACW
+    return(FALSE); /*SKS no longer fatal*/
+#endif /* SKS_ACW */
   }
   if (from->nbytes != 0) {
     to->entryspace = malloc(from->nbytes);
     if (to->entryspace == 0) {
-      werr(TRUE, msgs_lookup(MSGS_menu1));
+      free(to->m);
+      to->m = NULL;
+#ifdef SKS_ACW
+      return(FALSE); /*SKS no longer fatal*/
+#endif /* SKS_ACW */
     }
   } else {
     to->entryspace = 0;
   }
+  return(TRUE);
 }
 
 static void menu__copydata(menu from, menu to)
@@ -131,8 +142,18 @@
 
 /* The work area is allocated on the stack, with the following limits: */
 
+#ifdef SKS_ACW /* was originally inadequate */
+#define WORKAREASIZE  10*1024
+#define AVGINDSIZE    32         /* estimated average size of indirected entry */
+#define MAXITEMS      ((WORKAREASIZE - sizeof(menu__str) - sizeof(wimp_menuhdr)) / \
+                       (sizeof(wimp_menuitem) + AVGINDSIZE))
+                                 /* max size of a menu: ca. 192 - surely OK. */
+#define MAXENTRYSPACE (MAXITEMS * AVGINDSIZE)
+                                 /* space for building entries > 12 chars */
+#else /* SKS_ACW */
 #define MAXITEMS 64       /* max size of a menu: surely OK. */
 #define MAXENTRYSPACE 1024 /* space for building entries > 12 chars */
+#endif /* SKS_ACW */
 
 typedef struct {
   menu__str m;
@@ -143,9 +164,10 @@
 
 static void menu__initworkarea(menu__workarea *w)
 {
+  tracef3("menu__initworkarea(&%p): MAXITEMS %d, sizeof(menu__workarea) = %d\n", w, MAXITEMS, sizeof(menu__workarea));
   w->m.m = &w->menuhdr;
   w->m.nitems = 0;
-  w->m.entryspace = &w->entryspace;
+  w->m.entryspace = &w->entryspace[0];
   w->m.maxentrywidth = 0;
   /* insert a NIL in the entrySpace to distinguish sub-Menu pointers
   from text space. */
@@ -160,14 +182,18 @@
   menu__itemptr(&w->m, w->m.nitems-1)->flags &= ~wimp_MLAST;
 }
 
-static void menu__copyworkarea(menu__workarea *w /*in*/, menu m /*out*/)
+static BOOL menu__copyworkarea(menu__workarea *w /*in*/, menu m /*out*/)
 {
   if (w->m.nitems > 0) {
     menu__itemptr(&w->m, w->m.nitems-1)->flags |= wimp_MLAST;
   }
   menu__disposespace(m);
-  menu__preparecopy(&w->m, m);
+  if(!menu__preparecopy(&w->m, m))
+#ifdef SKS_ACW
+     return FALSE; /* SKS no longer fatal */
+#endif /* SKS_ACW */
   menu__copydata(&w->m, m);
+  return TRUE;
 }
 
 /* -------- Creating menu descriptions. -------- */
@@ -417,21 +443,34 @@
   menu__doextend(&menu__w, descr);
   m = malloc(sizeof(menu__str));
   if (m == 0) {
+#ifndef SKS_ACW
     werr(TRUE, msgs_lookup(MSGS_menu1));
+#else /* SKS_ACW */
+    return 0; /* SKS no longer fatal */
+#endif /* SKS_ACW */
   }
   m->m = 0;
   m->entryspace = 0;
-  menu__copyworkarea(&menu__w, m);
+  if(!menu__copyworkarea(&menu__w, m))
+    wlalloc_dispose((void **) &m);
+#ifndef SKS_ACW
   if (strlen(name) > 12) {
       *(char **)m->m->title = name;
       ptr = menu__itemptr(m, 0);
       ptr->flags |= wimp_MINDIRECTED;
   }
+#endif /* SKS_ACW */
   return m;
 }
 
 void menu_dispose(menu *m, int recursive)
 {
+#ifdef SKS_ACW /* SKS never recursive and extra arg checks */
+  if ((m == NULL) || (*m == NULL)) return;
+  if (recursive != 0) {
+  /*EMPTY*/
+  }
+#else /* SKS_ACW */
   if (recursive != 0) {
     menu *a = (menu*) ((*m)->entryspace);
     while (1) {
@@ -440,6 +479,7 @@
       menu_dispose(&subm, 1);
     }
   }
+#endif /* SKS_ACW */
   menu__disposespace(*m);
   free(*m);
 }
@@ -470,6 +510,8 @@
   }
 }
 
+#ifndef SKS_ACW
+
 void menu_make_writeable(menu m, int entry, char *buffer, int bufferlength,
                          char *validstring)
 {
@@ -501,9 +543,11 @@
   p->data.indirectsprite.nameisname = 1;
 }
 
+#endif /* SKS_ACW */
 
 void menu_submenu(menu m, int place, menu submenu)
 {
+#ifndef SKS_ACW
   int i;
   wimp_menuitem *p = menu__itemptr(m, place-1);
   menu__workarea menu__w;
@@ -524,6 +568,11 @@
     }
   }
   menu__copyworkarea(&menu__w, m);
+#else /* SKS_ACW */
+  /* SKS wildy different */
+  wimp_menuitem *p = menu__itemptr(m, place-1);
+  p->submenu = submenu ? (wimp_menustr *) submenu->m : (wimp_menustr *) submenu;
+#endif /* SKS_ACW */
 }
 
 void *menu_syshandle(menu m)
@@ -531,4 +580,131 @@
   return (void *) m->m;
 }
 
+#ifdef SKS_ACW
+
+/*
+SKS added for PipeDream
+*/
+
+extern menu
+menu_new_c(const char *name, const char *descr)
+{
+    return(menu_new(de_const_cast(char *, name), de_const_cast(char *, descr)));
+}
+
+extern menu
+menu_new_unparsed(const char *name, const char *descr)
+{
+    menu m;
+    menu__workarea menu__w;
+
+    tracef2("menu_new_unparsed(%s, %s)\n", name, descr);
+
+    menu__initworkarea(&menu__w);
+    menu__initmenu(de_const_cast(char *, name), &menu__w.m);
+
+    menu__additem(&menu__w, de_const_cast(char *, descr), strlen(descr));
+    /* Not menu__doextend, cos we don't want ',' or '|' treated as a separator */
+
+    m = malloc(sizeof(menu__str));
+    if(m)
+    {
+        m->m            = NULL;
+        m->entryspace   = NULL;
+
+        if(!menu__copyworkarea(&menu__w, m))
+            wlalloc_dispose((void **) &m);
+    }
+
+    tracef1("menu_new_unparsed() returns &%p\n", m);
+
+    return(m);
+}
+
+extern BOOL
+menu_extend_unparsed(menu * mm, const char *descr)
+{
+    menu m = *mm;
+    menu__workarea menu__w;
+    BOOL res;
+
+    tracef2("menu_extend_unparsed(&%p, %s)\n", m, descr);
+
+    if(m)
+    {
+        menu__copytoworkarea(m, &menu__w);
+
+        menu__additem(&menu__w, de_const_cast(char *, descr), strlen(descr));
+       /* Not menu__doextend, cos we don't want ',' or '|' treated as a separator */
+
+        res = menu__copyworkarea(&menu__w, m);
+
+        if(!res)
+            m = NULL;
+    }
+    else
+        res = FALSE;
+
+    *mm = m;
+
+    return(res);
+}
+
+extern void
+menu_settitle(menu m, const char * title)
+{
+    int i;
+
+    tracef2("menu_settitle(&%p, %s)\n", m, title);
+
+    if(!m)
+        return;
+
+    i = 0;
+    do  {
+        /* could use strncpy but for width calculation */
+        m->m->title[i] = title[i];
+        if (title[i] == CH_NULL) break;
+    }
+    while(++i < 12);
+
+    m->m->width = max(m->m->width, i * 16);
+}
+
+extern void
+menu_entry_changetext(menu m, int entry, const char *text)
+{
+    int i, length;
+    wimp_menuitem *p;
+
+    tracef3("menu_entry_settext(&%p, %d, %s)\n",
+                m, entry, text);
+
+    if(!m)
+        return;
+
+#if TRACE
+    if((entry == 0)  ||  (entry > m->nitems))
+    {
+        werr(FALSE, "duff entry %d in menu_settext (%d items) - ignored", entry, m->nitems);
+        return;
+    }
+#endif
+
+    p = menu__itemptr(m, entry-1);
+
+    /* if menu entry is non-indirected text, copy upto 12 chars from supplied string */
+    if((p->iconflags & wimp_ITEXT) && !(p->iconflags & wimp_INDIRECT))
+    {
+        length = min(12, strlen(text));
+
+        for(i = 0; i < length; i++)
+            p->data.text[i] = text[i];
+        if(length < 12)
+            p->data.text[length] = CH_NULL;
+    }
+}
+
+#endif /* SKS_ACW */
+
 /* end */
