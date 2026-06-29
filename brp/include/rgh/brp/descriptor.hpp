/*
[RGH] CAUTION!
THIS FILE WAS GENERATED DURING BUILD AND IT WILL BE OVERRIDEN IN THE NEXT ONE.
DO NOT MODIFY AS THE MODIFICATIONS WILL BE LOST.
*/
#pragma once
/**
 * @file: BRp/descriptor.hpp.in
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */


#define RGH_VERSION_MAJOR 1
#define RGH_VERSION_MINOR 0
#define RGH_VERSION_PATCH 3
#define RGH_VERSION_STRING "rghv1.0.3"

#define RGH_inline inline
#define RGH_IMPL_FNC

#define _RGH_PROTECTED protected
#define _RGH_PRIVATE   private

#ifdef __EXCEPTIONS
    #define _RGH_EXCEPTIONS

    #define _rgh_try try
    #define _rgh_catch( scope_ ) catch(...) scope_
#else
    #define _rgh_try
    #define _rgh_catch( scope_ )
#endif

#define RGH_ASSERT_OR(c)            if( !(c) )
#define RGH_ASSERT_AND(c)           if((c))
#define RGH_ASSERT_STATUS_OR(c)     if(rgh::status_t status_=(c);RGH_OK!=status_)
#define RGH_ASSERT_STATUS_OR_RET(c) RGH_ASSERT_STATUS_OR(c) {return status_;}
#define RGH_ASSERT_STATUS_AND(c)     if(rgh::status_t status_=(c);RGH_OK==status_)

#ifndef RGH_NO_EZ_ASSERTS
    #define ASSERT_OR(c) RGH_ASSERT_OR(c)
    #define ASSERT_AND(c) RGH_ASSERT_AND(c)
    #define ASSERT_STATUS_OR(c) RGH_ASSERT_STATUS_OR(c)
    #define ASSERT_STATUS_OR_RET(c) RGH_ASSERT_STATUS_OR_RET(c)
    #define ASSERT_STATUS_AND(c) RGH_ASSERT_STATUS_AND(c)
#endif

#define RGH_STRUCT_HAS_OVR( obj, fnc ) ((void*)((obj).*(&fnc))!=(void*)(&fnc))


#ifndef BV
    #define BV(b) (decltype(b){0x1}<<(b))
#endif
#ifndef SBV
    #define SBV(x,b) ((x)|=BV((decltype(x))(b)))
#endif
#ifndef RBV
    #define RBV(x,b) ((x)&=~BV((decltype(x))(b)))
#endif
#ifndef FBV
    #define FBV(x,f,b) (f?SBV((x),(b)):RBV((x),(b)))
#endif
#ifndef SET
    #define SET 0x1
#endif
#ifndef RESET
    #define RESET 0x0
#endif


#define RGH_IN
#define RGH_OUT
#define RGH_IN_OPT
#define RGH_OUT_OPT
#define RGH_IN_OUT
#define RGH_IN_OUT_OPT


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


namespace rgh {

typedef   int   status_t;

#define RGH_OK               0x0
#define RGH_ERR_GENERAL      -0x1
#define RGH_ERR_SYSCALL      -0x2
#define RGH_ERR_WOULD_OVRWR  -0x3
#define RGH_ERR_OPEN         -0x4
#define RGH_ERR_EXCOMCALL    -0x5
#define RGH_ERR_LOGIC        -0x6
#define RGH_ERR_USERCALL     -0x7
#define RGH_ERR_PLATFORMCALL -0x8
#define RGH_ERR_BADARG       -0x9
#define RGH_ERR_FLOW         -0xA
#define RGH_ERR_NOT_IMPL     -0xB
#define RGH_ERR_BUSY         -0xC
#define RGH_ERR_NOT_FOUND    -0xD
#define RGH_ERR_ENGINECALL   -0xE
#define RGH_ERR_TIMEOUT      -0xF
#define RGH_ERR_TERMINATED   -0x10
#define RGH_ERR_INTERRUPTED  -0x11
#define RGH_ERR_NO_RESOLVE   -0x12
#define RGH_ERR_BADALLOC     -0x13

inline static const char* const _status_msgs[] = {
    "OK", "GENERAL", "SYSCALL", "WOULD_OVRWR", "OPEN",
    "EXCOMCALL", "LOGIC", "USERCALL", "PLATFORMCALL", "BADARG",
    "FLOW", "NOT_IMPL", "BUSY", "NOT_FOUND", "ENGINECALL",
    "TERMINATED", "NO_RESOLVE", "BADALLOC"
};
#define RGH_STATUS_MSG(s) (rgh::_status_msgs[-(s)])

#ifndef RGH_NO_UNSCOPED_STATUS_DEFS
    #define OK               0x0
    #define ERR_GENERAL      -0x1
    #define ERR_SYSCALL      -0x2
    #define ERR_WOULD_OVRWR  -0x3
    #define ERR_OPEN         -0x4
    #define ERR_EXCOMCALL    -0x5
    #define ERR_LOGIC        -0x6
    #define ERR_USERCALL     -0x7
    #define ERR_PLATFORMCALL -0x8
    #define ERR_BADARG       -0x9
    #define ERR_FLOW         -0xA
    #define ERR_NOT_IMPL     -0xB
    #define ERR_BUSY         -0xC
    #define ERR_NOT_FOUND    -0xD
    #define ERR_ENGINECALL   -0xE
    #define ERR_TIMEOUT      -0xF
    #define ERR_TERMINATED   -0x10
    #define ERR_INTERRUPTED  -0x11
    #define ERR_NO_RESOLVE   -0x12
    #define ERR_BADALLOC     -0x13
#endif

#define RGH_UNIMPLEMENTED {return RGH_ERR_NOT_IMPL;}

struct MDsc {
    typedef   int   n_t;

    char*   ptr   = nullptr;
    n_t     n     = 0;    
};

};

