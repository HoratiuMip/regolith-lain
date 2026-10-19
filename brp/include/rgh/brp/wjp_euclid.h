/*
# FILE: brp/wjp_euclid.h
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
*/
#ifndef WJP_EUCLID_H
#define WJP_EUCLID_H

#define WJP_EUCLID_DEFAULT_BOF ((WJP_u8)0xA5)  

#define WJP_EUCLID_PHS_BOF 0b00
#define WJP_EUCLID_PHS_MAL 0b01
#define WJP_EUCLID_PHS_PCK 0b10
#define WJP_EUCLID_PHS_CRC 0b11

#define WJP_RET_OK       0
#define WJP_RET_GENERAL  -1
#define WJP_RET_BADARG   -2
#define WJP_RET_CORRUPT  -3
#define WJP_RET_OVERFLOW -4

typedef   unsigned char   WJP_u8;

typedef struct WJP_euclid_context {
    WJP_u8    bof;
    WJP_u8    mrksz   : 2;
    WJP_u8    lensz   : 2; 
    WJP_u8    crcsz   : 2;

    WJP_u8    _phs;
    int       _cnt1;
    int       _mrk;
    int       _len;

    WJP_u8*   rxbuf;
    WJP_u8*   _rxptr;
    int       rxlen;

    int       (*cbk)( int, WJP_u8*, int );
} WJP_euclid_context_t;

#define WJP_EUCLID_CONTEXT( mrksz_, lensz_, crcsz_, rxbuf_, rxlen_, cbk_ ) \
    { .bof = WJP_EUCLID_DEFAULT_BOF, .mrksz = mrksz_, .lensz = lensz_, .crcsz = crcsz_, .rxbuf = rxbuf_, .rxlen = rxlen_, .cbk = cbk_ }

int WJP_euclid_init( WJP_euclid_context_t* ctx_ );

int WJP_euclid_push( WJP_euclid_context_t* ctx_, const WJP_u8* src_, int len_ );


#ifdef WJP_EUCLID_IMPLEMENTATION

static void _WJP_euclid_prep( WJP_euclid_context_t* ctx_ ) {
    ctx_->_cnt1  = 0;
    ctx_->_mrk   = 0;
    ctx_->_len   = ctx_->rxlen;
    ctx_->_rxptr = ctx_->rxbuf;
}

int WJP_euclid_init( WJP_euclid_context_t* ctx_ ) {
    if( !ctx_ ) return WJP_RET_BADARG;

    ctx_->_phs = WJP_EUCLID_PHS_BOF;

    return WJP_RET_OK;
}

int WJP_euclid_push( WJP_euclid_context_t* ctx_, const WJP_u8* src_, int len_ ) {
    if( !ctx_ || !src_ || len_ < 0 ) return WJP_RET_BADARG;

#define _WJP_EUCLID_ADVANCE {*ctx_->_rxptr = *src_; ++ctx_->_rxptr; ++src_; --len_;}
#define _WJP_EUCLID_SELECT( phs_ ) ctx_->_phs = phs_;

lbl_next:
    if( len_ == 0 ) return WJP_RET_OK;

    switch( ctx_->_phs ) {
        case WJP_EUCLID_PHS_BOF: {
            if( *src_ == ctx_->bof ) {
                _WJP_euclid_prep( ctx_ );
                _WJP_EUCLID_SELECT( ( ctx_->mrksz != 0 || ctx_->lensz != 0 ) ? WJP_EUCLID_PHS_MAL : WJP_EUCLID_PHS_PCK );
            }
            ++src_; --len_; goto lbl_next;
        break; }

        case WJP_EUCLID_PHS_MAL: {
            _WJP_EUCLID_ADVANCE; 
            ++ctx_->_cnt1;
            if( ctx_->_cnt1 == ctx_->mrksz + ctx_->lensz ) {
                if( ctx_->lensz != 0 ) ctx_->_len = 0;

                WJP_u8* p = ctx_->rxbuf;
                for( int i = 1; i <= ctx_->mrksz; ++i, ++p ) { if( i != 1 ) ctx_->_mrk <<= 8; ctx_->_mrk |= *p; }
                for( int i = 1; i <= ctx_->lensz; ++i, ++p ) { if( i != 1 ) ctx_->_len <<= 8; ctx_->_len |= *p; }

                if( ctx_->_len < 0 ) return WJP_RET_CORRUPT; 

                ctx_->_cnt1 = 0;
                _WJP_EUCLID_SELECT( ctx_->_len != 0 ? WJP_EUCLID_PHS_PCK : WJP_EUCLID_PHS_CRC );
            }
            goto lbl_next;
        break; }

        case WJP_EUCLID_PHS_PCK: {
            _WJP_EUCLID_ADVANCE;
            ++ctx_->_cnt1;
            if( ctx_->_cnt1 == ctx_->_len ) {
                ctx_->_cnt1 = 0;
                _WJP_EUCLID_SELECT( WJP_EUCLID_PHS_CRC );
            }
            goto lbl_next;
        break; }

        case WJP_EUCLID_PHS_CRC: {

        break; }

        default: return WJP_RET_CORRUPT;
    }

    ctx_->cbk( ctx_->_mrk, ctx_->rxbuf, ctx_->_len );
    _WJP_EUCLID_SELECT( WJP_EUCLID_PHS_BOF );
    goto lbl_next;

#undef _WJP_EUCLID_SELECT
#undef _WJP_EUCLID_ADVANCE
    return WJP_RET_OK;
}

#endif//#WJP_EUCLID_IMPLEMENTATION

#ifdef WJP_EUCLID_TEST

#define _WJP_EUCLID_TEST_ASSERT( c_ ) if( !(c_) ) return #c_;
#define _WJP_EUCLID_TEST_BUF_SZ 1024
#define _WJP_EUCLID_TEST_CTLPT_RESET _wjp_ctlpt_A = _wjp_ctlpt_B = 0;

static int _wjp_ctlpt_A = 0;
static int _wjp_ctlpt_B = 0;

int _WJP_euclid_test_cbk( int mrk_, WJP_u8* buf_, int len_ ) {
    _wjp_ctlpt_A = mrk_; _wjp_ctlpt_B = len_;
    return WJP_RET_OK;
}

const char* WJP_euclid_test( void ) {
    static WJP_u8 buffer[ _WJP_EUCLID_TEST_BUF_SZ ];
{
    WJP_euclid_context_t ctx = WJP_EUCLID_CONTEXT( 0, 0, 0, buffer, _WJP_EUCLID_TEST_BUF_SZ, &_WJP_euclid_test_cbk );
    _WJP_EUCLID_TEST_ASSERT( WJP_euclid_init( &ctx ) == WJP_RET_OK );
    
    _WJP_EUCLID_TEST_ASSERT( WJP_euclid_push( &ctx, buffer, _WJP_EUCLID_TEST_BUF_SZ ) == WJP_RET_OK );
}
{
    _WJP_EUCLID_TEST_CTLPT_RESET;

    WJP_euclid_context_t ctx = WJP_EUCLID_CONTEXT( 1, 1, 1, buffer, _WJP_EUCLID_TEST_BUF_SZ, &_WJP_euclid_test_cbk );
    _WJP_EUCLID_TEST_ASSERT( ctx.rxbuf == buffer );
    _WJP_EUCLID_TEST_ASSERT( ctx.rxlen == _WJP_EUCLID_TEST_BUF_SZ );
    _WJP_EUCLID_TEST_ASSERT( WJP_euclid_init( &ctx ) == WJP_RET_OK );

    WJP_euclid_push( &ctx, (const WJP_u8[1]){ 0xAA }, 1 );
    _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_BOF );

    WJP_euclid_push( &ctx, (const WJP_u8[1]){ WJP_EUCLID_DEFAULT_BOF }, 1 );
    _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_MAL );
    _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == buffer );

    WJP_euclid_push( &ctx, (const WJP_u8[1]){ 6 }, 1 );
    _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_MAL );
    _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == buffer+1 );

    WJP_euclid_push( &ctx, (const WJP_u8[1]){ 9 }, 1 );
    _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_PCK );
    _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == buffer+2 );
    _WJP_EUCLID_TEST_ASSERT( ctx._mrk == 6 );
    _WJP_EUCLID_TEST_ASSERT( ctx._len == 9 );

    for( int i = 1; i <= 9; ++i ) {
        WJP_euclid_push( &ctx, (const WJP_u8[1]){ i }, 1 );
    }
    _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_CRC );
    _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == buffer+11 );

    WJP_euclid_push( &ctx, (const WJP_u8[1]){ 0 }, 1 );
    _WJP_EUCLID_TEST_ASSERT( _wjp_ctlpt_A == 6 && _wjp_ctlpt_B == 9 );
    _WJP_EUCLID_TEST_ASSERT( ctx._phs == WJP_EUCLID_PHS_BOF );
    _WJP_EUCLID_TEST_ASSERT( ctx._rxptr == buffer+11 );
}
    

    return (const char*)0;
}

#endif//#WJP_EUCLID_TEST

#endif//#WJP_EUCLID_H