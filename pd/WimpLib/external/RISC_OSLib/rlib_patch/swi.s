--- _src	2019-07-29 19:20:28.000000000 +0100
+++ _dst	2019-07-30 11:45:39.440000000 +0100
@@ -44,9 +44,16 @@
         GBLL    ModeMayBeNonUser
 ModeMayBeNonUser   SETL  {FALSE}
 
+ [ {TRUE}
+        GET     as_flags_h
+
+        ;GET     as_regs_h
+        ;GET     as_macro_h
+ ]
         GET     Hdr:ListOpts
         GET     Hdr:APCS.Common
         GET     Hdr:Macros
+ ; ]
 
 XOS_MASK      * &00020000 ; mask to make a swi a RISC OS V-error SWI
