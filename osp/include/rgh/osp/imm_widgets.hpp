#pragma once
/**
 * @file: osp/imm_widgets.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/osp/immersive.hpp>
#include <rgh/osp/IO_utils.hpp>
#include <rgh/osp/thread_pool.hpp>

namespace rgh::imm_widgets {


class Dropdown_list {
public:
    Dropdown_list( const char* const strs_[], int size_, int selected_ = -0x1 ) : _strs{ strs_ }, _size{ size_ }, _sel{ selected_ } {}

_RGH_PROTECTED:
    int                  _sel     = -0x1;
    const char* const*   _strs    = nullptr;
    int                  _size    = 0x0;

public:
    int imm_frame( const char* const title_, bool* changed_ = nullptr ) {
        const int prev_sel = _sel;

        if( ImGui::BeginCombo( title_, _sel >= 0 && _sel < _size ? _strs[ _sel ] : "N/A" ) ) {
            for( int idx = 0x0; idx < _size; ++idx ) {
                const bool selected = idx == _sel;
                if( ImGui::Selectable( _strs[ idx ], selected ) ) _sel = idx;
            }
            ImGui::EndCombo();
        }

        if( changed_ ) *changed_ |= _sel != prev_sel;
        return _sel;
    }

};

class COM_ports {
public:
    struct frame_args_t : rgh::Immersive::frame_cb_args_t {
        IN       const char*                    scan_btn_lbl_   = "Scan for COM ports";
        IN       const char*                    no_ports_lbl_   = "No COM ports found.";
        IN_OPT   HVec< Task_taker >             tsk_tkr_        = nullptr;
        IN_OPT   std::array< const char*, 3 >   hl_keys_        = {};
    };

public:
    COM_ports( void ) = default;

    COM_ports( HVec< io::COM_ports > ports_ ) { this->bind( std::move( ports_ ) ); }

    ~COM_ports( void ) {
        _ports->unregister_hotplug_callback( RGH_TXTUUID_FROM_THIS );
    }

_RGH_PROTECTED:
    HVec< io::COM_ports >   _ports    = nullptr;
    int                     _sel      = -0x1;
    std::string             _sel_id   = "";

public:
    status_t bind( HVec< io::COM_ports > ports_ ) {
        _ports = std::move( ports_ );

    //# The registered hotplug callback lambda function is invoked under a dispenser control, 
    //#   therefore there is no need to worry about races over the container.
        _ports->register_hotplug_callback( RGH_TXTUUID_FROM_THIS, [ this ] ( io::COM_ports::container_t& ports_ ) -> void {
            auto itr = std::ranges::find_if( ports_, [ this ] ( const io::COM_port_t& port_ ) -> bool {
                return port_.id == _sel_id;
            } );
            RGH_ASSERT_OR( ports_.end() != itr ) {
                _sel    = -0x1;
                _sel_id = "";
                return;
            }

            _sel = std::distance( ports_.begin(), itr );
        } ); 

        return OK;
    }

//# connect, scan requested
    std::tuple< const io::COM_port_t*, bool > imm_frame( 
        IN   io::COM_ports::watch_t&   watch_,
        IN   const frame_args_t&       args_      
    ) {
        const io::COM_port_t* conn_to  = nullptr;
        bool                  scan_req = ImGui::Button( args_.scan_btn_lbl_ );
        bool dconn   = false;

        auto& ports = *watch_;
        if( not ports.empty() ) {
            for( int idx = 0x0; idx < ports.size(); ++idx ) {
                auto& crt_port = ports[ idx ];

                ImGui::Separator();
                    int  selectable_flags = ImGuiSelectableFlags_None;
                    bool selected         = false;
                
                    ImGui::SameLine();
                    if( ImGui::Button( "Connect" ) ) conn_to = &crt_port;
                    if( ImGui::IsItemHovered() ) { selectable_flags |= ImGuiSelectableFlags_Highlight; }

                    for( const char* hl_key : args_.hl_keys_ ) {
                        if( not args_.blink_500ms || hl_key == nullptr ) break;
                        
                        if( crt_port.detail.contains( hl_key ) ) {
                            selected |= true;
                            break;
                        }
                    }
                    ImGui::SameLine(); ImGui::Bullet();
                    if( ImGui::Selectable( crt_port.detail.c_str(), selected, static_cast< ImGuiSelectableFlags_ >( selectable_flags ) ) ) {
                        
                    }
                ImGui::Separator();
            }
        } else {
            ImGui::Separator();
            ImGui::BulletText( args_.no_ports_lbl_ );
            ImGui::Separator();
        }
        
        if( scan_req && args_.tsk_tkr_ ) args_.tsk_tkr_->task_taker_pass( [ ports = _ports ] ( void ) -> void { ports->scan(); } );

        return { conn_to, scan_req }; 
    }

public:
    RGH_inline io::COM_ports* operator -> ( void ) { return _ports.get(); }
};


};

