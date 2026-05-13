#include <bridge.hpp>
MDN_MODULE_HEADER( honopt, "hon-opt" )

struct Dock : dock_t {
public:
    Dock( void ) = default;

public:
    inline static const char* const METHODS[] = {
        "Newt",
        "Steep",
        "Cnj-FR",
        "Cnj-PR",
        "Cnj-HS",
        "QN-DFP",
        "QN-BFGS",
        "Rosen",
        "Nel-Md"
    };
    inline static constexpr int METHOD_COUNT = sizeof( METHODS ) / sizeof( const char* );
    enum Method_ {
        Method_Newton,
        Method_Steepest,
        Method_ConjugateFr,
        Method_ConjugatePr,
        Method_ConjugateHs,
        Method_QuasiNewtonDFP,
        Method_QuasiNewtonBFGS,
        Method_Rosenbrock,

        Method_NelderMead
    };

public:
    struct _ex_t {
        _ex_t( void ) = default;

         _ex_t( string_view in_exp_, const ImPlotRect& lims_ )
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

            constexpr int steps = 680;
            grid.span_n( { { steps, lims_.X.Min, lims_.X.Max }, { steps, lims_.Y.Max, lims_.Y.Min } } );
            grid.apply( [ this ] ( double* x ) -> double { return f( x[0], x[1] ); } );
        }

        Fast_exp< double >   exp;
        fnc_2d_t<>           f;
        grid_t<>             grid;
    };

public:
    Dispenser< _ex_t >   ex                           = { DispenserMode_Drop };   
    int                  slices                       = 100;
    ImPlotRect           limits                       = { -1, 1, -1, 1 };
    atomic_bool          updating                     = { false };
 
    string               in_exp;

    int                  step_count[ METHOD_COUNT ]   = { 0 };
    int                  all_step_count               = 0;
    bool                 controlled[ 2 ]              = { false, false };

    arr_t<2>             grad;
    arr_t<4>             hess;
    arr_t<2>             x0                           = { 0, 0 };

    arr_t<2>             v0[ 3 ]                      = { 0 };

public:
    void compute_gradient( arr_t<2> X ) { 
        grad = d1_f2( X[0], X[1], 1e-6, ex->f ); 
    }

    void compute_hessian( arr_t<2> X ) { 
        hess = d2h_f2( X[0], X[1], 1e-6, ex->f ); 
    }

    void plot_trig( const char* lbl_, arr_t<2> v_[], int stride_ ) {
        ImPlot::PlotLine( lbl_, &v_[0][0], &v_[0][1], 3, ImPlotLineFlags_Loop, 0, stride_ );
    }

public:
    MDN_DOCK_NAME_FNC override { return DOCK_NAME; }

    MDN_DOCK_GUIX_FNC override {
        auto ex = this->ex.watch();

        bool open = true;
        if( ImGui::Begin( "Diff Optimizations", &open, ImGuiWindowFlags_None ) ) {
            ImGui::Separator();
            ImGui::TextUnformatted( "f(x1,x2) =" ); ImGui::SameLine();
            bool new_exp = ImGui::InputText( "##in-exp", &in_exp, ImGuiInputTextFlags_EnterReturnsTrue );
            ImGui::Separator();

            if( ex ) {
                ImPlot::PushColormap( ImPlotColormap_Viridis );
                if( ImPlot::BeginPlot( "##plt-f", {680, 680} ) ) {
                    if( ImPlot::IsPlotHovered() && ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) ) {
                        controlled[ 0x0 ] = ImGui::IsKeyDown( ImGuiKey_1 );
                        controlled[ 0x1 ] = ImGui::IsKeyDown( ImGuiKey_2 ); 

                        if( ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
                            auto p = ImPlot::GetPlotMousePos();

                            if( controlled[ 0x0 ] ) {
                                x0 = { p.x, p.y };
                            }

                            if( controlled[ 0x1 ] ) {
                                *std::min_element( v0, v0+3, [ &p ] ( const auto& v1_, const auto& v2_ ) -> bool {
                                    return v1_.dist_sq( p ) < v2_.dist_sq( p );
                                } ) = { p.x, p.y };
                            }
                        }
                    } else {
                        fill_n( controlled, size( controlled ), 0x0 );
                    }
                    
                    ImPlot::PlotHeatmap(
                        "##htm-f", ex->grid.raw(), ex->grid.n_of(1), ex->grid.n_of(0),
                        ex->grid.min(), ex->grid.max(), nullptr, { limits.X.Min, limits.Y.Min }, { limits.X.Max, limits.Y.Max },
                        ImPlotHeatmapFlags_None
                    );

                    const auto new_limits = ImPlot::GetPlotLimits();
                    if( memcmp( &new_limits, &limits, sizeof( limits ) != 0x0 ) ) {
                        limits = new_limits;
                        new_exp |= true;
                    }

                    ImPlot::PushColormap( ImPlotColormap_Twilight );
                    ImPlot::PushStyleVar( ImPlotStyleVar_LineWeight, 3.0f );
                    const auto ctrl_sin = sin( args_.t*5.6 );

                    /* =-. Newton .-= */ {
                    auto xk = x0;
                    for( int n = 1; n <= step_count[ Method_Newton ]; ++n ) {
                        compute_gradient( xk ); 
                        compute_hessian( xk );
                        MDN_ASSERT_OR( RGH_OK == invm( hess.get(), 2 ) ) break;

                        arr_t<2> xk1 = {
                            xk.x() - hess[0]*grad.x() - hess[1]*grad.y(),
                            xk.y() - hess[2]*grad.x() - hess[3]*grad.y()
                        };

                        ImPlot::PlotLine( METHODS[ Method_Newton ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_Newton ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        xk = xk1;
                    }
                    }

                    /* =-. Steepest .-= */ {
                    auto xk = x0;
                    for( int n = 1; n <= step_count[ Method_Steepest ]; ++n ) {
                        compute_gradient( xk );
                        auto dk = -grad; dk /= dk.norm();
                        
                        auto [ s, _ ] = search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_Steepest ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_Steepest ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        xk = xk1;
                    }
                    }

                    /* =-. Conjugate FR .-= */ {
                    auto             xk = x0;
                    arr_t<2>  dk = { 0, 0 };
                    double           bk = 0;
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_ConjugateFr ]; ++n ) {
                        auto gk = grad;
                        
                        dk = -gk + dk*bk;

                        auto [ s, _ ] = search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_ConjugateFr ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_ConjugateFr ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        compute_gradient( xk1 );
                        auto gk1 = grad;
                        
                        bk = pow(gk1.norm(),2) / pow(gk.norm(),2);

                        xk = xk1;
                    }
                    }

                    /* =-. Conjugate PR .-= */ {
                    auto             xk = x0;
                    arr_t<2>  dk = { 0, 0 };
                    double           bk = 0;
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_ConjugatePr ]; ++n ) {
                        auto gk = grad;
                        
                        dk = -gk + dk*bk;

                        auto [ s, _ ] = search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_ConjugatePr ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_ConjugatePr ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        compute_gradient( xk1 );
                        auto gk1 = grad;

                        bk = gk1.dot( gk1-gk ) / gk.dot( gk );

                        xk = xk1;
                    }
                    }

                    /* =-. Conjugate HS .-= */ {
                    auto             xk = x0;
                    arr_t<2>  dk = { 0, 0 };
                    double           bk = 0;
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_ConjugateHs ]; ++n ) {
                        auto gk = grad;
                        
                        dk = -gk + dk*bk;

                        auto [ s, _ ] = search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_ConjugateHs ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_ConjugateHs ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        compute_gradient( xk1 );
                        auto gk1 = grad;

                        bk = gk1.dot( gk1-gk ) / dk.dot( gk1-gk );

                        xk = xk1;
                    }
                    }

                    /* =-. Quasi Newton DFP .-= */ {
                    auto            xk = x0;
                    double          k  = 0;
                    arr_t<4> B  = { 1,0, 0,1 };
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_QuasiNewtonDFP ]; ++n ) {
                        arr_t<2> dk = {
                            -B[0]*grad[0] - B[1]*grad[1],
                            -B[2]*grad[0] - B[3]*grad[1]
                        };

                        auto [ s, _ ] = search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk[0], dxk[1] );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_QuasiNewtonDFP ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_QuasiNewtonDFP ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }


                        auto gk = grad;
                        compute_gradient( xk1 );
                        auto gk1 = grad;

                        auto dxk = xk1 - xk; 
                        auto Gk  = gk1 - gk;

                        arr_t<4> a = {
                            dxk[0]*dxk[0], dxk[0]*dxk[1],
                            dxk[1]*dxk[0], dxk[1]*dxk[1]
                        };
                        a /= dxk.dot(Gk);

                        arr_t<2> b = {
                            B[0]*Gk[0] + B[1]*Gk[1], B[2]*Gk[0] + B[3]*Gk[1]
                        };

                        arr_t<4> c = {
                            b[0]*b[0], b[0]*b[1],
                            b[1]*b[0], b[1]*b[1]
                        };
                        c /= Gk.dot(b);

                        B = B + a - c;
                        xk = xk1;
                    }
                    }

                    /* =-. Quasi Newton BFGS .-= */ {
                    auto            xk = x0;
                    double          k  = 0;
                    arr_t<4> B  = { 1,0, 0,1 };
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_QuasiNewtonBFGS ]; ++n ) {
                        arr_t<2> dk = {
                            -B[0]*grad[0] - B[1]*grad[1],
                            -B[2]*grad[0] - B[3]*grad[1]
                        };

                        auto [ s, _ ] = search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk[0], dxk[1] );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_QuasiNewtonBFGS ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_QuasiNewtonBFGS ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }


                        auto gk = grad;
                        compute_gradient( xk1 );
                        auto gk1 = grad;

                        auto dxk = xk1 - xk; 
                        auto Gk  = gk1 - gk;

                        arr_t<4> a = {
                            Gk[0]*Gk[0], Gk[0]*Gk[1],
                            Gk[1]*Gk[0], Gk[1]*Gk[1]
                        };
                        a /= Gk.dot(dxk);

                        arr_t<2> b = {
                            B[0]*dxk[0] + B[1]*dxk[1], B[2]*dxk[0] + B[3]*dxk[1]
                        };

                        arr_t<4> c = {
                            b[0]*b[0], b[0]*b[1],
                            b[1]*b[0], b[1]*b[1]
                        };
                        c /= dxk.dot(b);

                        B = B + a - c;
                        xk = xk1;
                    }
                    }

                    /* =-. Rosenbrock .-= */ {
                    arr_t<2> d[ 2 ] = { { 1.0, 0.0 }, { 0.0, 1.0 } };
                    arr_t<2> s      = { 1.0, 1.0 };
                    double   a      = 0.5;
                    double   b      = -0.5;
                    auto     xk     = x0;

                    for( int n = 1; n <= step_count[ Method_Rosenbrock ]; ++n ) {
                        double   c[ 2 ]       = { 0, 0 };
                        bool     osc          = false;
                        bool     success[ 2 ] = { false, false };
                        bool     fail[ 2 ]    = { false, false };
                        arr_t<2> xk1;

                        while( not osc ) {
                            for( int i = 0; i < 2; ++i ) {
                                if( ex->f( xk + d[i]*s[i] ) < ex->f( xk ) ) {
                                    xk1 = xk + d[i]*s[i];

                                    ImPlot::PlotLine( METHODS[ Method_Rosenbrock ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                                    if( ImPlot::IsLegendEntryHovered( METHODS[ Method_Rosenbrock ] ) ) {
                                        ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                                    }

                                    xk = xk1;

                                    success[i] =  true;
                                    c[i]       += s[i]; 
                                    s[i]       *= a;
                                } else {
                                    fail[ i ] =  true;
                                    s[i]      *= b;
                                }
                            }
                            osc = count( success, success+2, true ) == 2 && count( fail, fail+2, true ) == 2;
                        }

                        const auto a2 = d[1]*c[1]; 
                        const auto a1 = d[0]*c[0] + a2;

                        const auto b1 = a1;
                        const auto b2 = a2 - b1 * (a2.dot(b1) / b1.norm());

                        d[0] = b1 / b1.norm();
                        d[1] = b2 / b2.norm();
                    }
                    }

                    const float ctrl_0 = controlled[ 0x0 ] ? ctrl_sin : 1;
                    ImPlot::PushStyleColor( ImPlotCol_MarkerFill, ImVec4{ 1, ctrl_0, ctrl_0, 1 } );
                    ImPlot::PlotScatter( "", (double*)&x0, (double*)&x0 + 1, 1 );
                    ImPlot::PopStyleColor( 1 );

                    /* =-. Nelder Mead .-= */ {
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

                    const float ctrl_1 = controlled[ 0x1 ] ? ctrl_sin : 1; 
                    ImPlot::PushStyleColor( ImPlotCol_MarkerFill, ImVec4{ 1, ctrl_1, ctrl_1, 1 } );
                    ImPlot::PlotScatter( "", (double*)&v0, (double*)&v0 + 1, 3, 0, 0, sizeof( v0[0] ) );
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

                ImGui::SeparatorText( "Initial guess [ Point ]" );
                ImGui::SetNextItemWidth( 680 );
                ImGui::InputScalarN( "##in-ig", ImGuiDataType_Double, &x0, 2 );

                ImGui::SeparatorText( "Initial guess [ Triangle ]" );
                ImGui::SetNextItemWidth( 680 ); ImGui::InputScalarN( "v1", ImGuiDataType_Double, v0, 2 );
                ImGui::SetNextItemWidth( 680 ); ImGui::InputScalarN( "v2", ImGuiDataType_Double, v0+1, 2 );
                ImGui::SetNextItemWidth( 680 ); ImGui::InputScalarN( "v3", ImGuiDataType_Double, v0+2, 2 );
            }

            if( new_exp ) {
                bool upd_ex = false;
                if( updating.compare_exchange_strong( upd_ex, true, std::memory_order_release ) ) {
                    thread( [ this ] { 
                        if( not in_exp.empty() ) this->ex.control( in_exp, limits ); 
                        updating.store( false, std::memory_order_release );
                    } ).detach();
                }
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