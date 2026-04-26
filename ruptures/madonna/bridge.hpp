#pragma once

#include <rgh/gep/fastcli.hpp>
#include <rgh/gep/fastexp.hpp>
#include <rgh/gep/dispenser.hpp>
using namespace rgh;

#define RGH_MDN_REAL_T double
#include <rgh/osp/madonna.hpp>
using namespace rgh::mdn;

#include <rgh/osp/immersive.hpp>

#define MDN_VERSION_MAJOR 1
#define MDN_VERSION_MINOR 0
#define MDN_VERSION_PATCH 0
#define MDN_VERSION_STR "madonna-v1.0.0"

#define MDN_ASSERT_OR(c) RGH_ASSERT_OR(c)

#define MDN_IN
#define MDN_OUT
#define MDN_IN_OPT
#define MDN_OUT_OPT
#define MDN_IN_OUT
#define MDN_IN_OUT_OPT

using namespace std;

#define MDN_MODULE_HEADER(namespace_name,dock_name) \
    static const char* const DOCK_NAME = dock_name; \
    namespace namespace_name {
#define MDN_MODULE_FOOTER \
    };

/// ====== PROXY ====== ///
#define MDN_PROXY_CLI_BASIC_INSTALL { \
        .text = "install", \
        .fnc = [ this ] ( auto& stencil_ ) -> status_t { return BridgE.install( rgh::HVec< Dock >::make() ); } \
    }

class proxy_t {
public:
    friend class _bridge_t;

public:
    proxy_t( 
        MDN_IN   const string&                name_, 
        MDN_IN   const Fast_cli::config_t&    cli_config_, 
        MDN_IN   const Fast_cli::cmd_map_t&   cli_cmd_map_
    ) : _name{ name_ }, _cli{ cli_config_, cli_cmd_map_ } {}

protected:
    string    _name   = {};
    Fast_cli  _cli    = {};

public:
    status_t pass( const string& text_, string* out_ ) {
        return _cli.execute( text_, out_ );
    }

};

/// ====== DOCK ====== ///
#define MDN_DOCK_NAME_FNC \
    virtual string_view name( void )

#define MDN_DOCK_WAKE_FNC \
    virtual status_t wake( \
        MDN_IN   const wake_args_t&    args_ \
    )

#define MDN_DOCK_GUIX_FNC \
    virtual status_t guix_frame( \
        MDN_IN   const Immersive::frame_cb_args_t&   args_ \
    )

class dock_t {
public:
    friend class _bridge_t;

public:
    struct wake_args_t {
        int      argc;
        char**   argv;
    };

protected:
    string   _id   = {};

protected:
    MDN_DOCK_NAME_FNC { return "unknown"; }
    MDN_DOCK_WAKE_FNC { return RGH_OK; }
    MDN_DOCK_GUIX_FNC { return RGH_OK; }

};

#define MDN_AUTO_INSTALL( t, ... ) \
    static struct _mdn_installer_##t##_t_ { \
        _mdn_installer_##t##_t_( void ) { \
            BridgE.install( HVec< t >::make( __VA_ARGS__ ) ); \
        } \
    } _mdn_installer_##t##_; 


class var_t {
public:
    enum What_ { Null, Scalar };

public:
    What_   what   = Null;
    any     val    = {};
};

/// ====== BRIDGE ====== ///
class _bridge_t : public bridge_t {
public:
    _bridge_t( void ) : bridge_t{ MDN_VERSION_STR } {
        logger->info( "bridge: init done." );
    }

// ======================= Fields =======================
public:
    Immersive                                          imm           = {};
    
    Dispenser< map< string, HVec< var_t > > >          var_reg       = { DispenserMode_Lock };

protected:
    atomic< status_t >                                 _status       = { RGH_ERR_TERMINATED };

    Dispenser< map< string_view, HVec< proxy_t > > >   _proxys       = { DispenserMode_Lock };
    Dispenser< map< string_view, HVec< dock_t > > >    _docks        = { DispenserMode_Lock };

    thread                                             _th_guix      = {};

public:
    RGH_inline auto status( void ) { return _status.load( memory_order_relaxed ); }

    RGH_inline void wait_stop( void ) { _status.wait( RGH_OK, memory_order_seq_cst ); this->stop(); }

public:
    status_t install(
        MDN_IN   HVec< proxy_t>   proxy_
    ) {
        MDN_ASSERT_OR( proxy_ ) {
            logger->error( "bridge: install proxy: null proxy." );
            return RGH_ERR_BADARG;
        }

        string_view sv = proxy_->_name;
        MDN_ASSERT_OR( not sv.empty() ) {
            logger->error( "bridge: install proxy: empty name." ); 
            return RGH_ERR_FLOW;
        }

        auto proxys = _proxys.control();

        auto& proxy = ( *proxys )[ sv ];
        MDN_ASSERT_OR( not proxy ) {
            proxys.release();
            logger->error( "bridge: register proxy: \"{}\" already exists.", sv );
            return RGH_ERR_WOULD_OVRWR;
        }

        proxy = move( proxy_ );
        proxys.release();

        logger->info( "bridge: installed proxy \"{}\".", sv );
        return RGH_OK;
    }

    void uninstall_proxy(
        MDN_IN   const string&   proxy_name_  
    ) {
        auto proxys = _proxys.control();
        proxys->erase( proxy_name_ );
        proxys.release();

        logger->info( "bridge: uninstalled proxy \"{}\".", proxy_name_ );
    }

    status_t install( 
        MDN_IN   HVec< dock_t >    dock_ 
    ) {
        MDN_ASSERT_OR( dock_ ) {
            logger->error( "bridge: install dock: null dock." );
            return RGH_ERR_BADARG;
        }

        dock_->_id = format( "{}-{:x}", dock_->name(), time( nullptr ) );

        string_view sv = dock_->_id;
        MDN_ASSERT_OR( not sv.empty() ) {
            logger->error( "bridge: install dock: empty id." ); 
            return RGH_ERR_FLOW;
        }

        auto docks = _docks.control();
        auto& dock = ( *docks )[ sv ];
        MDN_ASSERT_OR( not dock ) {
            docks.release();
            logger->error( "bridge: install dock: \"{}\" already exists.", sv ); 
            return RGH_ERR_WOULD_OVRWR;
        }

        dock = move( dock_ );
        docks.release();

        logger->info( "bridge: installed dock \"{}\".", sv );
        return RGH_OK;
    }

    void uninstall_dock( 
        MDN_IN   const string&   dock_id_
    ) {
        auto docks = _docks.control();
        docks->erase( dock_id_ );
        docks.release();

        logger->info( "bridge: uninstalled dock \"{}\".", dock_id_ );
    }

public:
    status_t start( int argc_, char* argv_[] ) {
        MDN_ASSERT_OR( not _th_guix.joinable() && this->status() != RGH_OK ) {
            logger->error( "bridge: start: already running." );
            return RGH_ERR_LOGIC;
        }

        _status.store( RGH_OK, memory_order_release );

        _th_guix = thread( &Immersive::main, &imm, 0, nullptr, Immersive::config_t{
            .ctx        = nullptr,
            .title      = MDN_VERSION_STR,
            .width      = 680,
            .height     = 680,
            .srf_bgn_as = Immersive::SrfBeginAs_Default,
            .init_cb    = [ this ] ( const auto& args_ ) -> auto {
                ImGui::StyleColorsClassic();     
                imm.imgui.io->FontGlobalScale = 1.22f;
                imm->disengage_face_culling();   
                return RGH_OK;
            },
            .loop_cb    = [ this ] ( const auto& args_ ) -> auto { 
                return this->guix_frame( args_ );
            },
            .exit_cb    = [ this ] ( const auto& args_ ) -> auto { 
                this->signal_stop();
                return RGH_OK;
            }
        } );

        auto docks = _docks.watch();
        for( auto& [ id, dock ] : *docks ) {
            dock->wake( {
                .argc = argc_,
                .argv = argv_
            } );
        }
        docks.release();

        logger->info( "bridge: started." );
        return RGH_OK;
    }

    void signal_stop( void ) {
        _status.store( RGH_ERR_TERMINATED, memory_order_release );
        _status.notify_all();
        logger->info( "bridge: signaling stop..." );
    }

    void stop( void ) {
        this->signal_stop();
        if( _th_guix.joinable() ) _th_guix.join();
        logger->info( "bridge: stopped." );
    }

public:
    status_t proxy_pass( 
        MDN_IN    string_view     proxy_name_, 
        MDN_IN    const string&   command_, 
        MDN_OUT   string*         out_
    ) {
        auto proxys = _proxys.control();

        auto itr = proxys->find( proxy_name_ );

        MDN_ASSERT_OR( itr != proxys->end() ) {
            proxys.release();
            *out_ += format( "proxy pass: no such proxy: \"{}\".", proxy_name_ );
            return RGH_ERR_BADARG;
        }

        auto proxy = itr->second;
        proxys.release();

        return proxy->pass( command_, out_ );
    }

public:
    MDN_DOCK_GUIX_FNC {
        RGH_ASSERT_OR( _status.load( memory_order_relaxed ) == RGH_OK ) return RGH_ERR_TERMINATED;

        imm.assets_idle_splash_render( args_ );

        auto docks = _docks.control();
        int crt = 0;
        erase_if( *docks, [ &args_, &crt ] ( auto& itr_ ) -> bool {
            ImGui::PushID( crt++ );
            const bool erase = RGH_OK != itr_.second->guix_frame( args_ );
            ImGui::PopID();
            return erase;
        } );

        return RGH_OK;
    }

}; inline _bridge_t BridgE;
