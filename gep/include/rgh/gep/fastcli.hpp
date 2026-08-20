#pragma once /*
# FILE: gep/fastcli.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# CAUTION:
#   The Fast_cli structure is NOT thread-safe because every method would be under a lock, which would create 
#     complications. Therefore, when you require a thread-safe version, wrap it in a Dispenser or such.
#
# ENHANCEMENTS:
#   [K] Check whether to implement the command map as an actual map or a sorted vector.
#   [K] Add functions that can push and pop commands dynamically from the command map.
#   [K] Implement command manuals.
#   [ ] Remake the parsing system such that there exists an entry point for already split tokens,
#         with the posibillity to call the parser with either a range or a single string.
*/
#include <rgh/gep/core.hpp>
#include <rgh/gep/dispenser.hpp>
#include <rgh/gep/text_utils.hpp>

namespace rgh {

#define RGH_FASTCLI_DEFAULT_STENCIL_CASES case 0x1: return RGH_ERR_BADARG;
#define RGH_FASTCLI_OPT_SWITCH_BEGIN(st_) char opt; while( opt = (st_).next() ) { switch( opt ) { RGH_FASTCLI_DEFAULT_STENCIL_CASES
#define RGH_FASTCLI_OPT_SWITCH_END } }

class Fast_cli {
public:
    struct cmd_t;

_RGH_PROTECTED:
//# Parsing context.
    struct _parse_ctx_t {
    //# The whole command line.
        const std::string&           text      = {};
    //# Final accumulated output of the command line execution.
        std::string*                 out       = nullptr;
    //# Tokens extracted from 'text'.
        std::vector< std::string >   toks      = {};
    //# Reference to effective command structure.
        const cmd_t*                 cmd       = nullptr;
    //# Context-wise index 0. Multiple uses.
        int                          id0       = 0x0;
    //# Current token during splitting 'text'.
        std::string                  crt_tok   = "";
    //# Flag if we are inside quotes while tokening.
        bool                         in_qte    = false;
    //# Reference to effective option token.
        const std::string*           opt_tok   = nullptr;
    //# Reference to effective option argument token.
        const std::string*           arg_tok   = nullptr; 

        bool push_crt_tok( void ) {
            if( crt_tok.empty() ) return false;
            toks.emplace_back( std::move( crt_tok ) ); return true;
        }
    };

public:
//# Master configuration of the parser.
    struct config_t {
    //# WHen this is not NULL, it is called back when "man <command>" is parsed.
        std::function< void( std::string_view, std::string_view ) >   when_man   = nullptr;

    //# Use std::string to take advantage of SSO.
        std::string   delim_chrs    = " \t\n";
        std::string   xtra_chrs     = ".,_*/+-?!:=~()[]^";
        std::string   esc_chrs      = "\\";
        std::string   qte_chrs      = "\"\'";
        std::string   var_chrs      = "$";
    };
    
    enum argtype_e {
        argflag,
        argtext, arglmhi = argtext,
        argi32, argf32, argf64
    };
    enum argcollect_e {
        argcsingle, argcmulticompact
    };
/*
# DETAILS: Command option.
#            Examples: (1): --some-opt (2): --some-opt-w-arg 69 (3): -s (4): -S 69
#          An option that does not start with neither '-' nor '--', is considered the nth option (using fast index).
*/
    struct opt_t {
        char           sh0rt     = '\0';
        std::string    l0ng      = {};
        argtype_e      arg       = argflag;
        int            fast_id   = -0x1;
        argcollect_e   argc      = argcsingle;
    };

//# Structure passed to the user parse callback. Usage is intended to be as close as possible to getopt_long().
    struct stencil_t {
        _parse_ctx_t*   _ctx   = nullptr;
        int             _fid   = 0x0;

        char            _arg_mem[ 32 ];
        void*           _arg   = nullptr;

    public:
        char next( void );
    
    public:
        [[deprecated]]RGH_inline auto& arg_text( void ) { return *(const std::string*)_arg; }
        [[deprecated]]RGH_inline auto  arg_i32( void )  { return *(int32_t*)_arg; }
        [[deprecated]]RGH_inline auto  arg_f32( void )  { return *(float*)_arg; }
        [[deprecated]]RGH_inline auto  arg_f64( void )  { return *(double*)_arg; }

        [[deprecated]]RGH_inline auto& arg_textv( void ) { return *(std::vector< const std::string* >*)_arg; }
        [[deprecated]]RGH_inline auto& arg_i32v( void )  { return *(std::vector< int32_t >*)_arg; }
        [[deprecated]]RGH_inline auto& arg_f32v( void )  { return *(std::vector< float >*)_arg; }
        [[deprecated]]RGH_inline auto& arg_f64v( void )  { return *(std::vector< double >*)_arg; }

        RGH_inline auto& text( void ) { return *(const std::string*)_arg; }
        RGH_inline auto str( void ) { return std::move( *reinterpret_cast< std::string* >( _arg ) ); }
        RGH_inline auto c_str( void ) { return reinterpret_cast< std::string* >( _arg )->c_str(); }
        RGH_inline auto i32( void )  { return *(int32_t*)_arg; }
        RGH_inline auto f32( void )  { return *(float*)_arg; }
        RGH_inline auto f64( void )  { return *(double*)_arg; }

        RGH_inline auto& textv( void ) { return *(std::vector< const std::string* >*)_arg; }
        RGH_inline auto& i32v( void )  { return *(std::vector< int32_t >*)_arg; }
        RGH_inline auto& f32v( void )  { return *(std::vector< float >*)_arg; }
        RGH_inline auto& f64v( void )  { return *(std::vector< double >*)_arg; }

    public:
        RGH_inline void operator += ( const std::string& line_ ) { *_ctx->out += line_; }

        template< typename ..._Args_ > RGH_inline void operator () ( const char* const fmt_, _Args_&&... args_ ) {
            this->operator+=( std::vformat( fmt_, std::make_format_args( std::forward< _Args_ >( args_ )... ) ) );
        }

    _RGH_PROTECTED:
        template< typename _T_cvt_ > bool _cvt_opt_arg( void* where_ ) {
            if constexpr( std::is_same_v< _T_cvt_, int32_t > ) {
                char* endptr = const_cast< char* >( _ctx->arg_tok->c_str() );
                *(int32_t*)where_ = (int32_t)strtol( endptr, &endptr, 0 );
                return endptr == &*_ctx->arg_tok->end();
            } else
            if constexpr( std::is_same_v< _T_cvt_, float > ) {
                char* endptr = const_cast< char* >( _ctx->arg_tok->c_str() );
                *(float*)where_ = (float)strtof( endptr, &endptr );
                return endptr == &*_ctx->arg_tok->end();
            } else
            if constexpr( std::is_same_v< _T_cvt_, double > ) {
                char* endptr = const_cast< char* >( _ctx->arg_tok->c_str() );
                *(double*)where_ = (double)strtod( endptr, &endptr );
                return endptr == &*_ctx->arg_tok->end();
            } 
            return false;
        }

        bool _cvt_opt_arg_single( argtype_e arg_ ) {
            const char* bad_cvt = nullptr;

            switch( arg_ ) {
                case argtext: {
                    _arg = (void*)_ctx->arg_tok;
                break; }
                case argi32: {
                    _arg = (void*)_arg_mem;
                    RGH_ASSERT_OR( _cvt_opt_arg< int32_t >( _arg ) ) bad_cvt = "i32";
                break; }
                case argf32: {
                    _arg = (void*)_arg_mem;
                    RGH_ASSERT_OR( _cvt_opt_arg< float >( _arg ) ) bad_cvt = "f32";
                break; }
                case argf64: {
                    _arg = (void*)_arg_mem;
                    RGH_ASSERT_OR( _cvt_opt_arg< double >( _arg ) ) bad_cvt = "f64";
                break; }
            }

            RGH_ASSERT_OR( bad_cvt == nullptr ) {
                *_ctx->out += std::format( "fast-cli: bad convert \"{}\" to {} required by option \"{}\".\n", *_ctx->arg_tok, bad_cvt, *_ctx->opt_tok );
                return false;
            }
            return true;
        }

        void _cvt_opt_arg_multi_compact_init( argtype_e arg_ ) {
            _arg = (void*)_arg_mem;
            switch( arg_ ) {
                case argtext: { new (_arg) std::vector< const std::string* >{}; break; }
                case argi32: { new (_arg) std::vector< int32_t >{}; break; }
                case argf32: { new (_arg) std::vector< float >{}; break; }
                case argf64: { new (_arg) std::vector< double >{}; break; }
            }
        }

        bool _cvt_opt_arg_multi_compact( argtype_e arg_ ) {
            switch( arg_ ) {
                case argtext: {
                    textv().emplace_back( _ctx->arg_tok );
                break; }
                case argi32: {
                    int32_t cvt = 0x0;
                    RGH_ASSERT_OR( _cvt_opt_arg< int32_t >( &cvt ) ) return false;
                    i32v().emplace_back( cvt );
                break; }
                case argf32: {
                    float cvt = 0x0;
                    RGH_ASSERT_OR( _cvt_opt_arg< float >( &cvt ) ) return false;
                    f32v().emplace_back( cvt );
                break; }
                case argf64: {
                    double cvt = 0x0;
                    RGH_ASSERT_OR( _cvt_opt_arg< double >( &cvt ) ) return false;
                    f64v().emplace_back( cvt );
                break; }
            }
            return true;
        }
    };

    using cmd_fnc_t = std::function< status_t( stencil_t& ) >;
    struct cmd_t {
        std::string            text   = {};
        std::vector< opt_t >   opts   = {};
        std::string_view       man    = "No manual provided for this command.";
        cmd_fnc_t              fnc    = nullptr;

        const opt_t* opt_by_short( char short_ ) const {
            auto itr = std::ranges::find_if( opts, [ short_ ] ( const opt_t& opt_ ) -> bool {
                return opt_.sh0rt == short_; 
            } );
            return itr != opts.end() ? &*itr : nullptr;
        }
        const opt_t* opt_by_long( const std::string& long_ ) const {
            auto itr = std::ranges::find_if( opts, [ long_ ] ( const opt_t& opt_ ) -> bool {
                return opt_.l0ng == long_; 
            } );
            return itr != opts.end() ? &*itr : nullptr;
        }
        const opt_t* opt_by_fast( int fid_ ) const {
            auto itr = std::ranges::find_if( opts, [ fid_ ] ( const opt_t& opt_ ) -> bool {
                return opt_.fast_id == fid_; 
            } );
            return itr != opts.end() ? &*itr : nullptr;
        }
    
        std::strong_ordering operator <=> ( const cmd_t& rhs_ ) const { return text <=> rhs_.text; }
        bool operator == ( const cmd_t& rhs_ ) const { return text == rhs_.text; }

        std::strong_ordering operator <=> ( std::string_view rhs_ ) const { return text <=> rhs_; }
        bool operator == ( std::string_view rhs_ ) const { return text == rhs_; }
    };

    using cmd_map_t = std::vector< cmd_t >;

public:
    Fast_cli( void ) = default;

    Fast_cli( const config_t& config_, const cmd_map_t& cmd_map_ ) 
    : _config{ config_ }, _cmd_map{ cmd_map_ } 
    {
        this->_prepare();
    }

_RGH_PROTECTED:
    config_t    _config    = {};
    cmd_map_t   _cmd_map   = {};

_RGH_PROTECTED:
//# Prepare the cli based on the current config and command map.
    void _prepare( void ) {
        if( _config.when_man ) {
            this->push_cmd( {
                .text = "man",
                .opts = { { .sh0rt = 'c', .l0ng = "command", .arg = argtext, .fast_id = 0x0 } },
                .man  = "MAN-CEPTION!!!",
                .fnc  = [ this ] ( auto& C_ ) -> status_t {
                    RGH_ASSERT_OR( _config.when_man ) return RGH_ERR_CORRUPTED;

                    std::string cmd = {};

                    RGH_FASTCLI_OPT_SWITCH_BEGIN(C_)
                        case 'c': cmd = C_.str(); break;
                    RGH_FASTCLI_OPT_SWITCH_END

                    auto itr = std::ranges::find( _cmd_map, cmd, &cmd_t::text );
                    RGH_ASSERT_OR( itr != _cmd_map.end() ) return RGH_ERR_NOT_FOUND;

                    this->_config.when_man( itr->text, itr->man );
                    return RGH_OK;
                }
            } );
        }

        std::ranges::sort( _cmd_map );
    }

public:
//# Bind a new config and command map.
    status_t bind( const config_t& config_, const cmd_map_t& cmd_map_ ) {
        _config  = config_;
        _cmd_map = cmd_map_;
        this->_prepare();
        return RGH_OK;
    }
//# Add a command to the command map.
    status_t push_cmd( const cmd_t& cmd_ ) {
        auto itr = std::ranges::lower_bound( _cmd_map, cmd_ );
        _cmd_map.insert( itr, cmd_ );
        return RGH_OK;
    }
//# Erase a command from the command map.
    status_t pop_cmd( std::string_view text_ ) {
        auto itr = std::ranges::find( _cmd_map, text_, &cmd_t::text );
        RGH_ASSERT_OR( itr != _cmd_map.end() ) return RGH_ERR_NOT_FOUND;
        _cmd_map.erase( itr );
        return RGH_OK;
    }

_RGH_PROTECTED:
    RGH_inline bool _is_xtra_chr( const char c_ ) const {
        return _config.xtra_chrs.find( c_, 0x0 ) != std::string::npos;
    }
    RGH_inline bool _is_text_chr( const char c_ ) const {
        return std::isalpha( c_ ) || std::isdigit( c_ ) || _is_xtra_chr( c_ );
    }
    RGH_inline bool _is_delim_chr( const char c_ ) const {
        return _config.delim_chrs.find( c_, 0x0 ) != std::string::npos;
    }
    RGH_inline bool _is_esc_chr( const char c_ ) const {
        return _config.esc_chrs.find( c_, 0x0 ) != std::string::npos;
    }
    RGH_inline bool _is_qte_chr( const char c_ ) const {
        return _config.qte_chrs.find( c_, 0x0 ) != std::string::npos;
    }

_RGH_PROTECTED:
    status_t _resolve_esc_chr( _parse_ctx_t* ctx_ ) {
        RGH_ASSERT_OR( ++ctx_->id0 < ctx_->text.length() ) return RGH_ERR_BADARG;

        static std::map< char, char > mrph_esc_chrs_map{
            { 'n', '\n' }, { 't', '\t' }
        };

        const char c = ctx_->text[ ctx_->id0 ];

        if( auto itr = mrph_esc_chrs_map.find( c ); itr != mrph_esc_chrs_map.end() ) {
            ctx_->crt_tok += itr->second;
        } else {
            ctx_->crt_tok += c;
        }
        return RGH_OK;
    }

    status_t _split_cmd( _parse_ctx_t* ctx_ ) {        
        for(; ctx_->id0 < ctx_->text.length(); ++ctx_->id0 ) {
            const char c = ctx_->text[ ctx_->id0 ];

            if( _is_text_chr( c ) ) {
                ctx_->crt_tok += c; 
                continue;
            }

            if( _is_esc_chr( c ) ) {
                RGH_ASSERT_OR( RGH_OK == _resolve_esc_chr( ctx_ ) ) {
                    *ctx_->out += std::format( "fast-cli: bad escape near {}.\n", ctx_->id0 );
                    return RGH_ERR_BADARG;
                }
                continue;
            }

            if( _is_delim_chr( c ) ) {
                if( not ctx_->in_qte ) {
                    ctx_->push_crt_tok();
                } else {
                    ctx_->crt_tok += c;
                }
                continue;
            }

            if( _is_qte_chr( c ) ) {
                ctx_->in_qte ^= true;
                continue;
            }

            *ctx_->out += std::format( "fast-cli: bad char near {}.\n", ctx_->id0 );
            return RGH_ERR_BADARG;
        }

        if( ctx_->in_qte ) {
            *ctx_->out += std::format( "fast-cli: bad quotes.\n", ctx_->id0 );
            return RGH_ERR_BADARG;
        }

        ctx_->push_crt_tok();
        return RGH_OK;
    }

    status_t _consume_toks( _parse_ctx_t* ctx_ ) {
        auto itr = std::ranges::lower_bound( _cmd_map, ctx_->toks[ 0x0 ], {}, &cmd_t::text );
        RGH_ASSERT_OR( itr != _cmd_map.end() && itr->text == ctx_->toks[ 0x0 ] ) {
            *ctx_->out += std::format( "fast-cli: unknown command \"{}\".\n", ctx_->toks[ 0x0 ] );
            return RGH_ERR_NOT_FOUND;
        }

        ctx_->cmd = &*itr;
        ctx_->id0 = 0x1;
        stencil_t stencil{
            ._ctx = ctx_
        };

        return itr->fnc( stencil );
    }

public:
    status_t execute( const std::string& text_, std::string* out_ ) {
        _parse_ctx_t ctx{
            .text = text_,
            .out  = out_
        };
        ctx.toks.reserve( 0x4 );

        RGH_ASSERT_STATUS_OR_RET( _split_cmd( &ctx ) );

        RGH_ASSERT_OR( not ctx.toks.empty() ) {
            *out_ += "fast-cli: nothing to do."; return RGH_ERR_BADARG;
        }

        return _consume_toks( &ctx );
    }

    RGH_inline status_t operator () ( const std::string& text_, std::string* out_ ) {
        return execute( text_, out_ );
    }
};

inline char Fast_cli::stencil_t::next( void ) {
#define _RET_DONE return 0x0;
#define _RET_ERR { _ctx->id0 = _ctx->toks.size(); return 0x1; }

    RGH_ASSERT_OR( _ctx->id0 < _ctx->toks.size() ) _RET_DONE;

    const opt_t* opt     = nullptr;
    const auto&  opt_tok = _ctx->toks[ _ctx->id0 ]; 
    _ctx->opt_tok        = &opt_tok;
    
    if( opt_tok.starts_with( "--" ) ) {
        opt = _ctx->cmd->opt_by_long( opt_tok.substr( 0x2 ) );
    } else if( opt_tok[ 0x0 ] == '-' ) {
        opt = _ctx->cmd->opt_by_short( opt_tok[ 0x1 ] );
    } else {
        opt = _ctx->cmd->opt_by_fast( _fid++ );
        RGH_ASSERT_OR( opt != nullptr ) {
            *_ctx->out += std::format( "fast-cli: unresolvable token \"{}\".\n", opt_tok );
            _RET_ERR;
        }
        goto l_fast_skip;
    }

    RGH_ASSERT_OR( opt != nullptr ) {
        *_ctx->out += std::format( "fast-cli: unresolvable option \"{}\".\n", opt_tok );
        _RET_ERR;
    }
    ++_ctx->id0;
    if( opt->arg != argflag ) { RGH_ASSERT_OR( _ctx->id0 < _ctx->toks.size() ) {
        *_ctx->out += std::format( "fast-cli: missing argument(s) for option \"{}\".\n", opt_tok );
        _RET_ERR;
    } } else {
        return opt->sh0rt;
    }
   
l_fast_skip:
    _ctx->arg_tok = &_ctx->toks[ _ctx->id0++ ];

    switch( opt->argc ) {
        case argcsingle: {
            RGH_ASSERT_OR( _cvt_opt_arg_single( opt->arg ) ) _RET_ERR;
        break; }

        case argcmulticompact: {
            _cvt_opt_arg_multi_compact_init( opt->arg );
            bool once = false;
            while( _cvt_opt_arg_multi_compact( opt->arg ) ) {
                once = true;
                RGH_ASSERT_OR( _ctx->id0 < _ctx->toks.size() ) goto l_toks_consumed;
                _ctx->arg_tok = &_ctx->toks[ _ctx->id0++ ];
            }
            --_ctx->id0;
            
        l_toks_consumed:
            RGH_ASSERT_OR( once ) {
                *_ctx->out += std::format( "fast-cli: missing or bad argument(s) for option \"{}\".\n", opt->l0ng );
                _RET_ERR;
            }
        break; }
    }

    return opt->sh0rt;
#undef _RET_ERR
#undef _RET_DONE
}

}