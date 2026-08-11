#pragma once /*
# FILE: brp/wjp_euclid.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
*/

#include <rgh/brp/descriptor.hpp>
#include <rgh/brp/IO_port.hpp>
#include <rgh/brp/crc_utils.hpp>

namespace rgh {

class WJP_euclid {
public:
    static constexpr int   BUFFER_MAX_SZ   = 254;

public:
    struct adapter_t {
        virtual int wjp_euclid_map_mark2sz( uint8_t mrk_ ) const noexcept = 0;
        virtual void wjp_euclid_process_packet( uint8_t mrk_, byte_t* pck_ ) noexcept = 0;
    };

public:
    WJP_euclid( void ) = default;

    WJP_euclid( adapter_t* adapter_ ) : _adapter{ adapter_ } {}

_RGH_PROTECTED:
    struct _state_t {
        uint8_t   trg_sz   = 0;
        uint8_t   crt_sz   = 0;
    }            _state     = {};
    adapter_t*   _adapter   = nullptr;

    byte_t       _mrk       = 0x0;
    byte_t       _buffer[ BUFFER_MAX_SZ ];

public:
    RGH_inline void swap_adapter( adapter_t* adapter_ ) { _adapter = adapter_; }

public:
    status_t encode( const byte_t* bytes_, int len_ ) {
        RGH_ASSERT_OR( bytes_ ) return RGH_ERR_BADARG;
    l_begin:
        RGH_ASSERT_OR( len_ > 0 ) return RGH_OK;

        if( _state.trg_sz == 0 ) {
            _mrk = bytes_[ 0x0 ];

            int sz = _adapter->wjp_euclid_map_mark2sz( _mrk );
            RGH_ASSERT_OR( sz <= BUFFER_MAX_SZ and sz >= 0 ) {
                ++bytes_; --len_; goto l_begin;
            }

            _state.trg_sz = ( uint8_t )sz;
            _state.crt_sz = 0;

            if( len_ == 1 ) return RGH_OK;
            ++bytes_; --len_;
        }

        const int  copy_len = _state.trg_sz - _state.crt_sz;
        const bool pck_end  = len_ > copy_len;
        
        if( pck_end ) {
            memcpy( &_buffer[ _state.crt_sz ], bytes_, copy_len );

            RGH_ASSERT_OR( crc8_smbus( &_mrk, _state.trg_sz + 1 ) == bytes_[ copy_len ] ) {
                return RGH_ERR_CORRUPTED;
            }
            
            _adapter->wjp_euclid_process_packet( _mrk, _buffer );

            _mrk = 0x0;
            _state.crt_sz = _state.trg_sz = 0;

            const int shift = copy_len + 1;
            if( (len_ -= shift) > 0 ) {
                bytes_ += shift;
                goto l_begin;
            }
        } else {
            memcpy( &_buffer[ _state.crt_sz ], bytes_, len_ );
            _state.crt_sz += len_;
        }

        return RGH_OK;
    }

public:
    static status_t slow_send( io::Port* port_, uint8_t mrk_, uint8_t sz, const byte_t* bytes_ ) {
        RGH_ASSERT_OR( port_ ) return RGH_ERR_BADARG;
        RGH_ASSERT_OR( sz <= 254 and sz >= 0 ) return RGH_ERR_BADARG;

        const uint8_t pck_sz = sz + 2;
        byte_t buffer[ pck_sz ];

        buffer[ 0x0 ] = mrk_;
        memcpy( buffer + 0x1, bytes_, sz );
        buffer[ pck_sz - 1 ] = crc8_smbus( buffer, pck_sz - 1 );

        return port_->write( {
            .src_ptr = buffer,
            .src_n   = pck_sz
        } );
    }

};

};