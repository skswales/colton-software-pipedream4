--- _src	2001-08-23 14:47:04.000000000 +0100
+++ _dst	2016-09-16 14:50:26.550000000 +0100
@@ -36,6 +36,7 @@
  *
  */
 
+#include "include.h" /* for SKS_ACW */
 
 #include <stdlib.h>
 #include <stdio.h>
@@ -61,6 +62,10 @@
 
 #ifndef UROM
 
+extern int
+_stricmp(const char * a, const char * b);
+
+
 /* -------------------------------------------------- */
 /* Globals defined */
 /* -------------------------------------------------- */
@@ -74,10 +79,10 @@
 
 /* Create a tree of all the available fonts in font__tree, returning NULL if an error occurred.
    system is TRUE if the system font is to appear in the tree */
-extern fontlist_node *font__list_all_fonts( BOOL system );
+extern fontlist_node *fontlist_list_all_fonts( BOOL system );                                         
 
 /* Free the font tree structure */
-extern void font__free_font_tree( fontlist_node *font_tree );
+extern void fontlist_free_font_tree( fontlist_node * font_tree );
 
 /* Add a new font to the font tree strucutre */
 static fontlist_node *font__add_new_font( fontlist_node **font_list_ptr, char *font_name );
@@ -107,6 +112,10 @@
    int    font_count = 0;
    char   font[40];
 
+#ifdef SKS_ACW
+   fontlist_free_font_tree(font__tree);
+#endif /* SKS_ACW */
+
    if (system)
            font__add_new_font( &font__tree, "System" );
 
@@ -117,7 +126,9 @@
       if (font_count != -1)
          if ( font__add_new_font( &font__tree, font ) == NULL )
          {
+#ifndef SKS_ACW
             werr(0, "No room for font list.\n");
+#endif /* SKS_ACW */
             return NULL;
          }
    }
@@ -132,6 +143,11 @@
 extern void fontlist_free_font_tree( fontlist_node *font_tree )
 {
    font__deallocate_font( font_tree );
+
+#ifdef SKS_ACW
+   /* SKS after PD 4.11 07jan92 - must clear out the global tree assuming it's this one! */
+   font__tree = NULL;
+#endif /* SKS_ACW */
 }
 
 
@@ -143,6 +159,11 @@
 {
    fontlist_node *font_ptr;
 
+#ifdef SKS_ACW /* SKS 25oct96 remove stupid font from all lists */
+   if(0 == _stricmp(font_name, "WIMPSymbol"))
+       return((void *) 1);
+#endif /* SKS_ACW */
+
    /* Find/create font with the first portion of the font name */
    font_ptr = font__seek_font( font_list_ptr, &font_name, FALSE );
 
@@ -199,7 +220,7 @@
       return *font_list_ptr;
    }
 
-   for (i=0; ((*font_name_ptr)[i] != '\0' && (rest_of_name || (*font_name_ptr)[i] != '.')) ;i++);
+   for (i=0; (((*font_name_ptr)[i] != CH_NULL) && (rest_of_name || (*font_name_ptr)[i] != '.')) ;i++) /*EMPTY*/;
    if ( ( strncmp( *font_name_ptr, (*font_list_ptr)->name, i) != 0 ) ||
         ( (*font_list_ptr)->name[i]!='\0' ) )
            return font__seek_font( &((*font_list_ptr)->brother), font_name_ptr, rest_of_name );
