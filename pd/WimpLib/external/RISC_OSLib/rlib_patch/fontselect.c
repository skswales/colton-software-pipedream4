--- _src	2009-05-31 18:58:59 +0100
+++ _dst	2013-09-15 16:40:52 +0100
@@ -35,6 +35,7 @@
  * History: IDJ: 06-Feb-92: prepared for source release
  */
 
+#include "include.h" /* SKS_ACW */
 
 #ifndef UROM
 
@@ -66,6 +67,7 @@
 #define typeface_icon    4
 #define weight_icon      7
 #define style_icon       10
+#ifndef SKS_ACW
 #define height_up_icon   15
 #define width_up_icon    16
 #define height_icon      17
@@ -74,6 +76,17 @@
 #define width_down_icon  20
 #define font_text_icon   21
 #define try_icon         22
+#else /* SKS_ACW */
+/* icons now rearranged in template */
+#define height_up_icon   13
+#define height_down_icon 14
+#define height_icon      15
+#define width_up_icon    16
+#define width_down_icon  17
+#define width_icon       18
+#define try_icon         35
+#define font_text_icon   36
+#endif /* SKS_ACW */
 
 #define typeface_level          0
 #define weight_level            1
@@ -93,6 +106,9 @@
 typedef struct pane_block
 {
     int         level;
+#ifdef SKS_ACW
+    wimp_wind  *core; /* from template_copy() */
+#endif /* SKS_ACW */
     wimp_w      handle;
     wimp_i      selection;
     int         no_icons;
@@ -103,6 +119,10 @@
 typedef struct global_data
 {
     font       font_handle;
+#ifdef SKS_ACW
+    wimp_wind *font_window_core; /* from template_copy() */
+    fontselect_fn unknown_icon_routine;
+#endif /* SKS_ACW */
     wimp_w     font_window_handle;
     pane_block *(panes_store[3]);
     double     start_width;
@@ -185,11 +205,27 @@
    wimp_icreate    icon_create;
    os_error        *error;
 
+#ifndef SKS_ACW
    if ( (window = template_syshandle(name))==0 )
        return NULL;
+#else /* SKS_ACW */
+   if(pane->core == 0)
+   {
+       template * subwindow_template;
+
+       if((subwindow_template = template_find_new(name)) == 0)
+           return NULL;
+
+       if((pane->core = template_copy_new(subwindow_template)) == 0)
+           return(NULL);
+   }
+
+   window = pane->core;
+#endif /* SKS_ACW */
+
    window -> nicons = 2;
    if ( (error=wimp_create_wind( window, &(pane->handle) )) !=NULL )
-       return error;
+       goto dealloc; /* SKS dealloc then return error */
 
    icon_create.w = pane->handle;
    wimpt_noerr( wimp_get_icon_info( pane->handle, 0 , &icon_create.i) );
@@ -213,9 +249,18 @@
       y0 += dy; y1 += dy;
       icon_create.i.data.indirecttext.buffer = pane->icons[i];
       if ( (error=wimp_create_icon( &icon_create, &j )) !=NULL )
-          return error;
+          goto dealloc; /* SKS dealloc then return error */
    }
-   return ( reopen_subwindow( -1, pane->handle, x_offset, y_offset, icon) );
+   error = reopen_subwindow( (wimp_w) -1, pane->handle, x_offset, y_offset, icon);
+
+   if(!error)
+       return(error);
+
+   /* window deletion done by caller calling fontselect_closewindows() */
+
+dealloc:;
+   template_copy_dispose(&pane->core);
+   return(error);
 }
 
 /* ------------------------------------------------------------- */
@@ -251,14 +296,34 @@
    wimp_wind       *window_block_ptr;
    wimp_wstate     window_state;
 
-
    /* Create the dialogue box */
+#ifndef SKS_ACW
    if ( (window_block_ptr=template_syshandle("FontSelect")) == NULL)
    {
        werr(0, "Failed to find FontSelect template.");
        return FALSE;
    }
    else
+#else /* SKS_ACW */
+   if(globals->font_window_core == 0)
+       {
+       template * main_template;
+
+       if((main_template = template_find_new("FontSelect")) == 0)
+           {
+           werr(0, msgs_lookup("FNTSELM00" /*":Failed to find FontSelect template."*/));
+           return(FALSE);
+           }
+
+       if((globals->font_window_core = template_copy_new(main_template)) == 0)
+           {
+           werr(0, msgs_lookup("FNTSELM01" /*":Memory full"*/));
+           return(FALSE);
+           }
+       }
+
+   window_block_ptr = globals->font_window_core;
+#endif /* SKS_ACW */
    {
 
       /* Show the main window */
@@ -294,7 +359,12 @@
 
 
       /* Attach handlers */
+#ifdef SKS_ACW
+      globals->unknown_icon_routine = unknown_icon_routine;
+      win_register_event_handler( globals->font_window_handle, window_process, NULL );
+#else /* SKS_ACW */
       win_register_event_handler( globals->font_window_handle,                 window_process,    (void *) unknown_icon_routine );
+#endif /* SKS_ACW */
       win_register_event_handler( (globals->panes_store)[typeface_level]->handle, subwindow_process, (void *) (globals->panes_store)[typeface_level] );
       win_register_event_handler( (globals->panes_store)[weight_level]->handle,   subwindow_process, (void *) (globals->panes_store)[weight_level] );
       win_register_event_handler( (globals->panes_store)[style_level]->handle,    subwindow_process, (void *) (globals->panes_store)[style_level] );
@@ -557,30 +627,42 @@
    {
       if ((globals->panes_store)[typeface_level]->handle != -1)
       {
+#ifndef SKS_ACW
          wimpt_noerr( wimp_close_wind(  (globals->panes_store)[typeface_level]->handle ) );
+#endif
          wimpt_noerr( wimp_delete_wind( (globals->panes_store)[typeface_level]->handle ) );
          win_register_event_handler((globals->panes_store)[typeface_level]->handle, 0, 0);
          (globals->panes_store)[typeface_level]->handle = -1;
       }
       if ((globals->panes_store)[style_level]->handle != -1)
       {
+#ifndef SKS_ACW
          wimpt_noerr( wimp_close_wind(  (globals->panes_store)[style_level]->handle ) );
+#endif
          wimpt_noerr( wimp_delete_wind( (globals->panes_store)[style_level]->handle ) );
          win_register_event_handler((globals->panes_store)[style_level]->handle, 0, 0);
          (globals->panes_store)[style_level]->handle = -1;
       }
       if ((globals->panes_store)[weight_level]->handle != -1)
       {
+#ifndef SKS_ACW
          wimpt_noerr( wimp_close_wind(  (globals->panes_store)[weight_level]->handle ) );
+#endif
          wimpt_noerr( wimp_delete_wind( (globals->panes_store)[weight_level]->handle ) );
          win_register_event_handler((globals->panes_store)[weight_level]->handle, 0, 0);
          (globals->panes_store)[weight_level]->handle = -1;
       }
       if ( globals->font_window_handle != -1 )
       {
+#ifndef SKS_ACW
          wimpt_noerr( wimp_close_wind(  globals->font_window_handle ) );
          wimpt_noerr( wimp_delete_wind( globals->font_window_handle ) );
          win_register_event_handler(globals->font_window_handle, 0, 0);
+#else /* SKS_ACW */
+         win_register_event_handler(globals->font_window_handle, 0, 0);
+         /*wimpt_noerr( wimp_close_wind(  globals->font_window_handle ) );*/
+         wimpt_noerr( wimp_delete_wind( globals->font_window_handle ) );
+#endif /* SKS_ACW */
          globals->font_window_handle = -1;
       }
       if (globals->font_handle!=-1)
@@ -588,6 +670,9 @@
          font_lose(globals->font_handle);
          globals->font_handle = -1;
       }
+#ifdef SKS_ACW
+      template_copy_dispose(&globals->font_window_core);
+#endif /* SKS_ACW */
    }
 }
 
@@ -662,7 +747,11 @@
    e->data.msg.hdr.your_ref = e->data.msg.hdr.my_ref;
    e->data.msg.hdr.action = wimp_MHELPREPLY;
    e->data.msg.hdr.size = 256;
+#ifdef SKS_ACW
+   safe_strkpy( e->data.msg.data.helpreply.text, elemof32(e->data.msg.data.helpreply.text), text );
+#else /* SKS_ACW */
    strcpy( e->data.msg.data.helpreply.text, text );
+#endif /* SKS_ACW */
    wimp_sendmessage( wimp_ESEND, &(e->data.msg), e->data.msg.hdr.task );
 }
 
@@ -684,7 +773,11 @@
    char            font_name[40];
    wimp_icon       icon_block;
    wimp_wstate     pane_posn;
+#ifndef SKS_ACW
    fontselect_fn   unknown_icon_routine = (fontselect_fn) handle;
+#else /* SKS_ACW */
+   fontselect_fn   unknown_icon_routine = globals->unknown_icon_routine;
+#endif /* SKS_ACW */
 
    handle = handle;
 
@@ -757,7 +850,9 @@
                    }
                    break;
                default:
-                   if ((*unknown_icon_routine)( font_name, globals->start_width, globals->start_height, e ))
+                   read_font_name( font_name );
+
+                   if ((*unknown_icon_routine)( font_name, read_float(width_icon), read_float(height_icon), e ))
                        fontselect_closewindows();
                    break;
            }
@@ -773,6 +868,10 @@
                i = read_int( e->data.but.m.i );
                set_int( width_icon,  i );
                set_int( height_icon, i );
+
+               read_font_name( font_name );
+               if ((*unknown_icon_routine)( font_name, /*implicit cast to double*/ i, /*ditto*/ i, e ))
+                   fontselect_closewindows();
            }
            else
            {
@@ -799,28 +898,33 @@
                                add_to_width( -1.0 );
                            else
                                add_to_width( 1.0 );
-                           break;
+                           goto tell_client; /* SKS must tell client */
+
                        case width_down_icon:
                            if (e->data.but.m.bbits & wimp_BRIGHT)
                                add_to_width( 1.0 );
                            else
                                add_to_width( -1.0 );
-                           break;
+                           goto tell_client; /* SKS must tell client */
+
                        case height_up_icon:
                            if (e->data.but.m.bbits & wimp_BRIGHT)
                                add_to_height( -1.0 );
                            else
                                add_to_height( 1.0 );
-                           break;
+                           goto tell_client; /* SKS must tell client */
+
                        case height_down_icon:
                            if (e->data.but.m.bbits & wimp_BRIGHT)
                                add_to_height( 1.0 );
                            else
                                add_to_height( -1.0 );
-                           break;
+                           goto tell_client; /* SKS must tell client */
+
                        default:
+                       tell_client:
                            read_font_name( font_name );
-                           if ((*unknown_icon_routine)( font_name, read_int(width_icon), read_int(height_icon), e ))
+                           if ((*unknown_icon_routine)( font_name, read_float(width_icon), read_float(height_icon), e )) /* SKS keep as float */
                                fontselect_closewindows();
                            break;
                    }
@@ -832,6 +936,7 @@
            switch (e->data.msg.hdr.action)
            {
                case wimp_MHELPREQUEST:
+#ifndef SKS_ACW
                    if (e->data.msg.data.helprequest.m.i <= try_icon)
                    {
                        sprintf(buf,"FNTSELI%02d:\x01",e->data.msg.data.helprequest.m.i);
@@ -856,12 +961,34 @@
                                fontselect_closewindows();
                        }
                    }
+#else /* SKS_ACW */
+                   if(e->data.msg.data.helprequest.m.i == -1)
+                       chr_ptr = msgs_lookup("FNTSELIZZ");
+                   else
+                       {
+                       wimp_get_icon_info( globals->font_window_handle, e->data.msg.data.helprequest.m.i, &icon_block );
+                       if ( (icon_block.flags & (wimp_IESG*0x1f) ) == (wimp_IESG * 0x3) )
+                           {
+                           i = read_int( e->data.msg.data.helprequest.m.i );
+                           sprintf(buf,msgs_lookup("FNTSELPNT"),i);
+                           chr_ptr = buf;
+                           }
+                       else
+                           {
+                           sprintf(buf,"FNTSELI%02d",e->data.msg.data.helprequest.m.i);
+                           chr_ptr = msgs_lookup(buf);
+                           if(chr_ptr == buf)
+                               chr_ptr = msgs_lookup("FNTSELIZZ");
+                           }
+                       }
+                   help_reply(e, chr_ptr);
+#endif /* SKS_ACW */
                    break;
            }
            break;
        default:
            read_font_name( font_name );
-           if ((*unknown_icon_routine)( font_name, read_int(width_icon), read_int(height_icon), e ))
+           if ((*unknown_icon_routine)( font_name, read_float(width_icon), read_float(height_icon), e ))
                fontselect_closewindows();
            break;
    }
@@ -898,6 +1025,10 @@
           switch (e->data.msg.hdr.action)
           {
               case wimp_MHELPREQUEST:
+#ifdef SKS_ACW
+                  if(e->data.msg.data.helprequest.m.i == -1) /* SKS check */
+                      break;
+#endif /* SKS_ACW */
                   wimp_get_icon_info( e->data.msg.data.helprequest.m.w, e->data.msg.data.helprequest.m.i, &icon_block );
                   sprintf(buf, "FNTSELW%d", pane_handle->level );
                   sprintf(buf2, msgs_lookup(buf), icon_block.data.indirecttext.buffer );
@@ -1060,12 +1191,17 @@
 
    /* List the fonts. If we get an error then return */
    if (fontlist_list_all_fonts(TRUE) == NULL)
+       {
+#ifdef SKS_ACW
+       werr(0, msgs_lookup("FNTSELM03")); /* SKS need to discriminate errors */
+#endif /* SKS_ACW */
        return FALSE;
+       }
 
    /* Grab global workspace */
-   if ( ( globals = (global_data *)malloc( 3*sizeof(global_data) ) ) == NULL )
+   if ( ( globals = (global_data *)calloc( /*3**/sizeof(global_data), 1 ) ) == NULL )
    {
-       werr(0, "Out of memory for allocating global data for font selector" );
+       werr(0, msgs_lookup("FNTSELM01" /*":Out of memory for allocating global data for font selector"*/));
        return FALSE;
    }
 
@@ -1075,9 +1211,9 @@
 
    for  (i=typeface_level; i<=style_level; i++)
    {
-       if ( ( (globals->panes_store)[i] = (pane_block *)malloc( sizeof(pane_block) + no_icons[i]*max_font_length ) ) == NULL )
+       if ( ( (globals->panes_store)[i] = (pane_block *)calloc( sizeof(pane_block) + no_icons[i]*max_font_length , 1) ) == NULL )
        {
-           werr(0, "Out of memory for allocating panes" );
+           werr(0, msgs_lookup("FNTSELM02" /*":Out of memory for allocating panes"*/));
            return FALSE;
        }
        (globals->panes_store)[i]->handle = -1;
@@ -1103,7 +1239,7 @@
    /* Check they have already called font_selector_init */
    if (globals == NULL)
    {
-       werr(TRUE, "FontSel01:Must call font_selector_init before font_selector");
+       werr(TRUE, msgs_lookup("FontSel01" /*":Must call fontselector_init before font_selector"*/));
        return FALSE;
    }
 
