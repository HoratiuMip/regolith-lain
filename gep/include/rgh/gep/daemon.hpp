#pragma once
/**
 * @file: gep/daemon.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */
#include <rgh/gep/core.hpp>
#include <rgh/gep/dispenser.hpp>

namespace rgh {

class Daemon {
_RGH_PROTECTED:
    friend class Daemon_cluster;

public:
    enum State_ {
        State_STOPPED, State_STARTED, State_STOPPING, State_STARTING
    };

_RGH_PROTECTED:
    std::atomic< State_ >           _daemon_state   = { State_STOPPED };
    std::vector< HVec< Daemon > >   _daemon_deps    = {};

_RGH_PROTECTED:
    virtual status_t _daemon_start( void* ctx_ ) = 0;
    virtual status_t _daemon_stop( void* ctx_ ) = 0;

public:
    virtual std::string_view daemon_name( void ) const = 0;
    virtual std::string daemon_report( void* arg_ = nullptr ) const = 0;

public:
    RGH_inline State_ daemon_state( std::memory_order mo_ = std::memory_order_relaxed ) const {
        return _daemon_state.load( mo_ );
    }

public:
    status_t daemon_set_deps( decltype( _daemon_deps ) deps_ ) {
        _daemon_deps = std::move( deps_ );
        return RGH_OK;
    } 

    auto daemon_dep_count( void ) const {
        return _daemon_deps.size();
    }

    bool daemon_dep_of( std::string_view dmn_name_ ) const {
        for( const auto& dep : _daemon_deps ) {
            if( dep->daemon_name() == dmn_name_ ) return true;
            if( dep->daemon_dep_of( dmn_name_ ) ) return true;
        }
        return false;
    }

    RGH_inline bool daemon_dep_of( const Daemon& dmn_ ) const {
        return this->daemon_dep_of( dmn_.daemon_name() );
    }

public:
    status_t daemon_start( void* ctx_ = nullptr ) {
        State_ es = State_STOPPED;
        RGH_ASSERT_OR( _daemon_state.compare_exchange_strong( es, State_STARTING, std::memory_order_release ) ) {
            return RGH_ERR_WOULD_OVRWR;
        }
      
        RGH_ASSERT_STATUS_OR( this->_daemon_start( ctx_ ) ) {
            _daemon_state.store( State_STOPPING, std::memory_order_seq_cst );
            this->_daemon_stop( ctx_ );
            _daemon_state.store( State_STOPPED, std::memory_order_release );
            return status_;
        }
        _daemon_state.store( State_STARTED, std::memory_order_release );
        return RGH_OK;
    }

    status_t daemon_stop( void* ctx_ = nullptr ) {
        State_ es = State_STARTED;
        RGH_ASSERT_OR( _daemon_state.compare_exchange_strong( es, State_STOPPING, std::memory_order_release ) ) {
            return RGH_ERR_WOULD_OVRWR;
        }

        status_t status = this->_daemon_stop( ctx_ );
        _daemon_state.store( State_STOPPED, std::memory_order_release );
        return status;
    }

    virtual status_t daemon_restart( void* ctx_ = nullptr ) {
        (void)this->daemon_stop( ctx_ );
        return this->daemon_start( ctx_ );
    }

public:
    RGH_inline bool daemon_is_positive( void ) {
        State_ state = this->daemon_state();
        return State_STARTED == state || State_STARTING == state;
    }

    RGH_inline bool daemon_is_started( void ) {
        return State_STARTED == this->daemon_state();
    }

    RGH_inline bool daemon_is_stable( void ) {
        State_ state = this->daemon_state();
        return state != State_STOPPING and state != State_STARTING;
    }
};

class Daemon_cluster {
public:
    enum StateCtl_ {
        StateCtl_None,
        StateCtl_KeepAlive
    };

    typedef   std::function< void( HVec< Daemon > ) >   crit_fnc_t;

    struct rst_if_args_t {
        int   attempt   = 0;
    };

    typedef   std::function< status_t( const rst_if_args_t& ) >   rst_if_fnc_t;

    struct dock_t {
        HVec< Daemon >                  ref           = nullptr;

        uint8_t                         state_ctl     = StateCtl_None;

        rst_if_fnc_t                    rst_if        = nullptr;
        int                             crit_n_rsts   = -1;

        void*                           ctx           = nullptr;

        mutable int                     _bad_rst_cnt  = 0;
    };

_RGH_PROTECTED:
    struct _dock_compare_t {
        bool operator () ( const dock_t& lhs_, const dock_t& rhs_ ) const {   
            if( lhs_.ref->daemon_dep_of( rhs_.ref ) ) return false;
            if( rhs_.ref->daemon_dep_of( lhs_.ref ) ) return true;

            return lhs_.ref->daemon_name() < rhs_.ref->daemon_name();
        }
    };

_RGH_PROTECTED:
    Dispenser< std::set< dock_t, _dock_compare_t > >  _register    = { DispenserMode_Lock };

    struct _crit_t{
        std::atomic_bool   flag   = { false };
        crit_fnc_t         fnc    = {};
    }                                                  _crit       = {};                                        

public:
    RGH_inline void when_critical( const crit_fnc_t& crit_fnc_ ) { _crit.fnc = crit_fnc_; }

    void critical_handler( HVec< Daemon > cmpd_ ) {
        bool exflag = false;
        RGH_ASSERT_OR( _crit.flag.compare_exchange_strong( exflag, true, std::memory_order_seq_cst ) ) return;
        if( _crit.fnc ) this->_crit.fnc( std::move( cmpd_ ) );
    }

    RGH_inline bool went_critical( void ) { return _crit.flag.load( std::memory_order_relaxed ); }

public:
    status_t push( dock_t dock_ ) { 
        auto reg = _register.control();

        auto [ itr, inserted ] = reg->emplace( std::move( dock_ ) );

        ASSERT_OR( inserted ) return RGH_ERR_WOULD_OVRWR;
        return RGH_OK;
    }

    status_t pop( std::string_view name_ ) {
        auto reg = _register.control();

        return std::erase_if( *reg, [ &name_ ] ( const dock_t& dock_ ) -> bool {
            return name_ == dock_.ref->daemon_name();
        } ) > 0 ? RGH_OK : RGH_ERR_NOT_FOUND;
    }

public:
    status_t iterate_register( void ) {
        auto reg = _register.watch();

        for( auto& dock : *reg ) {
            RGH_ASSERT_OR( dock.ref->daemon_is_stable() ) continue;

            int up_deps = 0; for( const auto& dep : dock.ref->_daemon_deps ) up_deps += dep->daemon_is_started();
            RGH_ASSERT_OR( up_deps == dock.ref->daemon_dep_count() ) continue;
            
            const bool needs_restart = 
                ( ( dock.state_ctl & StateCtl_KeepAlive ) && not dock.ref->daemon_is_positive() )
                ||
                ( RGH_OK != dock.rst_if( {
                    .attempt = dock._bad_rst_cnt
                } ) );

            if( needs_restart ) {
                RGH_ASSERT_STATUS_OR( dock.ref->daemon_restart( dock.ctx ) ) {
                    ++dock._bad_rst_cnt;

                    if( dock.crit_n_rsts != -1 && dock._bad_rst_cnt >= dock.crit_n_rsts ) {
                        this->critical_handler( *dock.ref );
                        return RGH_ERR_TERMINATED; 
                    }
                } else {
                    dock._bad_rst_cnt = 0;
                }
            }
        }
        return RGH_OK;
    }

};

};