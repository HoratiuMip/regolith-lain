/*
# FILE: brp/WJPv5.h
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Layer 6 protocol.
*/
#ifndef WJPV5_H
#define WJPV5_H

#define WJP_RET_OK  0
#define WJP_RET_GEN -1
#define WJP_RET_ARG -2
#define WJP_RET_COR -3
#define WJP_RET_OFW -4

typedef   unsigned char   WJP_u8;
typedef   unsigned int    WJP_u;

#define WJP_ASSERT_OR(c_) if(!(c_))

//# CRC-8/SMBUS lookup table.
static const WJP_u8 _wjp_crc8_smbus_lktbl[ 256 ] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

WJP_u8 wjp_crc8( const WJP_u8* const src_, const int len_ ) {
    WJP_u8 crc = 0x00;
    for( int i = 0; i < len_; ++i ) crc = _wjp_crc8_smbus_lktbl[ crc ^ src_[i] ]; 
    return crc;
}

#pragma region WJP Euclid
#define WJP_EUCLID_DEFAULT_BOF ((WJP_u8)0xA5)  

#define WJP_EUCLID_PHS_BOF 0b000
#define WJP_EUCLID_PHS_MRK 0b001
#define WJP_EUCLID_PHS_LEN 0b010
#define WJP_EUCLID_PHS_PCK 0b011
#define WJP_EUCLID_PHS_CRC 0b100
#define WJP_EUCLID_PHS_INV 0xFF

#define WJP_EUCLID_RX_CALLBACK_RET_OK     0x0
#define WJP_EUCLID_RX_CALLBACK_RET_RELOCK -0x1

typedef struct WJP_euclid_config {
    WJP_u8    bof;
    WJP_u8    mrksz   : 2;
    WJP_u8    lensz   : 2; 
    WJP_u8    crcsz   : 2;
} WJP_euclid_config_t;

typedef struct WJP_euclid_context {
    WJP_euclid_config_t   cfg;

    WJP_u8    _phs;
    int       _cnt1;
    WJP_u     _mrk;
    WJP_u     _len;
    WJP_u     _crc;

    WJP_u8*   rxbuf;
    WJP_u8*   _rxptr;
    WJP_u     rxlen;

    int       (*rxcbk)( void*, WJP_u, WJP_u8*, WJP_u );
    void*     usr;
} WJP_euclid_context_t;
typedef   int( *wjp_euclid_rx_callback_t )( void*, WJP_u, WJP_u8*, WJP_u );

#define WJP_EUCLID_CONTEXT( mrksz_, lensz_, crcsz_, rxbuf_, rxlen_, rxcbk_, usr_ ) \
    { .cfg = { .bof = WJP_EUCLID_DEFAULT_BOF, .mrksz = (mrksz_), .lensz = (lensz_), .crcsz = (crcsz_) }, .rxbuf = (rxbuf_), .rxlen = (rxlen_), .rxcbk = (rxcbk_), .usr = (usr_) }
#define WJP_EUCLID_CONTEXT_EX( bof_, mrksz_, lensz_, crcsz_, rxbuf_, rxlen_, rxcbk_, usr_ ) \
    { .cfg = { .bof = (bof_), .mrksz = (mrksz_), .lensz = (lensz_), .crcsz = (crcsz_) }, .rxbuf = (rxbuf_), .rxlen = (rxlen_), .rxcbk = (rxcbk_), .usr = (usr_) }

WJP_u WJP_euclid_meta_size( const WJP_euclid_config* const );
void WJP_euclid_emplace_meta3( const WJP_euclid_config* const, WJP_u8** const, const WJP_u, const WJP_u );
void WJP_euclid_emplace_meta4( const WJP_euclid_config* const, WJP_u8*, const WJP_u );
void WJP_euclid_emplace_frame( const WJP_euclid_config* const, WJP_u8*, const WJP_u, const WJP_u8* const, const WJP_u );
int WJP_euclid_init( WJP_euclid_context_t* const ctx_ );
int WJP_euclid_effective( const WJP_euclid_context_t* const ctx_ );
int WJP_euclid_push( WJP_euclid_context_t* const ctx_, const WJP_u8* src_, int len_ );


#ifdef WJP_EUCLID_IMPLEMENTATION

#if defined(__has_include)
    #if __has_include( <string.h> )
        #include <string.h>

        void _wjp_memcpy( WJP_u8* dst_, const WJP_u8* src_, WJP_u sz_ ) { memcpy( dst_, src_, sz_ ); }
    #else
        void _wjp_memcpy( WJP_u8* dst_, const WJP_u8* src_, WJP_u sz_ ) { for( WJP_u i = 0; i < sz_; ++i ) dst_[i] = src_[i]; }
    #endif
#endif 

static WJP_u8 _wjp_euclid_crc8_rx( const WJP_euclid_context* const ctx_ ) {
    WJP_u8 crc = 0x00;
    for( int i = 0; i < ctx_->cfg.mrksz; ++i ) crc = _wjp_crc8_smbus_lktbl[ crc ^ ((WJP_u8*)&ctx_->_mrk)[i] ];
    for( int i = 0; i < ctx_->cfg.lensz; ++i ) crc = _wjp_crc8_smbus_lktbl[ crc ^ ((WJP_u8*)&ctx_->_len)[i] ];
    for( int i = 0; i < ctx_->_len; ++i ) crc = _wjp_crc8_smbus_lktbl[ crc ^ ctx_->rxbuf[i] ]; 
    return crc;
}

static WJP_u8* _wjp_euclid_rx_view( const WJP_euclid_context* const ctx_ ) {
    return (WJP_u8*)&ctx_->_mrk;
}

static WJP_u8* _wjp_euclid_rx_view_advance( const WJP_euclid_context* const ctx_, WJP_u8* p_ ) {
    ++p_;
    if( p_ == (WJP_u8*)&ctx_->_mrk + ctx_->cfg.mrksz ) p_ = (WJP_u8*)&ctx_->_len;
    if( p_ == (WJP_u8*)&ctx_->_len + ctx_->cfg.lensz ) p_ = ctx_->rxbuf;
    if( p_ == ctx_->rxbuf + ctx_->_len ) p_ = (WJP_u8*)&ctx_->_crc;
    if( p_ == (WJP_u8*)&ctx_->_crc + ctx_->cfg.crcsz ) p_ = (WJP_u8*)0;
    return p_;
}

/// Find the first BOF in the reception buffer and push again the bytes. This is used when an unaligned or partial
///   reception has occured: the packet validation failed (bad mark, length or crc) but there is a chance that the start of 
///   the actual packet is partially or completely in the reception buffer.
static void _wjp_euclid_relock_bof( WJP_euclid_context* const ctx_ ) {
//# Switch the context state machine back to searching for the BOF.
    ctx_->_phs = WJP_EUCLID_PHS_BOF;
//# Find the BOF, if any.
    int k = 0, f = 0;
    WJP_u8 buf[ 6 ];
    for( WJP_u8* p = _wjp_euclid_rx_view( ctx_ ); p; p = _wjp_euclid_rx_view_advance( ctx_, p ) ) { 
        if( p == ctx_->rxbuf ) break;
        if( *p == ctx_->cfg.bof ) f = 1;
        if( f ) buf[ k++ ] = *p;
    }
//# Push the mark and length bytes if the BOF has been found among them. Make a copy of the length so we 
//#   know how many bytes to push from the reception buffer.
    int prev_len = ctx_->_len;
    int prev_crc = ctx_->_crc;
    if( f ) {
        WJP_euclid_push( ctx_, buf, k );
        WJP_euclid_push( ctx_, ctx_->rxbuf, prev_len );
    } else {
        for( int i = 0; i < prev_len; ++i ) {
            if( ctx_->rxbuf[i] != ctx_->cfg.bof ) continue;
            WJP_euclid_push( ctx_, &ctx_->rxbuf[i], prev_len - i );
            break;
        }
    }
//# Push the bytes stored in the CRC field.
    WJP_euclid_push( ctx_, (WJP_u8*)&prev_crc, ctx_->cfg.crcsz );
    
    return;
}


WJP_u WJP_euclid_meta_size( const WJP_euclid_config* const cfg_ ) {
    return 1 + cfg_->mrksz + cfg_->lensz + cfg_->crcsz;
}

void WJP_euclid_emplace_meta3( const WJP_euclid_config* const cfg_, WJP_u8** const dst_, const WJP_u mrk_, const WJP_u len_ ) {
    *(*dst_)++ = cfg_->bof;
    for( int i = 0; i < cfg_->mrksz; ++i ) *(*dst_)++ = ((const WJP_u8*)&mrk_)[i];
    for( int i = 0; i < cfg_->lensz; ++i ) *(*dst_)++ = ((const WJP_u8*)&len_)[i];
}
void WJP_euclid_emplace_meta4( const WJP_euclid_config* const cfg_, WJP_u8* dst_, const WJP_u crc_ ) {
    for( int i = 0; i < cfg_->crcsz; ++i ) *dst_++ = ((const WJP_u8*)&crc_)[i];
}
void WJP_euclid_emplace_frame( const WJP_euclid_config* const cfg_, WJP_u8* dst_, const WJP_u mrk_, const WJP_u8* const src_, const WJP_u len_ ) {
    const WJP_u8* const dst_cpy = dst_;

    WJP_euclid_emplace_meta3( cfg_, &dst_, mrk_, len_ );
    _wjp_memcpy( dst_, src_, len_ ); dst_ += len_;

    WJP_u crc = 0x0;
    switch( cfg_->crcsz ) {
        case 0: break;
        case 1: WJP_euclid_emplace_meta4( cfg_, dst_, wjp_crc8( dst_cpy+1, cfg_->mrksz + cfg_->lensz + len_ ) ); 
    }
}



int WJP_euclid_init( WJP_euclid_context_t* const ctx_ ) {
    WJP_ASSERT_OR( ctx_ ) return WJP_RET_ARG;
    WJP_ASSERT_OR( (ctx_->rxbuf != (WJP_u8*)0) == (ctx_->rxlen != 0)  ) goto l_asserts_failed;

    goto l_asserts_passed;

l_asserts_failed:
    ctx_->_phs = WJP_EUCLID_PHS_INV;
    return WJP_RET_GEN;

l_asserts_passed:
    ctx_->_phs = WJP_EUCLID_PHS_BOF;
    return WJP_RET_OK;
}

int WJP_euclid_effective( const WJP_euclid_context_t* const ctx_ ) {
    return ctx_->_phs != WJP_EUCLID_PHS_INV ? WJP_RET_OK : WJP_RET_GEN;
}

int WJP_euclid_push( WJP_euclid_context_t* const ctx_, const WJP_u8* src_, int len_ ) {
    if( !ctx_ || !src_ || len_ < 0 ) return WJP_RET_ARG;

#define _WJP_EUCLID_CONSUME (--len_,src_++)
#define _WJP_EUCLID_ADVANCE {*ctx_->_rxptr++ = *_WJP_EUCLID_CONSUME; ++ctx_->_cnt1;}
#define _WJP_EUCLID_SELECT( phs_, cnt_, rxptr_ ) {ctx_->_phs = (phs_); ctx_->_cnt1 = (cnt_); ctx_->_rxptr = (WJP_u8*)(rxptr_); goto lbl_root;}

lbl_root:
    if( len_ == 0 ) return WJP_RET_OK;

    switch( ctx_->_phs ) {
        case WJP_EUCLID_PHS_BOF: {
            if( *_WJP_EUCLID_CONSUME == ctx_->cfg.bof ) {
                ctx_->_cnt1  = 0;
                ctx_->_mrk   = 0;
                ctx_->_crc   = 0;

                if( ctx_->cfg.mrksz == 0 ) {
lbl_len:
                    if( ctx_->cfg.lensz == 0 ) {
                        ctx_->_len = ctx_->rxlen;
                        _WJP_EUCLID_SELECT( WJP_EUCLID_PHS_PCK, 0, ctx_->rxbuf );
                    }
                    ctx_->_len = 0;
                    _WJP_EUCLID_SELECT( WJP_EUCLID_PHS_LEN, 0, &ctx_->_len );
                }
                _WJP_EUCLID_SELECT( WJP_EUCLID_PHS_MRK, 0, &ctx_->_mrk );
            }
        goto lbl_root; }

        case WJP_EUCLID_PHS_MRK: {
            _WJP_EUCLID_ADVANCE;
            if( ctx_->_cnt1 == ctx_->cfg.mrksz ) goto lbl_len;
        goto lbl_root; }

        case WJP_EUCLID_PHS_LEN: {
            _WJP_EUCLID_ADVANCE;
            if( ctx_->_cnt1 == ctx_->cfg.lensz ) {
                if( ctx_->_len == 0 ) {
lbl_crc:
                    if( ctx_->cfg.crcsz == 0 ) goto lbl_call_rxcbk;
                    _WJP_EUCLID_SELECT( WJP_EUCLID_PHS_CRC, 0, &ctx_->_crc );
                }
                WJP_ASSERT_OR( ctx_->_len <= ctx_->rxlen ) {
                    _wjp_euclid_relock_bof( ctx_ );
                    goto lbl_root;
                }
                _WJP_EUCLID_SELECT( WJP_EUCLID_PHS_PCK, 0, ctx_->rxbuf );
            }
        goto lbl_root; }

        case WJP_EUCLID_PHS_PCK: {
            _WJP_EUCLID_ADVANCE;
            if( ctx_->_cnt1 == ctx_->_len ) goto lbl_crc;
        goto lbl_root; }

        case WJP_EUCLID_PHS_CRC: {
            _WJP_EUCLID_ADVANCE;
            if( ctx_->_cnt1 == ctx_->cfg.crcsz ) {
                switch( ctx_->cfg.crcsz ) {
                    case 1: {
                        WJP_u8 crc = _wjp_euclid_crc8_rx( ctx_ );
                        WJP_ASSERT_OR( (WJP_u)crc == ctx_->_crc ) {
                            _wjp_euclid_relock_bof( ctx_ );
                            goto lbl_root;
                        }
                    goto lbl_call_rxcbk; }
                }
            }
        goto lbl_root; }

        default: return WJP_RET_COR;
    }

lbl_call_rxcbk: {
    ctx_->rxcbk( ctx_->usr, ctx_->_mrk, ctx_->rxbuf, ctx_->_len );
    _WJP_EUCLID_SELECT( WJP_EUCLID_PHS_BOF, 0, 0 );

    goto lbl_root; }

#undef _WJP_EUCLID_SELECT
#undef _WJP_EUCLID_ADVANCE
#undef _WJP_EUCLID_CONSUME
    return WJP_RET_OK;
}

#endif//#WJP_EUCLID_IMPLEMENTATION
#pragma endregion WJP Euclid

#ifdef WJP_EUCLID_TEST

#define _WJP_EUCLID_TEST_STR_( l_ ) #l_
#define _WJP_EUCLID_TEST_STR( l_ ) _WJP_EUCLID_TEST_STR_( l_ )
#define _WJP_EUCLID_TEST_ASSERT_( c_, f_, ... ) if( !(c_) ) _WJP_EUCLID_TEST_FMT_ERR( #c_ " @ line " _WJP_EUCLID_TEST_STR( __LINE__ ), f_ __VA_OPT__(,) __VA_ARGS__ )
#define _WJP_EUCLID_TEST_ASSERT( c_, ... ) _WJP_EUCLID_TEST_ASSERT_( c_ __VA_OPT__(,) __VA_ARGS__, "" )
#define _WJP_EUCLID_TEST_BUF_SZ 200
#define _WJP_EUCLID_TEST_CTLPT_RESET _wjp_ctlpt.A = _wjp_ctlpt.B = 0; _wjp_ctlpt.C = 0;

static char _wjp_error_str[ 256 ];
#if defined(__has_include)
    #if __has_include( <stdio.h> )
        #include <stdio.h>
        #define _WJP_EUCLID_TEST_FMT_ERR( ww_, f_, ... ) {snprintf( _wjp_error_str, sizeof(_wjp_error_str), "%s: " f_, ww_ __VA_OPT__(,) __VA_ARGS__ ); return _wjp_error_str;}
    #else
        #define _WJP_EUCLID_TEST_FMT_ERR( ww_, f_, ... ) return ww_;
    #endif
#endif 

static struct {
    int A, B, C;
} _wjp_ctlpt;

int _WJP_euclid_test_rxcbk( void*, WJP_u mrk_, WJP_u8* buf_, WJP_u len_ ) {
    _wjp_ctlpt.A = mrk_; _wjp_ctlpt.B = len_; ++_wjp_ctlpt.C;
    return WJP_RET_OK;
}

const char* WJP_euclid_test( void ) {
    static WJP_u8 buffer[ _WJP_EUCLID_TEST_BUF_SZ ];
//# 000
{
    _WJP_EUCLID_TEST_CTLPT_RESET

    WJP_euclid_context_t ctx = WJP_EUCLID_CONTEXT( 0, 0, 0, buffer, _WJP_EUCLID_TEST_BUF_SZ, &_WJP_euclid_test_rxcbk, (void*)0 );
        _WJP_EUCLID_TEST_ASSERT( WJP_euclid_init( &ctx ) == WJP_RET_OK );
        _WJP_EUCLID_TEST_ASSERT( WJP_euclid_push( &ctx, buffer, _WJP_EUCLID_TEST_BUF_SZ ) == WJP_RET_OK );
}
//# 111
{
    WJP_euclid_context_t ctx = WJP_EUCLID_CONTEXT( 1, 1, 1, buffer, _WJP_EUCLID_TEST_BUF_SZ, &_WJP_euclid_test_rxcbk, (void*)0 );
        _WJP_EUCLID_TEST_ASSERT( ctx.rxbuf == buffer );
        _WJP_EUCLID_TEST_ASSERT( ctx.rxlen == _WJP_EUCLID_TEST_BUF_SZ );
        _WJP_EUCLID_TEST_ASSERT( WJP_euclid_init( &ctx ) == WJP_RET_OK );

//# Byte by byte push, BAD CRC at the end.
    _WJP_EUCLID_TEST_CTLPT_RESET;
    WJP_euclid_push( &ctx, (const WJP_u8[1]){ 0xAA }, 1 );
        _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_BOF );
    WJP_euclid_push( &ctx, (const WJP_u8[1]){ WJP_EUCLID_DEFAULT_BOF }, 1 );
        _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_MRK );
        _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == (WJP_u8*)&ctx._mrk );
    WJP_euclid_push( &ctx, (const WJP_u8[1]){ 6 }, 1 );
        _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_LEN );
        _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == (WJP_u8*)&ctx._len );
    WJP_euclid_push( &ctx, (const WJP_u8[1]){ 9 }, 1 );
        _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_PCK );
        _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == buffer );
        _WJP_EUCLID_TEST_ASSERT( ctx._mrk == 6 );
        _WJP_EUCLID_TEST_ASSERT( ctx._len == 9 );
    for( int i = 1; i <= 9; ++i ) {
        WJP_euclid_push( &ctx, (const WJP_u8[1]){ (WJP_u8)i }, 1 );
    }
        _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_CRC );
        _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == (WJP_u8*)&ctx._crc );
        for( int i = 1; i <= 9; ++i ) {
            _WJP_EUCLID_TEST_ASSERT( (WJP_u8)i == buffer[i-1], "Expected %d at index %d, got %d.", i, i-1, (int)buffer[i-1] );
        }
    WJP_euclid_push( &ctx, (const WJP_u8[1]){ 0 }, 1 );
        _WJP_EUCLID_TEST_ASSERT( _wjp_ctlpt.A == 0 && _wjp_ctlpt.B == 0, "CRC must fail and NOT callback" );
        _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_BOF, "Bad CRC must switch context to BOF search" );
    
//# Whole correct packet pushed.
    {
        _WJP_EUCLID_TEST_CTLPT_RESET;
        WJP_u8 packet[ 13 ] = { WJP_EUCLID_DEFAULT_BOF, 6, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
        packet[12] = wjp_crc8( packet+1, 11 );
        WJP_euclid_push( &ctx, packet, sizeof( packet ) );
            _WJP_EUCLID_TEST_ASSERT( _wjp_ctlpt.A == 6 && _wjp_ctlpt.B == 9, "CRC must pass and callback" );
            _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_BOF );
            _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == (WJP_u8*)0 );

        WJP_u8* p = (WJP_u8*)&ctx._mrk; 
        p = _wjp_euclid_rx_view_advance( &ctx, p );
            _WJP_EUCLID_TEST_ASSERT( p == (WJP_u8*)&ctx._len );
        p = _wjp_euclid_rx_view_advance( &ctx, p );
            _WJP_EUCLID_TEST_ASSERT( p == ctx.rxbuf );
        for( int i = 1; i <= 9; ++i ) p = _wjp_euclid_rx_view_advance( &ctx, p );
            _WJP_EUCLID_TEST_ASSERT( p == (WJP_u8*)&ctx._crc );
        p = _wjp_euclid_rx_view_advance( &ctx, p );
            _WJP_EUCLID_TEST_ASSERT( p == (WJP_u8*)0 );
    }
//# Two whole packets pushed.
    {
        _WJP_EUCLID_TEST_CTLPT_RESET;
        WJP_u8 packet[ 17 ] = { WJP_EUCLID_DEFAULT_BOF, 6, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, WJP_EUCLID_DEFAULT_BOF, 3, 0, 0 };
        packet[12] = wjp_crc8( packet+1, 11 );
        packet[16] = wjp_crc8( packet+14, 2 );
        WJP_euclid_push( &ctx, packet, sizeof( packet ) );
            _WJP_EUCLID_TEST_ASSERT( _wjp_ctlpt.A == 3 && _wjp_ctlpt.B == 0 );
            _WJP_EUCLID_TEST_ASSERT( _wjp_ctlpt.C == 2, "Callback must be invoked twice, but it was invoked %d times", (int)_wjp_ctlpt.C );
            _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_BOF );

        WJP_u8* p = (WJP_u8*)&ctx._mrk; 
        p = _wjp_euclid_rx_view_advance( &ctx, p );
            _WJP_EUCLID_TEST_ASSERT( p == (WJP_u8*)&ctx._len );
        p = _wjp_euclid_rx_view_advance( &ctx, p );
            _WJP_EUCLID_TEST_ASSERT( p == (WJP_u8*)&ctx._crc );
        p = _wjp_euclid_rx_view_advance( &ctx, p );
            _WJP_EUCLID_TEST_ASSERT( p == (WJP_u8*)0 );
    }
//# One BAD packet and one correct packet, overlapped.
    {
        _WJP_EUCLID_TEST_CTLPT_RESET;
        WJP_u8 packet[ 10 ] = { 0, WJP_EUCLID_DEFAULT_BOF, 5, 3, 1, WJP_EUCLID_DEFAULT_BOF, 3, 1, 2, 0 };
        packet[9] = wjp_crc8( packet+6, 3 );
        WJP_euclid_push( &ctx, packet, sizeof( packet ) );
            _WJP_EUCLID_TEST_ASSERT( _wjp_ctlpt.A == 3 && _wjp_ctlpt.B == 1 && _wjp_ctlpt.C == 1, "Unexpected A=%d, B=%d, C=%d", (int)_wjp_ctlpt.A, (int)_wjp_ctlpt.B, (int)_wjp_ctlpt.C );
            _WJP_EUCLID_TEST_ASSERT( ctx.rxbuf[0x0] == 2 );
            _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_BOF );
    }
//# One BAD packet and one correct packet, overlapped.
    {
        _WJP_EUCLID_TEST_CTLPT_RESET;
        WJP_u8 packet[ 10 ] = { 0, WJP_EUCLID_DEFAULT_BOF, 5, 3, 1, WJP_EUCLID_DEFAULT_BOF, 3, 1, 2, 0 };
        packet[9] = wjp_crc8( packet+6, 3 );
        WJP_euclid_push( &ctx, packet, sizeof( packet ) );
            _WJP_EUCLID_TEST_ASSERT( _wjp_ctlpt.A == 3 && _wjp_ctlpt.B == 1 && _wjp_ctlpt.C == 1, "Unexpected A=%d, B=%d, C=%d", (int)_wjp_ctlpt.A, (int)_wjp_ctlpt.B, (int)_wjp_ctlpt.C );
            _WJP_EUCLID_TEST_ASSERT( ctx.rxbuf[0x0] == 2 );
            _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_BOF );
    }
//# Same as before but the length of first packet is more than the reception buffer length.
    {
        _WJP_EUCLID_TEST_CTLPT_RESET;
        WJP_u8 packet[ 10 ] = { 0, WJP_EUCLID_DEFAULT_BOF, 5, 210, 1, WJP_EUCLID_DEFAULT_BOF, 3, 1, 2, 0 };
        packet[9] = wjp_crc8( packet+6, 3 );
        WJP_euclid_push( &ctx, packet, sizeof( packet ) );
            _WJP_EUCLID_TEST_ASSERT( _wjp_ctlpt.A == 3 && _wjp_ctlpt.B == 1 && _wjp_ctlpt.C == 1, "Unexpected A=%d, B=%d, C=%d", (int)_wjp_ctlpt.A, (int)_wjp_ctlpt.B, (int)_wjp_ctlpt.C );
            _WJP_EUCLID_TEST_ASSERT( ctx.rxbuf[0x0] == 2 );
            _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_BOF );
    }
//# Whole correct packet pushed using helpers.
    {
        _WJP_EUCLID_TEST_CTLPT_RESET;
        WJP_u8 data[ 9 ] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        
        WJP_u sz = WJP_euclid_meta_size( &ctx.cfg ) + sizeof( data );
            _WJP_EUCLID_TEST_ASSERT( sz == 4 + 9 );

        WJP_u8 packet[ sz ];
        WJP_euclid_emplace_frame( &ctx.cfg, packet, 6, data, 9 );

        WJP_euclid_push( &ctx, packet, sizeof( packet ) );
            _WJP_EUCLID_TEST_ASSERT( _wjp_ctlpt.A == 6 && _wjp_ctlpt.B == 9, "CRC must pass and callback" );
            _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_BOF );
            _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == (WJP_u8*)0 );
    }
}
    

    return (const char*)0;
}

#endif//#WJP_EUCLID_TEST

#endif//#WJPV5_H