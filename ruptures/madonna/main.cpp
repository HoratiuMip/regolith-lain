#include <bridge.hpp>
#include <csignal>

int main( int argc, char* argv[] ) {
    rgh::init( argc, argv, rgh::init_args_t{
        .flags = rgh::InitFlags_None
    } );

    std::signal( SIGINT, [] ( int sig_ ) static -> void {
        BridgE.signal_stop();
    } );

    BridgE.start( argc, argv );
    BridgE.wait_stop();

    return 0x0;
}