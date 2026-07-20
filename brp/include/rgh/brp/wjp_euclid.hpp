#pragma once
/**
 * @file: brp/wjp_euclid.hpp
 * @brief:
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/brp/descriptor.hpp>
#include <rgh/brp/crc_utils.hpp>

namespace rgh {

class WJP_euclid_recv {
public:
    static constexpr int   BUFFER_MAX_SZ   = 254;

public:
    struct adapter_t {
        virtual int wjp_map_mark2sz( uint8_t mrk_ ) const noexcept = 0;
        virtual void wjp_process_packet( uint8_t mrk_, byte_t* pck_ ) noexcept = 0;
    };

public:
    WJP_euclid_recv( void ) = default;

    WJP_euclid_recv( adapter_t* adapter_ ) : _adapter{ adapter_ } {}

_RGH_PROTECTED:
    struct _state_t {
        uint8_t   mrk      = 0x0;
        uint8_t   trg_sz   = 0;
        uint8_t   crt_sz   = 0;
    }            _state     = {};
    adapter_t*   _adapter   = nullptr;
    byte_t       _buffer[ BUFFER_MAX_SZ ];

public:
    RGH_inline void swap_adapter( adapter_t* adapter_ ) { _adapter = adapter_; }

public:
    status_t encode( const byte_t* bytes_, int len_ ) {
        RGH_ASSERT_OR( bytes_ ) return RGH_ERR_BADARG;
    l_begin:
        RGH_ASSERT_OR( len_ > 0 ) return RGH_OK;

        if( _state.trg_sz == 0 ) {
            _state.mrk = bytes_[ 0x0 ];

            int sz = _adapter->wjp_map_mark2sz( _state.mrk );
            RGH_ASSERT_OR( sz <= BUFFER_MAX_SZ and sz >= 0 ) {
                ++bytes_; --len_; goto l_begin;
            }

            _state.trg_sz = ( uint8_t )sz;
            _state.crt_sz = 0;

            if( len_ == 1 ) return RGH_OK;
            ++bytes_; --len_;
        }

        int copy_len = _state.trg_sz - _state.crt_sz;
        const bool pck_end = len_ > copy_len;

        memcpy( &_buffer[ _state.crt_sz ], bytes_, copy_len );
        
        if( pck_end ) {
            RGH_ASSERT_OR( crc8_smbus( _buffer, _state.trg_sz ) != bytes_[ copy_len ] ) {
                return RGH_ERR_CORRUPTED;
            }
            
            _adapter->wjp_process_packet( _state.mrk, _buffer );

            _state.mrk = 0x0;
            _state.crt_sz = _state.trg_sz = 0;

            const int shift = copy_len + 1;
            if( (len_ -= shift) > 0 ) {
                bytes_ += shift;
                goto l_begin;
            }
        }

        return RGH_OK;
    }

};

};