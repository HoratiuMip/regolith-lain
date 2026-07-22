#pragma once
/**
 * @file: osp/immersive.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */
#include <rgh/osp/render3.hpp>

#ifdef RGH_DEPCOM_ELIGIBLE_RENDER3
    #define RGH_DEPCOM_ELIGIBLE_IMMERSIVE
#endif

#ifdef RGH_DEPCOM_ELIGIBLE_IMMERSIVE

#include <rgh/gep/dispenser.hpp>
#include <rgh/osp/tempo.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <ImGuiFileDialog.h>
#include <imgui-knobs.h>

namespace rgh {

class Immersive  {
public:
    struct frame_cb_args_t {
        void*    ctx;
        double   t;
        double   dt;
    };
    struct init_cb_args_t {
        void*   ctx;
    };
    struct exit_cb_args_t {
        void*   ctx;
    };

public:
    typedef   std::function< status_t( const frame_cb_args_t& ) >   frame_callback_t;
    typedef   std::function< status_t( const init_cb_args_t&  ) >   init_callback_t;   
    typedef   std::function< void( const exit_cb_args_t& ) >        exit_callback_t;       

public:
    enum SrfBeginAs_ {
        SrfBeginAs_Default, SrfBeginAs_Iconify, SrfBeginAs_Maximize, SrfBeginAs_Hide
    };

public:
    struct config_t {
        void*                   ctx           = nullptr;

        const char*             title         = RGH_VERSION_STRING"/Immersive";
        int                     width         = 64;
        int                     height        = 64;

        SrfBeginAs_             srf_bgn_as    = SrfBeginAs_Default;

        std::filesystem::path   icon_path     = {};

        init_callback_t         init_cb       = nullptr;
        frame_callback_t        loop_cb       = nullptr;
        exit_callback_t         exit_cb       = nullptr;

    } config;

public:
    HVec< imm::Cluster >     _cluster      = nullptr;
    imm::lens_t              _lens_0       = { glm::vec3{0,0,5}, glm::vec3{0,0,0}, glm::vec3{0,1,0} };

_RGH_PROTECTED:
    std::atomic_bool         _is_running   = false;

_RGH_PROTECTED:
    Dispenser< std::vector< std::string > >   _dnd_files   = { DispenserMode_Lock };
public:
    bool has_dropped_files( void ) const { return not _dnd_files.watch()->empty(); }
    auto flush_dropped_files( void ) { return std::move( *_dnd_files.control() ); }

public:
    struct imgui_t {
        ImGuiIO*      io    = nullptr;
        ImGuiStyle*   stl   = nullptr;
    } imgui;

_RGH_PROTECTED:
    struct _assets_t {
        /* A perlin noise splasher by XorDev: https://x.com/XorDev/status/1894123951401378051. */
        struct idle_splash_t {
            inline static float          vrtx[]   = { 1,1, 1,-1, -1,-1, -1,1 };
            inline static unsigned int   idx[]    = { 0,1,3, 1,2,3 };
            GLuint                       VAO      = GL_NONE;
            GLuint                       VBO      = GL_NONE;
            GLuint                       EBO      = GL_NONE;
            HVec< imm::pipe_t >          pipe     = nullptr;
        } idle_splash;
    } _assets;

    void _init_assets( void );
    void _clean_assets( void );

public:
    status_t assets_idle_splash_render( const frame_cb_args_t& args_ );

public:
    RGH_inline imm::Cluster& cluster( void ) {
        return *_cluster;
    }
    RGH_inline auto* operator -> ( void ) {
        return _cluster.get();
    }

    RGH_inline imm::lens_t lens_0( void ) {
        return _lens_0;
    }

    RGH_inline auto native_window_handle( void ) {
        return _cluster->handle();
    }

public:
    status_t set_icon( GLFWimage& img_ ) {
        glfwSetWindowIcon( _cluster->handle(), 1, &img_ );
        return RGH_OK;
    }

public:
    status_t main( int argc_, char* argv_[], const config_t& config_ );

    void sig_main_exit( void ) { RGH_ASSERT_AND( _cluster ) glfwSetWindowShouldClose( _cluster->handle(), GLFW_TRUE ); }

public:
    RGH_inline static void movx( float dx_ ) { ImGui::SetCursorPosX( ImGui::GetCursorPosX() + dx_ ); }
    RGH_inline static void movy( float dy_ ) { ImGui::SetCursorPosY( ImGui::GetCursorPosY() + dy_ ); }
    RGH_inline static void movxy( float dx_, float dy_ ) { movx( dx_ ); movy( dy_ ); }
    RGH_inline static void movxy( const ImVec2& dv_ ) { ImGui::SetCursorPos( dv_ ); }
    RGH_inline static void movxy( const ImVec2& dv_, const ImVec2& ddv_ ) { movxy( dv_ + ddv_ ); }

    RGH_inline static auto xycp( void ) { return ImGui::GetCursorPos(); }

    RGH_inline static void scale_font( float scl_ ) { ImGui::SetWindowFontScale( scl_ ); }

    RGH_inline static void scaled_text( float scl_, const char* fmt_, ... ) {
        auto prev_scl = ImGui::GetCurrentWindow()->FontWindowScale;
        scale_font( scl_ );
            va_list args; va_start( args, fmt_ ); ImGui::TextV( fmt_, args ); va_end( args );
        scale_font( prev_scl );
    }
    RGH_inline static void tabbed_text( const char* fmt_, ... ) {
        const float x = ImGui::GetCursorPosX();
        va_list args; va_start( args, fmt_ ); ImGui::TextV( fmt_, args ); va_end( args );
        ImGui::SetCursorPosX( x );
    }

    static std::string select_file_button( 
        RGH_IN_OPT   Immersive*              imm_,
        RGH_IN       const char*             label_,
        RGH_IN       const char*             key_id_,
        RGH_IN       const char*             title_,
        RGH_IN       const char*             format_,
        RGH_IN_OPT   ImVec2                  min_sz_   = { 400, 300 },
        RGH_IN_OPT   const char*             path_     = ".",
        RGH_IN_OPT   ImGuiFileDialogFlags_   flags_    = ImGuiFileDialogFlags_None
    ) {
        if( ImGui::Button( label_ ) ) {
            ImGuiFileDialog::Instance()->OpenDialog( key_id_, title_, format_, IGFD::FileDialogConfig{
                .path  = ".",
                .flags = flags_
            } );
        } else if( auto files_ = imm_->flush_dropped_files(); not files_.empty() ) {
            return files_.front();
        }
        if( ImGuiFileDialog::Instance()->Display( key_id_, ImGuiWindowFlags_NoCollapse, min_sz_ ) ) {
            std::string ret = {};
            ASSERT_AND( ImGuiFileDialog::Instance()->IsOk() ) ret = ImGuiFileDialog::Instance()->GetFilePathName();
            ImGuiFileDialog::Instance()->Close();
            return ret;
        }
        return {};
    }

};

}

#endif