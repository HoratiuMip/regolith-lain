#pragma once
/**
 * @file: osp/thread_pool.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/osp/core.hpp>

namespace rgh {


class Thread_pool {
public:
    typedef   std::function< void( void ) >   task_t;

public:
    Thread_pool( void ) = default;

    ~Thread_pool( void ) {
        this->kill();
    }

_RGH_PROTECTED:
    std::vector< std::jthread >   _threads   = {};

    std::queue< task_t >          _tasks   = {};
    std::atomic_int               _count   = { 0x0 };
    std::mutex                    _mtx     = {};

public:
    status_t push( const task_t& tsk_ ) noexcept _rgh_try {
        std::unique_lock lck{ _mtx }; _tasks.emplace( tsk_ );
        _count.fetch_add( 1, std::memory_order_release ); _count.notify_one();
        return RGH_OK;
    } _rgh_catch( {
        return RGH_ERR_BADALLOC;
    } );

public:
    status_t launch( int th_cnt_ ) noexcept _rgh_try {
        RGH_ASSERT_OR( _threads.empty() ) return RGH_ERR_LOGIC;

        _threads.reserve( th_cnt_ );
        for( int n = 1; n <= th_cnt_; ++n ) {
            _threads.emplace_back( [ this ] ( void ) -> void { for(;;) {
                _count.wait( 0x0 ); 
                RGH_ASSERT_OR( _count.load( std::memory_order_relaxed ) > 0x0 ) return;
                _count.fetch_sub( 1, std::memory_order_relaxed );

                std::unique_lock lck{ _mtx };
                RGH_ASSERT_OR( not _tasks.empty() ) return;

                auto task = std::move( _tasks.front() ); _tasks.pop();
                lck.unlock();

                task();
            } } );
        }
        return RGH_OK;
    } _rgh_catch( { 
        return RGH_ERR_SYSCALL;
    } ); 

public:
    void kill( void ) {
        RGH_ASSERT_OR( not _threads.empty() ) return;

        _count.store( -0x1, std::memory_order_release );
        _count.notify_all();
        _threads.clear();
    }

};


};
