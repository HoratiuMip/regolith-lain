#include <bridge.hpp>
MDN_MODULE_HEADER( honoptdiffless, "hon-opt-diffless" )

struct Dock : dock_t {
public:
    Dock( void ) = default;

public:
    inline static const char* const METHODS[] = {
        "NM"
    };
    inline static constexpr int METHOD_COUNT = sizeof( METHODS ) / sizeof( const char* );
    enum Method_ {
        Method_NelderMead
    };

public:
    struct _ex_t {
        _ex_t( void ) = default;

         _ex_t( string_view in_exp_ )
        : exp{ in_exp_ }
        {
            f = [ this ] ( double x1, double x2 ) {
                exp.bind( [ x1, x2 ] ( string_view var_, double* val_ ) -> status_t {
                    if( var_ == "x1" ) { *val_ = x1; return RGH_OK; }
                    if( var_ == "x2" ) { *val_ = x2; return RGH_OK; }
                    return RGH_ERR_NOT_FOUND;
                } );
                double res = 0.0; 
                MDN_ASSERT_OR( RGH_OK == exp.resolve( &res ) && not isnan( res ) ) return 0.0;
                return res;
            };

            constexpr double step = 0.1;
            grid.span_s( { { step, -8, 8 }, { step, 8, -8 } } );
            grid.apply( [ this ] ( double* x ) -> double { return f( x[0], x[1] ); } );
        }

        Fast_exp< double >   exp;
        fnc_2d_t<>           f;
        grid_t<>             grid;
    };

public:
    Dispenser< _ex_t >   ex                           = { DispenserMode_Drop };    

    string               in_exp;

    int                  step_count[ METHOD_COUNT ]   = { 0 };
    int                  all_step_count               = 0;

    arr_t<2>             grad;
    arr_t<4>             hess;
    arr_t<2>             v0[ 3 ]                      = { 0 };

public:
    void plot_trig( const char* lbl_, arr_t<2> v_[], int stride_ ) {
        ImPlot::PlotLine( lbl_, &v_[0][0], &v_[0][1], 3, ImPlotLineFlags_Loop, 0, stride_ );
    }

public:
    MDN_DOCK_NAME_FNC override { return DOCK_NAME; }

    MDN_DOCK_GUIX_FNC override {
        auto ex = this->ex.watch();

        bool open = true;
        if( ImGui::Begin( "Diffless Optimizations", &open, ImGuiWindowFlags_None ) ) {
            ImGui::Separator();
            ImGui::TextUnformatted( "f(x1,x2) =" ); ImGui::SameLine();
            const bool new_exp = ImGui::InputText( "##in-exp", &in_exp, ImGuiInputTextFlags_EnterReturnsTrue );
            ImGui::Separator();

            if( ex ) {
                ImPlot::PushColormap( ImPlotColormap_Viridis );
                if( ImPlot::BeginPlot( "##plt-f", {680, 680} ) ) {
                    if( ImPlot::IsPlotHovered() && ImGui::IsMouseDown( ImGuiMouseButton_Left ) && ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) ) {
                        auto p = ImPlot::GetPlotMousePos();
                        if( ImGui::IsKeyDown( ImGuiKey_1 ) )
                            v0[ 0 ] = { p.x, p.y };
                        else if( ImGui::IsKeyDown( ImGuiKey_2 ) )
                            v0[ 1 ] = { p.x, p.y };
                        else if( ImGui::IsKeyDown( ImGuiKey_3 ) )
                            v0[ 2 ] = { p.x, p.y };
                    }
                    
                    ImPlot::PlotHeatmap(
                        "##htm-f", ex->grid.raw(), ex->grid.n_of(1), ex->grid.n_of(0),
                        ex->grid.min(), ex->grid.max(), nullptr, {-8,-8}, {8,8},
                        ImPlotHeatmapFlags_None
                    );

                    ImPlot::PushColormap( ImPlotColormap_Twilight );
                    ImPlot::PushStyleVar( ImPlotStyleVar_LineWeight, 3.0f );
                    ImPlot::PushStyleColor( ImPlotCol_MarkerFill, ImVec4{ 1,1,1,1 } );
                    
                    /* === Nelder Mead === */ {
                    struct point_t {
                        arr_t<2>   vk;
                        double     f;
                    } points[ 3 ] = {
                        { .vk = v0[ 0 ] }, { .vk = v0[ 1 ] }, { .vk = v0[ 2 ] }
                    };  
                    auto& B = points[0].vk, &G = points[1].vk, &W = points[2].vk;
                    auto& fB = points[0].f, &fG = points[1].f, &fW = points[2].f;
                    
                    for( int n = 1; n <= step_count[ Method_NelderMead ]; ++n ) {
                        for( auto& p : points ) p.f = ex->f( p.vk );
                        std::sort( points, points+3, [] ( const auto& lhs_, const auto& rhs_ ) { return lhs_.f < rhs_.f; } );
                        plot_trig( METHODS[ Method_NelderMead ], &points[0].vk, sizeof( point_t ) );
                        
                        const auto M  = (B+G) / 2;
                        const auto R  = M*2 - W;
                        const auto fr = ex->f( R );

                        if( fr < fW ) {
                            const auto E = R*2 - M;
                            W = ex->f( E ) < fr ? E : R;
                        } else {
                            const auto C1 = (M+W) / 2;
                            const auto C2 = (M+R) / 2;
                            const auto C  = ex->f( C1 ) < ex->f( C2 ) ? C1 : C2;

                            if( ex->f( C ) < fW ) {
                                W = C;
                            } else {
                                const auto S = (B+W)/2;
                                W = S;
                                G = M;
                            }
                        }
                    }
                    }

                    ImPlot::PopStyleColor( 1 );
                    ImPlot::PopStyleVar( 1 );
                    ImPlot::PopColormap( 1 );
                    ImPlot::EndPlot();
                }

                ImGui::SameLine();
                ImPlot::ColormapScale( "z", ex->grid.min(), ex->grid.max(), {100,680} );
                ImPlot::PopColormap(); 
                
                ImGui::SameLine();
                ImGui::BeginTable( "Steps", 1+METHOD_COUNT, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_HighlightHoveredColumn );
                for( int i = 0x0; i < METHOD_COUNT; ++i ) {
                    ImGui::TableSetupColumn( METHODS[i], ImGuiTableColumnFlags_AngledHeader );
                }
                ImGui::TableSetupColumn( "##tblcol-all", ImGuiTableColumnFlags_None );

                ImGui::TableAngledHeadersRow();
                ImGui::TableNextRow();
                
                for( int i = 0x0; i < METHOD_COUNT; ++i ) {
                    ImGui::TableNextColumn();
                    ImGui::VSliderInt( format( "##in-step-count-{}", METHODS[i] ).c_str(), {36, 590}, step_count + i, 0, 100 );
                }
                ImGui::TableNextColumn();
                if( ImGui::VSliderInt( "##in-step-count-all", {36, 590}, &all_step_count, 0, 100 ) ) {
                    fill_n( step_count, METHOD_COUNT, all_step_count );
                }

                ImGui::EndTable();

                ImGui::SeparatorText( "Initial guess" );
                ImGui::SetNextItemWidth( 680 ); ImGui::InputScalarN( "v1", ImGuiDataType_Double, v0, 2 );
                ImGui::SetNextItemWidth( 680 ); ImGui::InputScalarN( "v2", ImGuiDataType_Double, v0+1, 2 );
                ImGui::SetNextItemWidth( 680 ); ImGui::InputScalarN( "v3", ImGuiDataType_Double, v0+2, 2 );
            }

            if( new_exp ) {
                thread( [ this ] { if( not in_exp.empty() ) this->ex.control( in_exp ); } ).detach();
            }
        } 
        ImGui::End();
        return open ? RGH_OK : RGH_ERR_TERMINATED; 
    }
};

struct Proxy : proxy_t {
public:
    Proxy( void )
    : proxy_t{
        DOCK_NAME,
        {},
        { 
        MDN_PROXY_CLI_BASIC_INSTALL
        }
    } {}
};

MDN_AUTO_INSTALL( Proxy );
MDN_MODULE_FOOTER