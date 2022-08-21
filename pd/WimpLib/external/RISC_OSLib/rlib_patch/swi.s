--- _src	2011-10-28 15:23:42 +0100
+++ _dst	2013-08-31 16:54:22 +0100
@@ -47,9 +47,16 @@
         GBLL    ModeMayBeNonUser
 ModeMayBeNonUser   SETL  {FALSE}
 
+ [ {TRUE}
+        GET     as_flags_h
+
+        GET     as_regs_h
+        GET     as_macro_h
+ |
         GET     Hdr:ListOpts
         GET     Hdr:APCS.Common
         GET     Hdr:Macros
+ ]
 
 XOS_MASK      * &00020000 ; mask to make a swi a RISC OS V-error SWI
 
@@ -108,18 +115,18 @@
 
         [ :LNOT:UROM
 bbc_get SWI     XOS_ReadC
-        Return  ,LinkNotStacked,VS
+        FunctionReturn  ,LinkNotStacked,VS
         ORRCS   a1, a1, #&100           ; SKS
-        Return  ,LinkNotStacked
+        FunctionReturn  ,LinkNotStacked
         ]
 
 bbc_vduw
         SWI     XOS_WriteC
-        Return  ,LinkNotStacked,VS
+        FunctionReturn  ,LinkNotStacked,VS
         MOV     a1, a1, LSR #8
 bbc_vdu SWI     XOS_WriteC
         MOVVC   a1, #0
-        Return  ,LinkNotStacked
+        FunctionReturn  ,LinkNotStacked
 
 
 ; void os_swi(int swicode, os_regset* /*inout*/);
@@ -136,10 +143,10 @@
         SWI     XOS_CallASWIR12
         LDR     ip, [sp], #4
         STMIA   ip, {r0-r9}
-        Return  "v1-v6"
+        FunctionReturn  "v1-v6"
 os_swi_noregset
         SWI     XOS_CallASWIR12
-        Return  "v1-v6"
+        FunctionReturn  "v1-v6"
 
 
 ; os_error *os_swix(int swicode, os_regset* /*inout*/);
@@ -158,11 +165,11 @@
         LDR     ip, [sp], #4
         STMIA   ip, {r0-r9}
         MOVVC   a1, #0
-        Return  "v1-v6"
+        FunctionReturn  "v1-v6"
 os_swix_noregset
         SWI     XOS_CallASWIR12
         MOVVC   a1, #0
-        Return  "v1-v6"
+        FunctionReturn  "v1-v6"
 
 os_swix0
 os_swix1
@@ -189,7 +196,7 @@
         LDMIA   lr, {a4, v1, v2}
         SWI     XOS_CallASWIR12
         MOVVC   a1, #0
-        Return  "v1-v6"
+        FunctionReturn  "v1-v6"
 
 swi_ret_inst
         MOV     pc, ip
@@ -202,11 +209,11 @@
         MOV     r0, a2
         SWI     XOS_CallASWIR12
         LDR     ip, [sp]
-        Return  "a2,v1-v6",,VS
+        FunctionReturn  "a2,v1-v6",,VS
         TEQ     ip, #0
         STRNE   a1, [ip]
         MOV     a1, #0
-        Return  "a2,v1-v6"
+        FunctionReturn  "a2,v1-v6"
 
 os_swix2r
         ORR     a1, a1, #&20000
@@ -217,14 +224,14 @@
         MOV     a2, a3
         SWI     XOS_CallASWIR12
         LDR     ip, [sp]
-        Return  "a2,v1-v6",,VS
+        FunctionReturn  "a2,v1-v6",,VS
         TEQ     ip, #0
         STRNE   a1, [ip]
         LDR     ip, [sp, #8 * 4]
         TEQ     ip, #0
         STRNE   a2, [ip]
         MOV     a1, #0
-        Return  "a2,v1-v6"
+        FunctionReturn  "a2,v1-v6"
 
 os_swix3r
         ORR     a1, a1, #&20000
@@ -237,7 +244,7 @@
         SWI     XOS_CallASWIR12
         ADD     ip, sp, #7 * 4
         LDMIA   ip, {v1, v2, v3}
-        Return  "v1-v6",,VS
+        FunctionReturn  "v1-v6",,VS
         TEQ     v1, #0
         STRNE   a1, [v1]
         TEQ     v2, #0
@@ -245,7 +252,7 @@
         TEQ     v3, #0
         STRNE   a3, [v3]
         MOV     a1, #0
-        Return  "v1-v6"
+        FunctionReturn  "v1-v6"
 
 os_swix4r
         ORR     a1, a1, #&20000
@@ -259,7 +266,7 @@
         SWI     XOS_CallASWIR12
         ADD     ip, sp, #8 * 4
         LDMIA   ip, {v1-v4}
-        Return  "v1-v6",,VS
+        FunctionReturn  "v1-v6",,VS
         TEQ     v1, #0
         STRNE   a1, [v1]
         TEQ     v2, #0
@@ -269,7 +276,7 @@
         TEQ     v4, #0
         STRNE   a4, [v4]
         MOV     a1, #0
-        Return  "v1-v6"
+        FunctionReturn  "v1-v6"
 
 os_swix5r
         ORR     a1, a1, #&20000
@@ -284,7 +291,7 @@
         SWI     XOS_CallASWIR12
         ADD     ip, sp, #9 * 4
         LDMIA   ip, {v3-v6, ip}
-        Return  "v1-v6",,VS
+        FunctionReturn  "v1-v6",,VS
         TEQ     v3, #0
         STRNE   a1, [v3]
         TEQ     v4, #0
@@ -296,7 +303,7 @@
         TEQ     ip, #0
         STRNE   v1, [ip]
         MOV     a1, #0
-        Return  "v1-v6"
+        FunctionReturn  "v1-v6"
 
 os_swix6r
         ORR     a1, a1, #&20000
@@ -311,7 +318,7 @@
         SWI     XOS_CallASWIR12
         ADD     ip, sp, #10 * 4
         LDMIA   ip, {v3-v6, ip, lr}     ; APCS-R assumption here
-        Return  "v1-v6",,VS
+        FunctionReturn  "v1-v6",,VS
         TEQ     v3, #0
         STRNE   a1, [v3]
         TEQ     v4, #0
@@ -325,7 +332,7 @@
         TEQ     lr, #0
         STRNE   v2, [lr]
         MOV     a1, #0
-        Return  "v1-v6"
+        FunctionReturn  "v1-v6"
 
 os_byte
         FunctionEntry
@@ -337,13 +344,13 @@
         STR     r1, [r3]
         STR     r2, [ip]
         MOVVC   r0, #0
-        Return
+        FunctionReturn
 
 os_word
         MOV     ip, lr
         SWI     XOS_Word
         MOVVC   r0, #0
-        Return  ,LinkNotStacked,,ip
+        FunctionReturn  ,LinkNotStacked,,ip
 
 os_read_var_val
         FunctionEntry "r4"
@@ -353,6 +360,6 @@
         MOV     r0, #0
         STRVSB  r0, [r1]
         STRVCB  r0, [r1, r2]
-        Return  "r4"
+        FunctionReturn  "r4"
 
         END
