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
    std::vector< HVec< Daemon > >   _deps           = {};

_RGH_PROTECTED:
    virtual status_t _daemon_start( void* ctx_ ) = 0;

    virtual status_t _daemon_stop( void* ctx_ ) = 0;

public:
    virtual std::string_view daemon_name( void ) const = 0;

    bool daemon_dep_of( const Daemon* dmn_ ) const {
        for( const auto& dep : _deps ) {
            if( dep->daemon_name() == dmn_->daemon_name() ) return true;
            if( dep->daemon_dep_of( dmn_ ) ) return true;
        }
        return false;
    }

public:
    status_t daemon_start( void* ctxu_ = nullptr, void* ctxud_ = nullptr ) {
        State_ es = State_STOPPED;
        RGH_ASSERT_OR( _daemon_state.compare_exchange_strong( es, State_STARTING, std::memory_order_release ) ) {
            return RGH_ERR_WOULD_OVRWR;
        }
      
        status_t status = this->_daemon_start( ctxu_ );
        RGH_ASSERT_OR( RGH_OK == status ) {
            this->_daemon_stop( ctxud_ );
            _daemon_state.store( State_STOPPED, std::memory_order_release );
            return status;
        }
        _daemon_state.store( State_STARTED, std::memory_order_release );
        return RGH_OK;
    }

    status_t daemon_stop( void* ctxd_ = nullptr ) {
        State_ es = State_STARTED;
        RGH_ASSERT_OR( _daemon_state.compare_exchange_strong( es, State_STOPPING, std::memory_order_release ) ) {
            return RGH_ERR_WOULD_OVRWR;
        }

        status_t status = this->_daemon_stop( ctxd_ );
        RGH_ASSERT_OR( RGH_OK == status ) {
            _daemon_state.store( State_STOPPED, std::memory_order_release );
            return status;
        }
        _daemon_state.store( State_STOPPED, std::memory_order_release );
        return RGH_OK;
    }

    virtual status_t daemon_restart( void* ctxd_ = nullptr, void* ctxu_ = nullptr, void* ctxud_ = nullptr ) {
        (void)this->daemon_stop( ctxd_ );
        return this->daemon_start( ctxu_, ctxud_ );
    }

public:
    RGH_inline bool daemon_is_up( void ) {
        State_ state = _daemon_state.load( std::memory_order_relaxed );
        return State_STARTED == state || State_STARTING == state;
    }

    RGH_inline bool daemon_is_stable( void ) {
        State_ state = _daemon_state.load( std::memory_order_relaxed );
        return state != State_STOPPING and state != State_STARTING;
    }
};

class Daemon_cluster {
public:
    typedef   std::function< void( HVec< Daemon > ) >   critical_fnc_t;

public:
    struct restart_if_args_t {
        int   attempt   = 0;
    };

    struct entry_t {
        typedef   std::function< status_t( Daemon&, const restart_if_args_t& ) >   restart_if_fnc_t;

        HVec< Daemon >                  ref                   = nullptr;
        std::vector< HVec< Daemon > >   deps                  = {};
        bool                            keep_alive            = false;
        restart_if_fnc_t                restart_if            = nullptr;
        int                             critical_n_restarts   = -1;
        void*                           ctxu                  = nullptr;
        void*                           ctxd                  = nullptr;
        void*                           ctxud                 = nullptr;

        mutable int                     _failed_restart_cnt   = 0;
    };

_RGH_PROTECTED:
    struct _entry_compare_t {
        bool operator () ( const entry_t& lhs_, const entry_t& rhs_ ) const {   
            if( lhs_.ref->daemon_dep_of( rhs_.ref.get() ) ) return false;
            if( rhs_.ref->daemon_dep_of( lhs_.ref.get() ) ) return true;

            return lhs_.ref->daemon_name() < rhs_.ref->daemon_name();
        }
    };

_RGH_PROTECTED:
    Dispenser< std::set< entry_t, _entry_compare_t > >  _register    = { DispenserMode_Lock };

    std::atomic_bool                                    _crit_flag   = { false };
    critical_fnc_t                                      _crit_fnc    = {};

public:
    void when_critical( const critical_fnc_t& crit_fnc_ ) { _crit_fnc = crit_fnc_; }

    void critical_handler( HVec< Daemon > cmpd_ ) {
        bool exflag = false;
        RGH_ASSERT_OR( _crit_flag.compare_exchange_strong( exflag, true, std::memory_order_seq_cst ) ) return;
        if( _crit_fnc ) this->_crit_fnc( std::move( cmpd_ ) );
    }

    RGH_inline bool went_critical( void ) { return _crit_flag.load( std::memory_order_relaxed ); }

public:
    status_t push( entry_t entry_ ) {
        entry_.ref->_deps = move( entry_.deps );
        
        auto reg = _register.control();

        auto [ itr, inserted ] = reg->emplace( std::move( entry_ ) );
        ASSERT_OR( inserted ) return RGH_ERR_WOULD_OVRWR;

        return RGH_OK;
    }

    status_t pop( std::string_view name_ ) {
        auto reg = _register.control();
        return std::erase_if( *reg, [ &name_ ] ( const entry_t& entry_ ) -> bool {
            return name_ == entry_.ref->daemon_name();
        } ) > 0 ? RGH_OK : RGH_ERR_NOT_FOUND;
    }

public:
    status_t iterate_register( void ) {
        auto reg = _register.watch();

        for( auto& entry : *reg ) {
            RGH_ASSERT_OR( entry.ref->daemon_is_stable() ) continue;

            int up_deps = 0;
            for( auto& dep : entry.deps ) up_deps += dep->daemon_is_up();
            RGH_ASSERT_OR( up_deps == entry.deps.size() ) continue;
            
            const bool needs_restart = 
                ( entry.keep_alive && not entry.ref->daemon_is_up() )
                ||
                ( RGH_OK != entry.restart_if( *entry.ref, {
                    .attempt = entry._failed_restart_cnt
                } ) );

            if( needs_restart ) {
                RGH_ASSERT_OR( RGH_OK == entry.ref->daemon_restart( entry.ctxd, entry.ctxu, entry.ctxud ) ) {
                    ++entry._failed_restart_cnt;

                    if( entry.critical_n_restarts != -1 && entry._failed_restart_cnt >= entry.critical_n_restarts ) {
                        this->critical_handler( *entry.ref );
                        return RGH_ERR_TERMINATED; 
                    }
                } else {
                    entry._failed_restart_cnt = 0;
                }
            }
        }
        return RGH_OK;
    }

};

};