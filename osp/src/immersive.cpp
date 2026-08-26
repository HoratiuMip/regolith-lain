/*
# FILE: osp/immersive.cpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Implementation file.
*/
#define STB_IMAGE_IMPLEMENTATION
#include <rgh/osp/immersive.hpp>
#ifdef RGH_DEPCOM_ELIGIBLE_IMMERSIVE

namespace rgh {

#pragma region ASSETS
// void Immersive::_init_assets( void ) {
// /* === idle_splash === */ {
//     auto& idle_splash = _assets.idle_splash;

//     glGenVertexArrays( 1, &idle_splash.VAO );
//     glGenBuffers     ( 1, &idle_splash.VBO );
//     glGenBuffers     ( 1, &idle_splash.EBO );

//     glBindVertexArray( idle_splash.VAO );

//     glBindBuffer( GL_ARRAY_BUFFER, idle_splash.VBO );
//     glBufferData( GL_ARRAY_BUFFER, sizeof( idle_splash.vrtx ), idle_splash.vrtx, GL_STATIC_DRAW );

//     glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, idle_splash.EBO );
//     glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( idle_splash.idx ), idle_splash.idx, GL_STATIC_DRAW );

//     glVertexAttribPointer    ( 0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0 );
//     glEnableVertexAttribArray( 0 );

//     glBindBuffer     ( GL_ARRAY_BUFFER, GL_NONE );
//     glBindVertexArray( GL_NONE );

//     /* Fragment shader by XorDev: https://x.com/XorDev/status/1894123951401378051. */
//     const char* shaders[ 5 ] = {
//         R"(
//             #version 410 core
//             //RGH#strid<rgh/immersive/idle_splash.vert>

//             layout ( location = 0 ) in vec2 vrtx;
//             out vec2 uv;
//             void main() {
//                 gl_Position = vec4( vrtx, 0.0, 1.0 );
//                 uv = vrtx.xy;
//             }
//         )",
//         nullptr, nullptr, nullptr,
//         R"(
//             #version 410 core
//             //RGH#strid<rgh/immersive/idle_splash.frag>

//             uniform float rtc;
//             uniform ivec2 res;
//             in      vec2  uv;
//             out     vec4  frag;

//             void main() {
//                 frag = vec4(0);
//                 vec2 p=uv*res/res.y/1.16,l=vec2(0),v=p*(1.-(l+=abs(.7-dot(p,p))))/.2;
//                 for(float i=0.0;i++<8.;frag+=(sin(v.xyyx)+1.)*abs(v.x-v.y)*.2)v+=cos(v.yx*i+vec2(0,i)+rtc)/i+.7;
//                 frag=vec4(tanh(exp(p.y*vec4(1,-1,-2,0))*exp(-4.*l.x)/frag).xyz,1.0);
//                 frag.rg = vec2( frag.g*frag.g, frag.r*frag.r );
//             }
//         )"
//     };

//     idle_splash.pipe = _pipe_cache.make_pipe_from_sources( shaders );
// }
// }

// void Immersive::_clean_assets( void ) {
// /* === idle_splash === */ {
//     auto& idle_splash = _assets.idle_splash;

//     if( idle_splash.VAO != GL_NONE ) glDeleteVertexArrays( 1, &idle_splash.VAO );
//     if( idle_splash.VBO != GL_NONE ) glDeleteBuffers     ( 1, &idle_splash.VBO );
//     if( idle_splash.EBO != GL_NONE ) glDeleteBuffers     ( 1, &idle_splash.EBO );

//     idle_splash.pipe.reset();
// }
// }

// status_t Immersive::assets_idle_splash_render( const frame_cb_args_t& args_ ) {
//     this->disengage_depth_test();
//     this->mode_fill();

//     _assets.idle_splash.pipe->use_program();
//     _assets.idle_splash.pipe->upload_unif( "rtc", (float)args_.t );
//     _assets.idle_splash.pipe->upload_unif( "res", this->top_ri() );

//     glBindVertexArray( _assets.idle_splash.VAO );
//     glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

//     this->engage_depth_test();
//     return RGH_OK;
// }
#pragma endregion ASSETS

status_t Immersive::main( 
    RGH_IN   int               argc_, 
    RGH_IN   char*             argv_[], 
    RGH_IN   const config_t&   config_ 
) {
    _config = std::move( config_ );
    RGH_BRDG_LOGI( "imm: starting main loop..." );

    glfwInit();
    glewInit();

    glfwSetErrorCallback( [] ( int err_, const char* desc_ ) static -> void {
       RGH_BRDG_LOGE( "imm: glfw says [{}]: \"{}\".", err_, desc_ );
    } );

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_RESIZABLE, GL_TRUE );
    glfwWindowHint( GLFW_DECORATED, GL_TRUE );

    if( Maximize == _config.srf_bgn_as ) glfwWindowHint( GLFW_MAXIMIZED, GL_TRUE );
    
    _glfwnd = glfwCreateWindow( _config.width, _config.height, _config.title, nullptr, nullptr );

    RGH_ASSERT_OR( _glfwnd ) { RGH_BRDG_LOGE( "imm: bad window handle." ); return RGH_ERR_EXCOMCALL; }
    RGH_BRDG_LOGI( "imm: window object created." );

    glfwMakeContextCurrent( _glfwnd );

    _rend_str = ( const char* )glGetString( GL_RENDERER ); 
    _gl_str   = ( const char* )glGetString( GL_VERSION );

    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );

    glFrontFace( GL_CCW );

    glCullFace( GL_BACK );
    glEnable( GL_CULL_FACE ); 

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_BLEND );

    glewExperimental = GL_TRUE; 
    glewInit();

    int wnd_w, wnd_h;
    glfwGetFramebufferSize( _glfwnd, &wnd_w, &wnd_h );
    glViewport( 0, 0, wnd_w, wnd_h );

    BridgE->info( "imm: docked on {} @ {}.", _rend_str ? _rend_str : "NULL", _gl_str ? _gl_str : "NULL" );

    glfwSetWindowUserPointer( _glfwnd, (void*)this );

    glfwSetFramebufferSizeCallback( _glfwnd, [] ( GLFWwindow* wnd_, int w_, int h_ ) static -> void {
        glViewport( 0, 0, w_, h_ );
    } );

    glfwSetDropCallback( _glfwnd, [] ( GLFWwindow* window_, int filc_, const char* filv_[] ) {
        auto* self = ( Immersive* )glfwGetWindowUserPointer( window_ );
        self->_dnd_files.control()->assign( filv_, filv_+filc_ );
    });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL( _glfwnd, true );
    ImGui_ImplOpenGL3_Init();

    imgui.io  = &ImGui::GetIO();
    imgui.stl = &ImGui::GetStyle();

    RGH_BRDG_LOGI( "imm: imgui component started." );
    
    if     ( Iconify == _config.srf_bgn_as ) glfwIconifyWindow( _glfwnd );
    else if( Hide    == _config.srf_bgn_as ) glfwHideWindow( _glfwnd );

    //this->_init_assets();
    RGH_BRDG_LOGI( "imm: assets initialization complete." );

    RGH_ASSERT_AND( _config.init_cb ) {
        RGH_BRDG_LOGI( "imm: invoking init callback..." );
        RGH_ASSERT_OR( RGH_OK == this->_config.init_cb( init_cb_args_t{
            .ctx = _config.ctx
        } ) ) {
            RGH_BRDG_LOGE( "imm: aborted by init callback." );
            return RGH_ERR_USERCALL;
        } else {
            RGH_BRDG_LOGI( "imm: init callback done." );
        }
    } else {
        RGH_BRDG_LOGI( "imm: no init callback to invoke." );
    }
    
    RGH_BRDG_LOGI( "imm: ready for loop..." );

    glViewport( 0, 0, _config.width, _config.height );

    _running.store( true, std::memory_order_relaxed );
    while( _running.load( std::memory_order_relaxed ) && !glfwWindowShouldClose( _glfwnd ) ) {
        this->_resolve_payloads();

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        const auto elapsed_s = glfwGetTime();
        RGH_ASSERT_OR( RGH_OK == this->_config.loop_cb( frame_cb_args_t{
            .ctx         = _config.ctx,
            .t           = elapsed_s,
            .dt          = imgui.io->DeltaTime,
            .blink_500ms = static_cast< int >( elapsed_s*1000.0 ) % 1000 <= 500
        } ) ) _running.store( false, std::memory_order_relaxed );

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        glfwMakeContextCurrent( _glfwnd );
        glfwSwapBuffers( _glfwnd );
    }

l_end:
    _running.store( false, std::memory_order_relaxed );

    RGH_BRDG_LOGI( "imm: shutting down..." );

    RGH_ASSERT_AND( _config.exit_cb ) _config.exit_cb( {
        .ctx = _config.ctx
    } );

    //this->_clean_assets();
    RGH_BRDG_LOGI( "imm: assets cleaned." );

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow( std::exchange( _glfwnd, nullptr ) );
    glfwTerminate();

    RGH_BRDG_LOGI( "imm: shutdown." );
    return RGH_OK;
}

}

#endif