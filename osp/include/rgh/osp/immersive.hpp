#pragma once /*
# FILE: osp/immersive.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Graphics handler.
*/
#if defined( RGH_EXCOM_OPENGL ) && defined( RGH_EXCOM_STB ) && defined( RGH_EXCOM_TINYOBJ ) && defined( RGH_EXCOM_DEARIMGUI )
    #define RGH_DEPCOM_ELIGIBLE_IMMERSIVE
#endif

#ifdef RGH_DEPCOM_ELIGIBLE_IMMERSIVE

#include <rgh/gep/dispenser.hpp>
#include <rgh/gep/ringatomic.hpp>
#include <rgh/osp/core.hpp>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <ImGuiFileDialog.h>
#include <imgui-knobs.h>
#include <imspinner_dots.h>
#include <imspinner_bars.h>

#define _RGH_IMM_STORE_TEX_2D GLint prev_tex_ = GL_NONE; glGetIntegerv( GL_TEXTURE_BINDING_2D, &prev_tex_ );
#define _RGH_IMM_RESTORE_TEX_2D glBindTexture( GL_TEXTURE_2D, prev_tex_ );

namespace rgh { class Immersive; }

namespace rgh::imm {

class Tex { friend class rgh::Immersive;
public:
    struct params_t {
        GLuint   min_filter   = GL_LINEAR_MIPMAP_LINEAR;
        GLuint   mag_filter   = GL_LINEAR;
        bool     v_flip       = false;
        bool     mipmaps      = true;
    };

public:
    Tex( void ) = default;
    Tex( const Tex& ) = delete;

    Tex( 
        RGH_IN   Tex&&   other_ 
    ) : _glidx{ std::exchange( other_._glidx, GL_NONE ) } 
    {}

    ~Tex( void ) { this->deld(); }

public:
    status_t upld( 
        RGH_IN       GLFWimage         image_,
        RGH_IN_OPT   const params_t&   params_
    ) {
        RGH_ASSERT_OR( _glidx == GL_NONE ) return RGH_ERR_WOULD_OVRWR;

        glGenTextures( 1, &_glidx );
        RGH_ASSERT_OR( _glidx != GL_NONE ) return RGH_ERR_BADALLOC;

        _RGH_IMM_STORE_TEX_2D;
            glBindTexture( GL_TEXTURE_2D, _glidx );
            glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, image_.width, image_.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_.pixels );
            
            _mipmaps = params_.mipmaps;
            RGH_ASSERT_AND( _mipmaps && nullptr != image_.pixels ) glGenerateMipmap( GL_TEXTURE_2D );

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params_.min_filter );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params_.mag_filter );
        _RGH_IMM_RESTORE_TEX_2D;
        return RGH_OK;
    }

    status_t deld( 
        void 
    ) {
        RGH_ASSERT_OR( _glidx != GL_NONE ) return RGH_OK;

        glDeleteTextures( 1, &_glidx );
        _glidx = GL_NONE; 

        return RGH_OK;
    }

    status_t reld( 
        RGH_IN   GLFWimage   image_
    ) {
        _RGH_IMM_STORE_TEX_2D;
            glBindTexture( GL_TEXTURE_2D, _glidx );
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, image_.width, image_.height, GL_RGBA, GL_UNSIGNED_BYTE, image_.pixels );
            RGH_ASSERT_AND( _mipmaps && nullptr != image_.pixels ) glGenerateMipmap( GL_TEXTURE_2D );
        _RGH_IMM_RESTORE_TEX_2D;

        return RGH_OK;
    }

_RGH_PROTECTED:
    GLuint   _glidx     = GL_NONE;
    bool     _mipmaps   = false;

public:
    operator GLuint ( void ) { return _glidx; }
    GLuint get( void ) { return _glidx; }
};

class Ren_target { friend class rgh::Immersive;
public:
    Ren_target( void ) = default;

    Ren_target( 
        RGH_IN   glm::vec< 2, int >   r_ 
    ) 
    : _r{ r_ }
    {
        this->upld();
    }

    ~Ren_target( void ) { this->deld(); }

_RGH_PROTECTED:
    GLuint               _tex_glidx   = GL_NONE;
    GLuint               _fbo         = GL_NONE;
    GLuint               _rbo         = GL_NONE;
    glm::vec< 2, int >   _r           = {};

public:
    status_t upld( void ) {
        RGH_ASSERT_OR( _tex_glidx == GL_NONE ) return RGH_ERR_WOULD_OVRWR;
        RGH_ASSERT_OR( _r.x > 0 && _r.y > 0 ) return RGH_ERR_BADARG;

        _RGH_IMM_STORE_TEX_2D;
            glGenTextures  ( 1, &_tex_glidx );
            glBindTexture  ( GL_TEXTURE_2D, _tex_glidx );
            glTexImage2D   ( GL_TEXTURE_2D, 0, GL_RGBA, _r.x, _r.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        _RGH_IMM_RESTORE_TEX_2D;

        glGenFramebuffers( 1, &_fbo );
        glBindFramebuffer( GL_FRAMEBUFFER, _fbo );

        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tex_glidx, 0 );

        glGenRenderbuffers   ( 1, &_rbo );
        glBindRenderbuffer   ( GL_RENDERBUFFER, _rbo );
        glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _r.x, _r.y );
        glBindRenderbuffer   ( GL_RENDERBUFFER, GL_NONE );

        glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _rbo );
        glBindFramebuffer        ( GL_FRAMEBUFFER, GL_NONE );
    }

    status_t upld( 
        RGH_IN   glm::vec< 2, int >   r_ 
    ) {
        _r = r_; return this->upld();
    }

    status_t deld(
        void
    ) {
        RGH_ASSERT_AND( GL_NONE != _tex_glidx ) { glDeleteTextures( 1, &_tex_glidx ); _tex_glidx = GL_NONE; }                   
        RGH_ASSERT_AND( GL_NONE != _fbo )       { glDeleteFramebuffers( 1, &_fbo );   _fbo       = GL_NONE; }
        RGH_ASSERT_AND( GL_NONE != _rbo )       { glDeleteRenderbuffers( 1, &_rbo );  _rbo       = GL_NONE; }

        return RGH_OK;
    }
};

}

namespace rgh {

class Immersive {
public:
    enum Word_ {
        Default, Iconify, Maximize, Hide
    };

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
    typedef   std::function< status_t( const frame_cb_args_t& ) >   frame_callback_t;
    typedef   std::function< status_t( const init_cb_args_t&  ) >   init_callback_t;   
    typedef   std::function< void( const exit_cb_args_t& ) >        exit_callback_t; 
      
public:
    struct config_t {
        void*              ctx          = nullptr;

        const char*        title        = RGH_VERSION_STRING"/Immersive";
        int                width        = 64;
        int                height       = 64;

        Word_              srf_bgn_as   = Default;

        init_callback_t    init_cb      = nullptr;
        frame_callback_t   loop_cb      = nullptr;
        exit_callback_t    exit_cb      = nullptr;

    };

_RGH_PROTECTED:
    config_t           _config    = {};
    std::atomic_bool   _running   = { false };

#pragma region WINDOW
_RGH_PROTECTED:
    GLFWwindow*                               _glfwnd      = nullptr;
    const char*                               _rend_str    = nullptr;     
    const char*                               _gl_str      = nullptr;
    std::stack< imm::Ren_target* >            _ren_targs   = {};
    Dispenser< std::vector< std::string > >   _dnd_files   = { DispenserMode_Lock };

public:
    void push_render_target( 
        RGH_IN   imm::Ren_target*   ren_trg_ 
    ) {
        _ren_targs.push( ren_trg_ );
        glBindFramebuffer( GL_FRAMEBUFFER, ren_trg_->_fbo );
        glViewport( 0, 0, ren_trg_->_r.x, ren_trg_->_r.y );
    }

    void pop_render_target( 
        void 
    ) {
        _ren_targs.pop();
        if( not _ren_targs.empty() ) {
            auto* ren_targ = _ren_targs.top();
            glBindFramebuffer( GL_FRAMEBUFFER, ren_targ->_fbo );
            glViewport( 0, 0, ren_targ->_r.x, ren_targ->_r.y );
        } else {
            glBindFramebuffer( GL_FRAMEBUFFER, GL_NONE );
            int w, h; glfwGetFramebufferSize( _glfwnd, &w, &h );
            glViewport( 0, 0, w, h );
        }
    }

public:
    void clear( 
        RGH_IN   glm::vec4   c_ = { .0, .0, .0, 1.0 } 
    ) {
        glClearColor( c_.r, c_.g, c_.b, c_.a );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }

    void swap( 
        void 
    ) {
        glfwSwapBuffers( _glfwnd );
    }

public:
    RGH_inline void engage_face_culling( void ) { glEnable( GL_CULL_FACE ); }
    RGH_inline void disengage_face_culling( void ) { glDisable( GL_CULL_FACE ); }

    RGH_inline void mode_fill( void ) { glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); }
    RGH_inline void mode_wireframe( void ) { glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); }
    RGH_inline void mode_points( void ) { glPolygonMode( GL_FRONT_AND_BACK, GL_POINT ); }

    RGH_inline void engage_depth_test( void ) { glEnable( GL_DEPTH_TEST ); }
    RGH_inline void disengage_depth_test( void ) { glDisable( GL_DEPTH_TEST ); }

public:
    bool has_dropped_files( void ) const { return !_dnd_files.watch()->empty(); }
    auto flush_dropped_files( void ) { return std::move( *_dnd_files.control() ); }

public:
    status_t set_icon( 
        RGH_IN   GLFWimage   img_ 
    ) {
        glfwSetWindowIcon( _glfwnd, 1, &img_ );
        return RGH_OK;
    }
#pragma endregion WINDOW

#pragma region ASSETS
public:
    struct payload_t {
        enum Verb_ {
            Verb_Nop,
            Verb_TexUpld, Verb_TexReld
        };
        
    #define _ADD_UN_STRUCT( struct_, fields_ ) struct struct_##_t{ fields_ }; payload_t( Verb_ verb_, struct_##_t st_ ) : verb{ verb_ }, struct_{ std::move( st_ ) } {}
        _ADD_UN_STRUCT( tex_upld,  
            HVec< imm::Tex >     ref    = nullptr;
            HVec< byte_t >       pxls   = nullptr;
            int                  w      = 0;
            int                  h      = 0;
            imm::Tex::params_t   prms   = {};
        )
        _ADD_UN_STRUCT( tex_reld, 
            HVec< imm::Tex >     ref    = nullptr;
            HVec< byte_t >       pxls   = nullptr;
            int                  w      = 0;
            int                  h      = 0;
        )
    #undef _ADD_UN_STRUCT
        union {
            char         dummy       = 0x00;    

            tex_upld_t   tex_upld;
            tex_reld_t   tex_reld;
        };

        Verb_   verb;

        payload_t( void ) = default;

        payload_t( payload_t&& other_ ) noexcept : verb{ other_.verb } {
            switch( verb ) {
                case Verb_Nop: break;
                case Verb_TexUpld:
                    ::new( &tex_upld ) tex_upld_t{ std::move( other_.tex_upld ) };
                    break;
                case Verb_TexReld:
                    ::new( &tex_reld ) tex_reld_t{ std::move( other_.tex_reld ) };
                    break;
            }
        }
        payload_t& operator = ( payload_t&& other_ ) noexcept {
            this->~payload_t();
            ::new( this ) payload_t{ std::move( other_ ) };
            return *this;
        }

        ~payload_t( void ) {
            switch( verb ) {
                case payload_t::Verb_Nop: break;

                case payload_t::Verb_TexUpld: tex_upld.~tex_upld_t(); break;
                case payload_t::Verb_TexReld: tex_reld.~tex_reld_t(); break;
            }
        }
    };

_RGH_PROTECTED:
    Ring_atomic_MPSC< payload_t, 16 >   _payload_ring;

_RGH_PROTECTED:
    void _resolve_payloads(
        void
    ) {
        payload_t payload = {};
        RGH_ASSERT_STATUS_OR( _payload_ring.pop( &payload ) ) return;

        switch( payload.verb ) {
            case payload_t::Verb_Nop: break;

            case payload_t::Verb_TexUpld: {
                auto& noun = payload.tex_upld;

                noun.ref->upld( { .width = noun.w, .height = noun.h, .pixels = ( unsigned char* )noun.pxls.get() }, noun.prms );
                break; }
            case payload_t::Verb_TexReld: {
                auto& noun = payload.tex_reld;

                noun.ref->reld( { .width = noun.w, .height = noun.h, .pixels = ( unsigned char* )noun.pxls.get() } );
                break; }
        }
    }

public:
    status_t push_payload( 
        IN   payload_t   payload_
    ) {
        return _payload_ring.push( std::move( payload_ ) );
    }

_RGH_PROTECTED:
    // struct _assets_t {
    //     /* A perlin noise splasher by XorDev: https://x.com/XorDev/status/1894123951401378051. */
    //     struct idle_splash_t {
    //         inline static float          vrtx[]   = { 1,1, 1,-1, -1,-1, -1,1 };
    //         inline static unsigned int   idx[]    = { 0,1,3, 1,2,3 };
    //         GLuint                       VAO      = GL_NONE;
    //         GLuint                       VBO      = GL_NONE;
    //         GLuint                       EBO      = GL_NONE;
    //         HVec< imm::pipe_t >          pipe     = nullptr;
    //     } idle_splash;
    // } _assets;

    // void _init_assets( void );
    // void _clean_assets( void );

public:
    status_t assets_idle_splash_render( const frame_cb_args_t& args_ );
#pragma endregion ASSETS

#pragma region DEARIMGUI
public:
    struct imgui_t {
        ImGuiIO*      io    = nullptr;
        ImGuiStyle*   stl   = nullptr;
    } imgui;

public:
    RGH_inline static auto& io( void ) { return ImGui::GetIO(); }

    RGH_inline static void movx( float dx_ ) { ImGui::SetCursorPosX( ImGui::GetCursorPosX() + dx_ ); }
    RGH_inline static void movy( float dy_ ) { ImGui::SetCursorPosY( ImGui::GetCursorPosY() + dy_ ); }
    RGH_inline static void movxy( float dx_, float dy_ ) { movx( dx_ ); movy( dy_ ); }
    RGH_inline static void movxy( const ImVec2& dv_ ) { ImGui::SetCursorPos( dv_ ); }
    RGH_inline static void movxy( const ImVec2& dv_, const ImVec2& ddv_ ) { movxy( dv_ + ddv_ ); }

    RGH_inline static auto cursor( void ) { return ImGui::GetCursorPos(); }
    RGH_inline static auto here( void ) { return ImGui::GetCursorPos(); }
    RGH_inline static auto ms_in_item( void ) { return ImGui::GetMousePos() - ImGui::GetItemRectMin(); }

    inline static ImVec2   _chpt_cursor   = {};
    RGH_inline static auto chpt_get( void ) { return _chpt_cursor; }
    RGH_inline static auto chpt_here( void ) { return _chpt_cursor = ImGui::GetCursorPos(); }  
    RGH_inline static void chpt_return( const ImVec2& ofs_ = {} ) { movxy( _chpt_cursor + ofs_ ); }
    RGH_inline static auto chpt_dx( void ) { return cursor().x - _chpt_cursor.x; }
    RGH_inline static auto chpt_dy( void ) { return cursor().y - _chpt_cursor.y; }

    RGH_inline static void scale_font( float scl_ = 1.0f ) { ImGui::SetWindowFontScale( scl_ ); }

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

    RGH_inline static bool was_dbl_clk( void ) { return ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ); }

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
        } else if( imm_ ) if( auto files_ = imm_->flush_dropped_files(); not files_.empty() ) {
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
#pragma endregion DEARIMGUI

public:
    status_t main( 
        RGH_IN   int               argc_, 
        RGH_IN   char*             argv_[], 
        RGH_IN   const config_t&   config_ 
    );

    void sig_main_exit( 
        void 
    ) { 
        _running.store( false, std::memory_order_relaxed ); 
    }
};

}

#endif