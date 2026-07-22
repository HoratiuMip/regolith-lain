/**
 * @file: osp/hyper_net.cpp
 * @brief: Implementation file.
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/osp/immersive.hpp>
#ifdef RGH_DEPCOM_ELIGIBLE_IMMERSIVE

namespace rgh {

void Immersive::_init_assets( void ) {
/* === idle_splash === */ {
    auto& idle_splash = _assets.idle_splash;

    glGenVertexArrays( 1, &idle_splash.VAO );
    glGenBuffers     ( 1, &idle_splash.VBO );
    glGenBuffers     ( 1, &idle_splash.EBO );

    glBindVertexArray( idle_splash.VAO );

    glBindBuffer( GL_ARRAY_BUFFER, idle_splash.VBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( idle_splash.vrtx ), idle_splash.vrtx, GL_STATIC_DRAW );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, idle_splash.EBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( idle_splash.idx ), idle_splash.idx, GL_STATIC_DRAW );

    glVertexAttribPointer    ( 0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0 );
    glEnableVertexAttribArray( 0 );

    glBindBuffer     ( GL_ARRAY_BUFFER, GL_NONE );
    glBindVertexArray( GL_NONE );

    /* Fragment shader by XorDev: https://x.com/XorDev/status/1894123951401378051. */
    const char* shaders[ 5 ] = {
        R"(
            #version 410 core
            //RGH#strid<rgh/immersive/idle_splash.vert>

            layout ( location = 0 ) in vec2 vrtx;
            out vec2 uv;
            void main() {
                gl_Position = vec4( vrtx, 0.0, 1.0 );
                uv = vrtx.xy;
            }
        )",
        nullptr, nullptr, nullptr,
        R"(
            #version 410 core
            //RGH#strid<rgh/immersive/idle_splash.frag>

            uniform float rtc;
            uniform ivec2 res;
            in      vec2  uv;
            out     vec4  frag;

            void main() {
                frag = vec4(0);
                vec2 p=uv*res/res.y/1.16,l=vec2(0),v=p*(1.-(l+=abs(.7-dot(p,p))))/.2;
                for(float i=0.0;i++<8.;frag+=(sin(v.xyyx)+1.)*abs(v.x-v.y)*.2)v+=cos(v.yx*i+vec2(0,i)+rtc)/i+.7;
                frag=vec4(tanh(exp(p.y*vec4(1,-1,-2,0))*exp(-4.*l.x)/frag).xyz,1.0);
                frag.rg = vec2( frag.g*frag.g, frag.r*frag.r );
            }
        )"
    };

    idle_splash.pipe = _cluster->pipe_handler().make_pipe_from_sources( shaders );
}
}

void Immersive::_clean_assets( void ) {
/* === idle_splash === */ {
    auto& idle_splash = _assets.idle_splash;

    if( idle_splash.VAO != GL_NONE ) glDeleteVertexArrays( 1, &idle_splash.VAO );
    if( idle_splash.VBO != GL_NONE ) glDeleteBuffers     ( 1, &idle_splash.VBO );
    if( idle_splash.EBO != GL_NONE ) glDeleteBuffers     ( 1, &idle_splash.EBO );

    idle_splash.pipe.reset();
}
}

status_t Immersive::assets_idle_splash_render( const frame_cb_args_t& args_ ) {
    _cluster->disengage_depth_test();
    _cluster->mode_fill();

    _assets.idle_splash.pipe->use_program();
    _assets.idle_splash.pipe->upload_unif( "rtc", (float)args_.t );
    _assets.idle_splash.pipe->upload_unif( "res", _cluster->top_ri() );

    glBindVertexArray( _assets.idle_splash.VAO );
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

    _cluster->engage_depth_test();
    return RGH_OK;
}

status_t Immersive::main( int argc_, char* argv_[], const config_t& config_ ) {
    config = config_;

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
    if( SrfBeginAs_Maximize == config.srf_bgn_as ) glfwWindowHint( GLFW_MAXIMIZED, GL_TRUE );
    
    GLFWwindow* window = glfwCreateWindow( config.width, config.height, config.title, nullptr, nullptr );

    RGH_ASSERT_OR( window ) { RGH_BRDG_LOGE( "imm: bad window handle." ); return RGH_ERR_EXCOMCALL; }
    RGH_BRDG_LOGI( "imm: window object created." );

    glfwMakeContextCurrent( window );
    
    if( not _cluster ) _cluster = HVec< imm::Cluster >::make( imm::Cluster::init_args_t{
        .glfwnd = window
    } );

    glfwSetWindowUserPointer( _cluster->handle(), (void*)this );

    glfwSetFramebufferSizeCallback( _cluster->handle(), [] ( GLFWwindow* wnd_, int w_, int h_ ) static -> void {
        glViewport( 0, 0, w_, h_ );
    } );

    glfwSetDropCallback( window, [] ( GLFWwindow* window_, int filc_, const char* filv_[] ) {
        auto* self = ( Immersive* )glfwGetWindowUserPointer( window_ );
        self->_dnd_files.control()->assign( filv_, filv_+filc_ );
    });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL( window, true );
    ImGui_ImplOpenGL3_Init();

    imgui.io  = &ImGui::GetIO();
    imgui.stl = &ImGui::GetStyle();

    RGH_BRDG_LOGI( "imm: imgui component started." );
    
    if     ( SrfBeginAs_Iconify == config.srf_bgn_as ) glfwIconifyWindow( window );
    else if( SrfBeginAs_Hide    == config.srf_bgn_as ) glfwHideWindow( window );

    this->_init_assets();
    RGH_BRDG_LOGI( "imm: assets initialization complete." );

    if( config.init_cb ) {
        RGH_BRDG_LOGI( "imm: invoke init callback..." );
        RGH_ASSERT_OR( RGH_OK == this->config.init_cb( init_cb_args_t{
            .ctx = config.ctx
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

    glViewport( 0, 0, config.width, config.height );

    _is_running.store( true, std::memory_order_release );
    while( _is_running.load( std::memory_order_relaxed ) && !glfwWindowShouldClose( window ) ) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        RGH_ASSERT_OR( RGH_OK == this->config.loop_cb( frame_cb_args_t{
            .ctx = config.ctx,
            .t   = glfwGetTime(),
            .dt  = imgui.io->DeltaTime
        } ) ) _is_running.store( false, std::memory_order_seq_cst );

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent( window );

        _cluster->swap();
    }

l_end:
    _is_running.store( false, std::memory_order_seq_cst );

    RGH_BRDG_LOGI( "imm: shutting down..." );

    if( config.exit_cb ) config.exit_cb( {
        .ctx = config.ctx
    } );

    this->_clean_assets();
    RGH_BRDG_LOGI( "imm: assets cleaned." );

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow( window );

    _cluster.reset();
    glfwTerminate();

    RGH_BRDG_LOGI( "imm: shutdown complete." );
    return RGH_OK;
}

}

#endif