#pragma once
/**
 * @file: osp/imm_widgets.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <rgh/osp/immersive.hpp>
#include <rgh/osp/IO_utils.hpp>

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
    COM_ports( HVec< io::COM_ports > ports_ ) : _ports{ std::move( ports_ ) } {
        /* @note The registered hotplug callback lambda function is invoked under a dispenser control, 
            therefore there is no need to worry about races over the container. */
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
    }

    ~COM_ports( void ) {
        _ports->unregister_hotplug_callback( RGH_TXTUUID_FROM_THIS );
    }

_RGH_PROTECTED:
    HVec< io::COM_ports >   _ports    = nullptr;
    int                     _sel      = -0x1;
    std::string             _sel_id   = "";

public:
    std::tuple< io::COM_port_t*, bool, bool > imm_frame( io::COM_ports::watch_t& watch_ ) {
        bool rescan = ImGui::Button( "Scan for COM ports" );

        auto& ports = *watch_;

        bool sel_now = false;
        if( not ports.empty() ) {
            for( int idx = 0x0; idx < ports.size(); ++idx ) {
                const bool selected = idx == _sel;

                ImGui::Separator();
                ImGui::Bullet();
                if( ImGui::Selectable( ports[ idx ].detail.c_str(), selected, selected ? ImGuiSelectableFlags_Highlight : ImGuiSelectableFlags_None ) ) {
                    _sel    = idx;
                    _sel_id = ports[ _sel ].id;
                    sel_now = true;
                }
                ImGui::Separator();
            }
        } else {
            ImGui::Separator();
            ImGui::BulletText( "No COM ports found." );
            ImGui::Separator();
        }
        
        if( _sel >= 0x0 && _sel < ports.size() ) return { &ports[ _sel ], sel_now, rescan }; 

        _sel    = -0x1;
        _sel_id = "";
        return { nullptr, false, rescan };
    }

public:
    RGH_inline io::COM_ports* operator -> ( void ) { return _ports.get(); }

};


bool small_X_button( void ) {
    ImGui::PushStyleColor( ImGuiCol_Button,        ImVec4{ 0,0,0,0 } );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0,0,0,0 } );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive,  ImVec4{ 0,0,0,0 } );
    ImGui::PushStyleColor( ImGuiCol_Text,          ImVec4{ 1,0,0,1 } );
    const bool pressed = ImGui::SmallButton( "X" );
    ImGui::PopStyleColor( 4 );
    return pressed;
}


};

