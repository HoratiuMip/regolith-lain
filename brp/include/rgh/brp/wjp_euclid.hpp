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

class WJP_euclid_MVC8 {
public:
    static constexpr int   BUFFER_MAX_SZ   = 254;

public:
    struct adapter_t {
        virtual int wjp_euclid_map_mark2sz( uint8_t mrk_ ) const noexcept = 0;
        virtual void wjp_euclid_process_packet( uint8_t mrk_, byte_t* pck_ ) noexcept = 0;
    };

public:
    WJP_euclid_MVC8( void ) = default;
    WJP_euclid_MVC8( adapter_t* adapter_ ) : _adapter{ adapter_ } {}

_RGH_PROTECTED:
    struct _state_t {
        uint8_t   trg_sz   = 0;
        uint8_t   crt_sz   = 0;
    }            _state                     = {};
    adapter_t*   _adapter                   = nullptr;

    byte_t       _mrk                       = 0x0;
    byte_t       _buffer[ BUFFER_MAX_SZ ];
 ImGui::InputFloat( "Base fare", &internal::tarrif_sim_config.base_fare, 0.01f, 0.0f, "%.2f Euro", ImGuiInputTextFlags_None );
_RGH_PROTECTED:
    RGH_inline void _reset_state( void ) {
        _mrk = 0x0;
        _state.trg_sz = _state.crt_sz = 0;
    }

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
            RGH_ASSERT_OR( sz <= BUFFER_MAX_SZ && sz >= 0 ) {
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
            _state.crt_sz += copy_len;

            RGH_ASSERT_OR( crc8_smbus( &_mrk, _state.trg_sz + 1 ) == bytes_[ copy_len ] ) {
                return RGH_ERR_CORRUPTED;
            }
            
            _adapter->wjp_euclid_process_packet( _mrk, _buffer );
            this->_reset_state();

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

    status_t encode_slide_when_corrupt( const byte_t* bytes_, int len_ ) {
    l_begin:
        status_t ret = this->encode( bytes_, len_ );
        if( ret != RGH_ERR_CORRUPTED ) return ret;

        RGH_ASSERT_OR( _state.crt_sz > 1 ) return RGH_ERR_CORRUPTED;

        memmove( _buffer, _buffer+1, _state.crt_sz-1 );
        this->_reset_state();
        goto l_begin;
    }

public:
    static status_t slow_send( io::Port* port_, uint8_t mrk_, uint8_t sz, const byte_t* bytes_ ) {
        RGH_ASSERT_OR( port_ ) return RGH_ERR_BADARG;
        RGH_ASSERT_OR( sz <= 254 ) return RGH_ERR_BADARG;

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
using WJP_euclid = WJP_euclid_MVC8;

};