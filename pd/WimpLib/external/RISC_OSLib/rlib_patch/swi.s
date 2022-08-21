--- _src	2021-12-31 14:59:31.180000000 +0000
+++ _dst	2022-08-18 16:55:47.140000000 +0000
@@ -44,6 +44,9 @@
         GBLL    ModeMayBeNonUser
 ModeMayBeNonUser   SETL  {FALSE}
 
+ [ {TRUE}
+        GET     as_flags_h
+ ]
         GET     Hdr:ListOpts
         GET     Hdr:APCS.Common
         GET     Hdr:Macros
@@ -104,17 +107,17 @@
 |v$codesegment|
 
         [ :LNOT:UROM
-bbc_get SWI     XOS_ReadC
+bbc_get SVC     #XOS_ReadC
         Return  ,LinkNotStacked,VS
         ORRCS   a1, a1, #&100           ; SKS
         Return  ,LinkNotStacked
         ]
 
 bbc_vduw
-        SWI     XOS_WriteC
+        SVC     #XOS_WriteC
         Return  ,LinkNotStacked,VS
         MOV     a1, a1, LSR #8
-bbc_vdu SWI     XOS_WriteC
+bbc_vdu SVC     #XOS_WriteC
         MOVVC   a1, #0
         Return  ,LinkNotStacked
 
@@ -130,12 +133,12 @@
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
 
 
@@ -151,13 +154,13 @@
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
 
@@ -184,46 +187,48 @@
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
@@ -231,10 +236,10 @@
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
@@ -242,9 +247,10 @@
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
@@ -253,10 +259,10 @@
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
@@ -266,9 +272,10 @@
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
@@ -278,10 +285,10 @@
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
@@ -293,9 +300,10 @@
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
@@ -305,10 +313,10 @@
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
@@ -322,6 +330,7 @@
         TEQ     lr, #0
         STRNE   v2, [lr]
         MOV     a1, #0
+99
         Return  "v1-v6"
 
 os_byte
@@ -330,7 +339,7 @@
         MOV     ip, r2
         LDR     r1, [r1]
         LDR     r2, [r2]
-        SWI     XOS_Byte
+        SVC     #XOS_Byte
         STR     r1, [r3]
         STR     r2, [ip]
         MOVVC   r0, #0
@@ -338,7 +347,7 @@
 
 os_word
         MOV     ip, lr
-        SWI     XOS_Word
+        SVC     #XOS_Word
         MOVVC   r0, #0
         Return  ,LinkNotStacked,,ip
 
@@ -346,10 +355,10 @@
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
