#include <bridge.hpp>
MDN_MODULE_HEADER( cli, "cli" )

struct Dock : dock_t {
public:
    Dock( void )
    : _cli{
        {},
        { 
        {   .text = "progctl",
            .opts = {
                { .sh0rt = 'c', .l0ng = "clear" },
                { .sh0rt = 'e', .l0ng = "exit" },
                { .sh0rt = 'v', .l0ng = "version" }
            },
            .fnc = [ this ] ( auto& stencil_ ) -> status_t {
                char opt; while( opt = stencil_.next() ) {
                    switch( opt ) { RGH_FASTCLI_DEFAULT_STENCIL_CASES
                        case 'c': {
                            
                        break; }
                        case 'e': {
                            BridgE.signal_stop();
                            return RGH_OK;
                        }
                        case 'v': {
                            stencil_( "Madonna version: {}", MDN_VERSION_STR );
                        break; }
                    }
                }
                return RGH_OK;
            }
        }, {
            .text = "proxy-pass",
            .opts = {
                { .sh0rt = 't', .l0ng = "target", .arg = Fast_cli::Arg_text, .fast_id = 0x0 },
                { .sh0rt = 'c', .l0ng = "command", .arg = Fast_cli::Arg_text, .fast_id = 0x1 }
            },
            .fnc = [] ( auto& stencil_ ) -> status_t {
                string target;
                string command;

                char opt;  while( opt = stencil_.next() ) {
                    switch( opt ) { RGH_FASTCLI_DEFAULT_STENCIL_CASES
                        case 't': {
                            target = move( stencil_.arg_text() );
                        break; }
                        case 'c': {
                            command = move( stencil_.arg_text() );
                        break; }
                    }
                }
                
                string out;
                BridgE.proxy_pass( target, command, &out );

                return RGH_OK;
            }
        }, {
            .text = "let",
            .opts = {
                { .sh0rt = 'i', .l0ng = "id", .arg = Fast_cli::Arg_text, .fast_id = 0x0 },
                { .sh0rt = 'v', .l0ng = "value", .arg = Fast_cli::Arg_f64, .fast_id = 0x1 }
            },
            .fnc = [ this ] ( auto& stencil_ ) -> status_t {
                string id;
                double      value;

                char opt; while( opt = stencil_.next() ) {
                    switch( opt ) { RGH_FASTCLI_DEFAULT_STENCIL_CASES
                        case 'i': { id = move( stencil_.arg_text() ); break; }
                        case 'v': { value = stencil_.arg_f64(); break; }
                    }
                }

                MDN_ASSERT_OR( not id.empty() ) {
                    stencil_ += "let: id not specified.";
                    return RGH_ERR_LOGIC;
                }

                auto var_reg = BridgE.var_reg.control();
                ( *var_reg )[ id ] = HVec< var_t >::make( var_t{
                    .what = var_t::Scalar,
                    .val  = value
                } );

                stencil_( "let: {} = {}", id, value );
                return RGH_OK;
            }
        }, {
            .text = "eval",
            .opts = {
                { .sh0rt = 's', .l0ng = "string", .arg = Fast_cli::Arg_text, .fast_id = 0x0 }
            },
            .fnc = [ this ] ( auto& stencil_ ) -> status_t {
                double result = 0.0;

                char opt;  while( opt = stencil_.next() ) {
                    switch( opt ) { RGH_FASTCLI_DEFAULT_STENCIL_CASES
                        case 's': {
                            MDN_ASSERT_OR( RGH_OK == _exp.parse( stencil_.arg_text() ) ) {
                                stencil_ += "eval: parse error.";
                                return RGH_ERR_BADARG;
                            }
                            _exp.resolve( &result );
                            stencil_( "eval: {}.", result );
                        break; }
                    }
                }
    
                return RGH_OK;
            }
        }, {
            .text = "roots",
            .opts = {
                { .sh0rt = 'c', .l0ng = "coeffs", .arg = Fast_cli::Arg_f64, .fast_id = 0x0, .argc = Fast_cli::Argc_multi_compact }
            },
            .fnc = [] ( auto& stencil_ ) -> status_t {
                vector< double > coeffs;

                char opt;  while( opt = stencil_.next() ) {
                    switch( opt ) { RGH_FASTCLI_DEFAULT_STENCIL_CASES
                        case 'c': {
                            coeffs = move( stencil_.arg_f64v() );
                        break; }
                    }
                }
        
                for( auto& root : roots( coeffs ) ) {
                    stencil_ += format( 
                        "{:+} {:+}i\n", root.real(), root.imag() 
                    );
                }
                return RGH_OK;
            }
        }
        }
    } {
        _exp.bind( [] ( string_view var_, double* res_ ) -> status_t {
            auto var_reg = BridgE.var_reg.watch();
            auto itr     = var_reg->find( var_.data() );
            MDN_ASSERT_OR( itr != var_reg->end() ) return RGH_ERR_NOT_FOUND;

            auto& var = itr->second;
            MDN_ASSERT_OR( var->what == var_t::Scalar ) return RGH_ERR_NOT_IMPL;

            *res_ = any_cast< double >( var->val );
            return RGH_OK;
        } );
    }

public:
    struct _guix_t {
        string                      cmd_in   = {};
        rgh::Dispenser< string >   cmd_out  = { rgh::DispenserMode_Trylock };
    } _guix;

    Fast_cli             _cli;
    Fast_exp< double >   _exp;

public:
    MDN_DOCK_NAME_FNC override { return DOCK_NAME; }

    MDN_DOCK_WAKE_FNC override {
        for( int idx = 0x1; idx < args_.argc; ++idx )
            thread( [ this, cmd = args_.argv[ idx ] ] {
                string out;
                _cli.execute( cmd, &out );
            } ).detach();
        
        return RGH_OK;
    }

    MDN_DOCK_GUIX_FNC override {
        const ImGuiWindowFlags_ window_flags = ImGuiWindowFlags_None;

        if( ImGui::Begin( "Command Line Interpreter", nullptr, window_flags ) ) {
            const auto cmd_in_flags = ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
            ImGui::SeparatorText( "Input" );
            const bool enter = ImGui::InputTextWithHint( format( " @ {}", MDN_VERSION_STR ).c_str(), "command...", &_guix.cmd_in, cmd_in_flags );
            ImGui::Separator();

            if( enter ) {
                thread( [ this ] {
                    string out;
                    _cli.execute( _guix.cmd_in, &out );
                    auto cmd_out = _guix.cmd_out.control();
                    *cmd_out = move( out );
                } ).detach();
            }

            ImGui::BeginChild( "##_cli_out" );
                if( auto cmd_out = _guix.cmd_out.watch() ) {
                    ImGui::TextUnformatted( cmd_out->c_str() );
                }
            ImGui::EndChild();
        } 
        ImGui::End();
        return RGH_OK; 
    }
};

MDN_AUTO_INSTALL( Dock );
MDN_MODULE_FOOTER
