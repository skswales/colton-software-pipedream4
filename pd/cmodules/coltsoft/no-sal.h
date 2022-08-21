/* no-sal.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2014 Stuart Swales */

/* SKS 2012 */

/*
Some SAL2.0 fakes to SAL1.1 - only defined as needed
*/

#ifndef _In_reads_

#define _In_reads_(size)                                _In_count_(size)
#define _In_reads_opt_(size)                            _In_opt_count_(size)

#define _In_reads_z_(size)                              _In_z_count_(size)

#define _In_reads_bytes_(size)                          _In_bytecount_(size)
#define _In_reads_bytes_opt_(size)                      _In_opt_bytecount_(size)

#define _Out_writes_(size)                              _Out_cap_(size)
#define _Out_writes_opt_(size)                          _Out_opt_cap_(size)

#define _Out_writes_z_(size)                            _Out_z_cap_(size)

#define _Out_writes_bytes_(size)                        _Out_bytecap_(size)
#define _Out_writes_bytes_opt_(size)                    _Out_opt_bytecap_(size)

#define _Inout_updates_(size)                           _Inout_cap_(size)
#define _Inout_updates_opt_(size)                       _Inout_opt_cap_(size)

#define _Inout_updates_z_(size)                         _Inout_z_cap_(size)

#define _Inout_updates_bytes_(size)                     _Inout_bytecap_(size)
#define _Inout_updates_bytes_opt_(size)                 _Inout_opt_bytecap_(size)

#define _Ret_maybenull_z_                               _Ret_opt_z_

#define _Ret_writes_(size)                              _Ret_count_(size)
#define _Ret_writes_maybenull_(size)                    _Ret_opt_count_(size)

#define _Ret_writes_to_(size, count)                    _Ret_count_(size)
#define _Ret_writes_to_maybenull_(size, count)          _Ret_opt_count_(size)

#define _Ret_writes_z_                                  _Ret_z_

#define _Ret_writes_bytes_(size)                        _Ret_bytecount_(size)
#define _Ret_writes_bytes_maybenull_(size)              _Ret_opt_bytecount_(size)

#define _Ret_writes_bytes_to_(size, count)              _Ret_bytecount_(size)
#define _Ret_writes_bytes_to_maybenull_(size, count)    _Ret_opt_bytecount_(size)

#endif /* _In_reads_ */

/*
Some more macro defs so that we can compile code
with these VC2008 style SAL1.1 decorations on
on VC2005 with SAL 1.0 decorations
or indeed with CC Norcroft on RISC OS!
*/

#ifndef _Success_

#ifdef __check_return
#define _Check_return_                  __check_return
#else
#define _Check_return_
#endif

#ifdef __success
#define _Success_(expr)                 __success(expr)
#else
#define _Success_(expr)
#endif

#define _Pre_valid_
#define _Pre_notnull_
#define _Pre_maybenull_

#define _Post_invalid_

#ifdef __in
#define _In_                            __in
#else
#define _In_
#endif

#ifdef __in_bcount
#define _In_bytecount_(countvar)        __in_bcount(countvar)
#else
#define _In_bytecount_(countvar)
#endif

#define _In_bytecount_c_(constexpr)
#define _In_bytecount_x_(complexexpr)

#ifdef __in_ecount
#define _In_count_(countvar)            __in_ecount(countvar)
#else
#define _In_count_(countvar)
#endif

#define _In_count_c_(constexpr)
#define _In_count_x_(complexexpr)

#ifdef __in_opt
#define _In_opt_                        __in_opt
#else
#define _In_opt_
#endif

#ifdef __in_bcount_opt
#define _In_opt_bytecount_(countvar)    __in_bcount_opt(countvar)
#else
#define _In_opt_bytecount_(countvar)
#endif

#ifdef __in_ecount_opt
#define _In_opt_count_(countvar)        __in_ecount_opt(countvar)
#else
#define _In_opt_count_(countvar)
#endif

#define _In_opt_count_c_(constexpr)
#define _In_opt_count_x_(complexexpr)

#ifdef __in_z_opt
#define _In_opt_z_                      __in_z_opt
#else
#define _In_opt_z_
#endif

#ifdef __in_z
#define _In_z_                          __in_z
#else
#define _In_z_
#endif

#ifdef __in_bcount_z
#define _In_z_bytecount_(countvar)      __in_bcount_z(countvar)
#else
#define _In_z_bytecount_(countvar)
#endif

#ifdef __in_ecount_z
#define _In_z_count_(countvar)          __in_ecount_z(countvar)
#else
#define _In_z_count_(countvar)
#endif

#ifdef __inout
#define _Inout_                         __inout
#else
#define _Inout_
#endif

#ifdef __inout_bcount
#define _Inout_bytecap_(countvar)       __inout_bcount(countvar)
#else
#define _Inout_bytecap_(countvar)
#endif

#define _Inout_bytecap_c_(constexpr)
#define _Inout_bytecap_x_(complexexpr)
#define _Inout_bytecount_(countvar)     /*__inout_bcount_full(countvar)*/
#define _Inout_bytecount_c_(constexpr)
#define _Inout_bytecount_x_(complexexpr)

#ifdef __inout_ecount
#define _Inout_cap_(countvar)           __inout_ecount(countvar)
#else
#define _Inout_cap_(countvar)
#endif

#define _Inout_cap_c_(constexpr)
#define _Inout_cap_x_(complexexpr)
#define _Inout_count_(countvar)         /*__inout_ecount_full(countvar)*/
#define _Inout_count_c_(constexpr)
#define _Inout_count_x_(complexexpr)

#ifdef __inout_opt
#define _Inout_opt_                     __inout_opt
#else
#define _Inout_opt_
#endif

#ifdef __inout_z
#define _Inout_z_                       __inout_z
#else
#define _Inout_z_
#endif

#ifdef __inout_bcount_z
#define _Inout_z_bytecap_(countvar)     __inout_bcount_z(countvar)
#else
#define _Inout_z_bytecap_(countvar)
#endif

#ifdef __inout_ecount_z
#define _Inout_z_cap_(countvar)         __inout_ecount_z(countvar)
#else
#define _Inout_z_cap_(countvar)
#endif

#ifdef __out
#define _Out_                           __out
#else
#define _Out_
#endif

#ifdef __out_bcount
#define _Out_bytecap_(countvar)         __out_bcount(countvar)
#else
#define _Out_bytecap_(countvar)
#endif

#define _Out_bytecap_c_(constexpr)
#define _Out_bytecap_x_(complexvar)
#define _Out_bytecapcount_(countvar)    /*__out_bcount_full(countvar)*/
#define _Out_bytecapcount_c_(constexpr)
#define _Out_bytecapcount_x_(complexvar)

#ifdef __out_ecount
#define _Out_cap_(countvar)             __out_ecount(countvar)
#else
#define _Out_cap_(countvar)
#endif

#define _Out_cap_c_(constexpr)
#define _Out_cap_x_(complexexpr)
#define _Out_capcount_(countvar)        /*__out_ecount_full(countvar)*/
/*#define _Out_capcount_c_(constexpr)*/
#define _Out_capcount_x_(complexexpr)

#ifdef __out_opt
#define _Out_opt_                       __out_opt
#else
#define _Out_opt_
#endif

#ifdef __out_bcount_opt
#define _Out_opt_bytecap_(countvar)     __out_bcount_opt(countvar)
#else
#define _Out_opt_bytecap_(countvar) 
#endif

#define _Out_opt_z_bytecap_(countvar)   /*__out_bcount_z_opt(countvar)*/
#define _Out_opt_z_cap_(countvar)       /*__out_ecount_z_opt(countvar)*/

#ifdef __out_bcount_z
#define _Out_z_bytecap_(countvar)       __out_bcount_z(countvar)
#else
#define _Out_z_bytecap_(countvar)
#endif

#define _Out_z_bytecap_c_(constexpr)

#ifdef __out_ecount_z
#define _Out_z_cap_(countvar)           __out_ecount_z(countvar)
#else
#define _Out_z_cap_(countvar)
#endif

#define _Printf_format_string_

#define _Ret_notnull_
#define _Ret_maybenull_

#define _Ret_
#define _Ret_opt_

#define _Ret_z_
#define _Ret_opt_z_

#define _Ret_count_(size)
#define _Ret_opt_count_(size)

#define _Ret_bytecount_(size)
#define _Ret_opt_bytecount_(size)

#endif /* _Success_ */

/* end of no-sal.h */
