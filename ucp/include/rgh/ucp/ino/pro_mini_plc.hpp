#include "core.hpp"

namespace rgh::ino {

class Pro_mini_PLC {
_RGH_PROTECTED:
#pragma region BOARD PINS
    static constexpr pin_t   _Pin_IN_1   = A1;
    static constexpr pin_t   _Pin_IN_2   = A0;
    static constexpr pin_t   _Pin_IN_3   = A3;
    static constexpr pin_t   _Pin_IN_4   = A2;

    static constexpr pin_t   _Pin_BTN_1   = 2;
    static constexpr pin_t   _Pin_BTN_2   = 3;
    static constexpr pin_t   _Pin_BTN_3   = 4;
    static constexpr pin_t   _Pin_BTN_4   = 5;

    static constexpr pin_t   _Pin_RELAY_1   = 10;
    static constexpr pin_t   _Pin_RELAY_2   = 11;
    static constexpr pin_t   _Pin_RELAY_3   = 12;
    static constexpr pin_t   _Pin_RELAY_4   = 6;

    static constexpr pin_t   _Pin_LATCH   = 8;
    static constexpr pin_t   _Pin_CLOCK   = 9;
    static constexpr pin_t   _Pin_DATA    = 7;
#pragma endregion BOARD PINS

public:
    Pro_mini_PLC(
        RGH_IN   ms_t rl1_to_, ms_t rl2_to_, ms_t rl3_to_, ms_t rl4_to_
    ) 
    : relay{ rl1_to_, rl2_to_, rl3_to_, rl4_to_ }
    {
        pin_direction( OUTPUT,
            _Pin_LATCH, _Pin_CLOCK, _Pin_DATA,
            _Pin_RELAY_1, _Pin_RELAY_2, _Pin_RELAY_3, _Pin_RELAY_4
        );
    }

public:
    struct relay_t {
        relay_t(
            RGH_IN   ms_t rl1_to_, ms_t rl2_to_, ms_t rl3_to_, ms_t rl4_to_
        )
        : one{ rl1_to_ }, two{ rl2_to_ }, three{ rl3_to_ }, four{ rl4_to_ }
        {}

        timed_output_ccv_t< _Pin_RELAY_1, LOW >   one     = {};
        timed_output_ccv_t< _Pin_RELAY_2, LOW >   two     = {};
        timed_output_ccv_t< _Pin_RELAY_3, LOW >   three   = {};
        timed_output_ccv_t< _Pin_RELAY_4, LOW >   four    = {};

        void tick( RGH_INO_NOW_MS_ARG ) {
            one.tick( now_ms_ ); two.tick( now_ms_ ); three.tick( now_ms_ ); four.tick( now_ms_ );
        }
    } relay;

    struct button_t {
        input_cc_t< _Pin_BTN_1, LOW >   one     = {};
        input_cc_t< _Pin_BTN_2, LOW >   two     = {};
        input_cc_t< _Pin_BTN_3, LOW >   three   = {};
        input_cc_t< _Pin_BTN_4, LOW >   four    = {};

        void tick( void ) {
            one.tick(); two.tick(); three.tick(); four.tick();
        }
    } button;

    struct input_t {
        input_cc_t< _Pin_IN_1, LOW >   one     = {};
        input_cc_t< _Pin_IN_2, LOW >   two     = {};
        input_cc_t< _Pin_IN_3, LOW >   three   = {};
        input_cc_t< _Pin_IN_4, LOW >   four    = {};

        void tick( void ) {
            one.tick(); two.tick(); three.tick(); four.tick();
        }
    } input;

public:
    void tick( RGH_INO_NOW_MS_ARG ) {
        relay.tick( now_ms_ );
        button.tick();
        input.tick();
        _seg7_disp_tick();
    }

#pragma region 7 SEGMENT DISPLAY 
_RGH_PROTECTED:
    inline static const uint8_t   _DISP_SEGS[]   = { 0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0x88,0x83,0xc6,
                                                     0xa7,0xa1,0x86,0x8e,0x89,0x8b,0xc7,0xab,0xc8,0xa3,0x8c, 0b10001100,0x87,
                                                     0xc1,0xbf, 0b10010010, 0xff };
    inline static const uint8_t   _DISP_POS[]    = { 0b0001, 0b0010, 0b0100, 0b1000 };

    uint8_t   _disp_pos                         = 0x0;
    uint8_t   _disp_buf[ sizeof( _DISP_POS ) ]   = { 0xff, 0xff, 0xff, 0xff };

    void _seg7_disp_tick( void ) {
        digitalWrite( _Pin_LATCH, LOW );
        shiftOut( _Pin_DATA, _Pin_CLOCK, MSBFIRST, _DISP_POS[ _disp_pos ] );
        shiftOut( _Pin_DATA, _Pin_CLOCK, MSBFIRST, _DISP_SEGS[ _disp_buf[ _disp_pos ] ] );
        digitalWrite( _Pin_LATCH, HIGH );

        if( ++_disp_pos >= sizeof( _DISP_POS ) ) _disp_pos = 0;
    }

public: 
//# 10 = A, b, C, c, d, 
//# 15 = E, F, H, h, L, 
//# 20 = n, N, o, P, r, 
//# 25 = t, U, -, S, ' '
    void seg7_write( 
        RGH_IN   uint8_t d1_, uint8_t d2_, uint8_t d3_, uint8_t d4_ 
    ) {
        _disp_buf[ 0x0 ] = d1_; _disp_buf[ 0x1 ] = d2_; _disp_buf[ 0x2 ] = d3_; _disp_buf[ 0x3 ] = d4_;
    }
    void seg7_write( 
        RGH_IN   int16_t num_ 
    ) {
        seg7_write( num_/1000%10, num_/100%10, num_/10%10, num_%10 );
    }
#pragma endregion 7 SEGMENT DISPLAY 

};

}