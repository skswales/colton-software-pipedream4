--- _src	2022-10-26 16:47:19.560000000 +0000
+++ _dst	2022-10-27 18:58:35.600000000 +0000
@@ -44,6 +44,9 @@
         GBLL    ModeMayBeNonUser
 ModeMayBeNonUser   SETL  {FALSE}
 
+ [ {TRUE}
+        GET     as_flags_h
+ ]
         GET     Hdr:ListOpts
         GET     Hdr:APCS.Common
         GET     Hdr:Macros
@@ -66,37 +69,37 @@
         EXPORT  |bbc_get|
         ]
         EXPORT  |bbc_vdu|
-        EXPORT  |bbc_vduw|
-        EXPORT  |os_swi|
+;;; SKS_ACW     EXPORT  |bbc_vduw|
+;;; SKS_ACW     EXPORT  |os_swi|
         EXPORT  |os_swix|
-        EXPORT  |os_swi0|
+;;; SKS_ACW     EXPORT  |os_swi0|
         EXPORT  |os_swi1|
-        EXPORT  |os_swi2|
-        EXPORT  |os_swi3|
-        EXPORT  |os_swi4|
-        EXPORT  |os_swi5|
-        EXPORT  |os_swi6|
-        EXPORT  |os_swi1r|
-        EXPORT  |os_swi2r|
-        EXPORT  |os_swi3r|
-        EXPORT  |os_swi4r|
-        EXPORT  |os_swi5r|
-        EXPORT  |os_swi6r|
+;;; SKS_ACW     EXPORT  |os_swi2|
+;;; SKS_ACW     EXPORT  |os_swi3|
+;;; SKS_ACW     EXPORT  |os_swi4|
+;;; SKS_ACW     EXPORT  |os_swi5|
+;;; SKS_ACW     EXPORT  |os_swi6|
+;;; SKS_ACW     EXPORT  |os_swi1r|
+;;; SKS_ACW     EXPORT  |os_swi2r|
+;;; SKS_ACW     EXPORT  |os_swi3r|
+;;; SKS_ACW     EXPORT  |os_swi4r|
+;;; SKS_ACW     EXPORT  |os_swi5r|
+;;; SKS_ACW     EXPORT  |os_swi6r|
         EXPORT  |os_swix0|
         EXPORT  |os_swix1|
         EXPORT  |os_swix2|
-        EXPORT  |os_swix3|
-        EXPORT  |os_swix4|
-        EXPORT  |os_swix5|
-        EXPORT  |os_swix6|
-        EXPORT  |os_swix1r|
-        EXPORT  |os_swix2r|
-        EXPORT  |os_swix3r|
-        EXPORT  |os_swix4r|
-        EXPORT  |os_swix5r|
-        EXPORT  |os_swix6r|
+;;; SKS_ACW     EXPORT  |os_swix3|
+;;; SKS_ACW     EXPORT  |os_swix4|
+;;; SKS_ACW     EXPORT  |os_swix5|
+;;; SKS_ACW     EXPORT  |os_swix6|
+;;; SKS_ACW     EXPORT  |os_swix1r|
+;;; SKS_ACW     EXPORT  |os_swix2r|
+;;; SKS_ACW     EXPORT  |os_swix3r|
+;;; SKS_ACW     EXPORT  |os_swix4r|
+;;; SKS_ACW     EXPORT  |os_swix5r|
+;;; SKS_ACW     EXPORT  |os_swix6r|
         EXPORT  |os_byte|
-        EXPORT  |os_word|
+;;; SKS_ACW     EXPORT  |os_word|
         EXPORT  |os_read_var_val|
 
         AREA    |C$$code|, CODE, READONLY
@@ -104,21 +107,23 @@
 |v$codesegment|
 
         [ :LNOT:UROM
-bbc_get SWI     XOS_ReadC
+bbc_get SVC     #XOS_ReadC
         Return  ,LinkNotStacked,VS
         ORRCS   a1, a1, #&100           ; SKS
         Return  ,LinkNotStacked
         ]
 
+ [ {FALSE} ;;; SKS_ACW
 bbc_vduw
-        SWI     XOS_WriteC
+        SVC     #XOS_WriteC
         Return  ,LinkNotStacked,VS
         MOV     a1, a1, LSR #8
-bbc_vdu SWI     XOS_WriteC
+ ] ;;; SKS_ACW
+bbc_vdu SVC     #XOS_WriteC
         MOVVC   a1, #0
         Return  ,LinkNotStacked
 
-
+ [ {FALSE} ;;; SKS_ACW
 ; void os_swi(int swicode, os_regset* /*inout*/);
 
 ; In    a1 contains swi number, a2 points to ARM register structure
@@ -130,14 +135,14 @@
         BEQ     os_swi_noregset
         STR     r1, [sp, #-4]!
         LDMIA   r1, {r0-r9}
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         LDR     ip, [sp], #4
         STMIA   ip, {r0-r9}
         Return  "v1-v6"
 os_swi_noregset
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         Return  "v1-v6"
-
+ ] ;;; SKS_ACW
 
 ; os_error *os_swix(int swicode, os_regset* /*inout*/);
 
@@ -145,38 +150,37 @@
 
 os_swix
         STMDB   sp!, {v1-v6, lr}
-        ORR     a1, a1, #XOS_MASK       ; make a SWI of V-error type
-        MOV     r12, r0
+        ORR     r12, a1, #XOS_MASK      ; make a SWI of V-error type (SKS_ACW shortened)
         CMP     r1, #0
         BEQ     os_swix_noregset
         STR     r1, [sp, #-4]!
         LDMIA   r1, {r0-r9}
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         LDR     ip, [sp], #4
         STMIA   ip, {r0-r9}
         MOVVC   a1, #0
         Return  "v1-v6"
 os_swix_noregset
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         MOVVC   a1, #0
         Return  "v1-v6"
 
 os_swix0
 os_swix1
 os_swix2
-os_swix3
-os_swix4
-os_swix5
-os_swix6
-os_swix7
+;;; SKS_ACW os_swix3
+;;; SKS_ACW os_swix4
+;;; SKS_ACW os_swix5
+;;; SKS_ACW os_swix6
+;;; SKS_ACW os_swix7
         ORR     a1, a1, #&20000
-os_swi0
+;;; SKS_ACW os_swi0
 os_swi1
-os_swi2
-os_swi3
-os_swi4
-os_swi5
-os_swi6
+;;; SKS_ACW os_swi2
+;;; SKS_ACW os_swi3
+;;; SKS_ACW os_swi4
+;;; SKS_ACW os_swi5
+;;; SKS_ACW os_swi6
         STMDB   sp!, {v1-v6, lr}
         MOV     r12, r0
         MOV     a1, a2
@@ -184,46 +188,49 @@
         MOV     a3, a4
         ADD     lr, sp, #7*4
         LDMIA   lr, {a4, v1, v2}
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         MOVVC   a1, #0
         Return  "v1-v6"
 
 swi_ret_inst
         MOV     pc, ip
 
-os_swix1r
+ [ {FALSE} ;;; SKS_ACW
+os_swix1r ROUT
         ORR     a1, a1, #&20000
 os_swi1r
         STMDB   sp!, {a3, v1-v6, lr}
         MOV     r12, r0
         MOV     r0, a2
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         LDR     ip, [sp]
-        Return  "a2,v1-v6",,VS
+        BVS     %FT99
         TEQ     ip, #0
         STRNE   a1, [ip]
         MOV     a1, #0
+99
         Return  "a2,v1-v6"
 
-os_swix2r
+os_swix2r ROUT
         ORR     a1, a1, #&20000
 os_swi2r
         STMDB   sp!, {a4, v1-v6, lr}
         MOV     r12, r0
         MOV     a1, a2
         MOV     a2, a3
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         LDR     ip, [sp]
-        Return  "a2,v1-v6",,VS
+        BVS     %FT99
         TEQ     ip, #0
         STRNE   a1, [ip]
         LDR     ip, [sp, #8 * 4]
         TEQ     ip, #0
         STRNE   a2, [ip]
         MOV     a1, #0
+99
         Return  "a2,v1-v6"
 
-os_swix3r
+os_swix3r ROUT
         ORR     a1, a1, #&20000
 os_swi3r
         STMDB   sp!, {v1-v6, lr}
@@ -231,10 +238,10 @@
         MOV     a1, a2
         MOV     a2, a3
         MOV     a3, a4
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         ADD     ip, sp, #7 * 4
         LDMIA   ip, {v1, v2, v3}
-        Return  "v1-v6",,VS
+        BVS     %FT99
         TEQ     v1, #0
         STRNE   a1, [v1]
         TEQ     v2, #0
@@ -242,9 +249,10 @@
         TEQ     v3, #0
         STRNE   a3, [v3]
         MOV     a1, #0
+99
         Return  "v1-v6"
 
-os_swix4r
+os_swix4r ROUT
         ORR     a1, a1, #&20000
 os_swi4r
         STMDB   sp!, {v1-v6, lr}
@@ -253,10 +261,10 @@
         MOV     a2, a3
         MOV     a3, a4
         LDR     a4, [sp, #7 * 4]
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         ADD     ip, sp, #8 * 4
         LDMIA   ip, {v1-v4}
-        Return  "v1-v6",,VS
+        BVS     %FT99
         TEQ     v1, #0
         STRNE   a1, [v1]
         TEQ     v2, #0
@@ -266,9 +274,10 @@
         TEQ     v4, #0
         STRNE   a4, [v4]
         MOV     a1, #0
+99
         Return  "v1-v6"
 
-os_swix5r
+os_swix5r ROUT
         ORR     a1, a1, #&20000
 os_swi5r
         STMDB   sp!, {v1-v6, lr}
@@ -278,10 +287,10 @@
         MOV     a3, a4
         ADD     lr, sp, #7 * 4
         LDMIA   lr, {a4, v1}
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         ADD     ip, sp, #9 * 4
         LDMIA   ip, {v3-v6, ip}
-        Return  "v1-v6",,VS
+        BVS     %FT99
         TEQ     v3, #0
         STRNE   a1, [v3]
         TEQ     v4, #0
@@ -293,9 +302,10 @@
         TEQ     ip, #0
         STRNE   v1, [ip]
         MOV     a1, #0
+99
         Return  "v1-v6"
 
-os_swix6r
+os_swix6r ROUT
         ORR     a1, a1, #&20000
 os_swi6r
         STMDB   sp!, {v1-v6, lr}
@@ -305,10 +315,10 @@
         MOV     a3, a4
         ADD     lr, sp, #7 * 4
         LDMIA   lr, {a4, v1, v2}
-        SWI     XOS_CallASWIR12
+        SVC     #XOS_CallASWIR12
         ADD     ip, sp, #10 * 4
         LDMIA   ip, {v3-v6, ip, lr}     ; APCS-R assumption here
-        Return  "v1-v6",,VS
+        BVS     %FT99
         TEQ     v3, #0
         STRNE   a1, [v3]
         TEQ     v4, #0
@@ -322,7 +332,9 @@
         TEQ     lr, #0
         STRNE   v2, [lr]
         MOV     a1, #0
+99
         Return  "v1-v6"
+ ] ;;; SKS_ACW
 
 os_byte
         FunctionEntry
@@ -330,26 +342,28 @@
         MOV     ip, r2
         LDR     r1, [r1]
         LDR     r2, [r2]
-        SWI     XOS_Byte
+        SVC     #XOS_Byte
         STR     r1, [r3]
         STR     r2, [ip]
         MOVVC   r0, #0
         Return
 
+ [ {FALSE} ;;; SKS_ACW
 os_word
         MOV     ip, lr
-        SWI     XOS_Word
+        SVC     #XOS_Word
         MOVVC   r0, #0
         Return  ,LinkNotStacked,,ip
+ ] ;;; SKS_ACW
 
 os_read_var_val
         FunctionEntry "r4"
         MOV     r3, #0
         MOV     r4, #3
-        SWI     XOS_ReadVarVal
+        SVC     #XOS_ReadVarVal
         MOV     r0, #0
-        STRVSB  r0, [r1]
-        STRVCB  r0, [r1, r2]
+        STRBVS  r0, [r1]
+        STRBVC  r0, [r1, r2]
         Return  "r4"
 
         END
