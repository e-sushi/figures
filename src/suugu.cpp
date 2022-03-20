/* suugu
TODO Board
----------------------------------------------------
most math should be f64 instead of f32

`Canvas`
--------
add checks to skip draw calls if they arent on screen
extract graphing to its own deshi module
workspaces
arbitrary text
custom to_string for f64 since we want to have config on sigfigs and it only goes to 6 decimals (this may be accuracy limit for f64)

`Config`
--------
decimal separator: comma vs point
thousand separator: space vs comma vs point vs apostrophe

`Input`
-------
cursor inside literals

`Parser`
--------
implement a system for adding to an already existing AST tree
parser is copying tokens from elements rather than simply viewing them

`Serialization`
--------------
constants loader

`Solver`
--------


Bug Board       //NOTE mark these with a last-known active date (MM/DD/YY)
---------
(03/07/22) fix zoom in/out consistency
(03/07/22) the program freezes if ALT is pressed and doesnt resume until the window is moved
(03/15/22) element hitboxes are incorrect
(03/19/22) minimizing the window with screen based equation sampling causes deshi to freeze in BuildCommonds() (vulkan) when you reopen the window
*/

#ifdef TRACY_ENABLE
#  include "Tracy.hpp"
#endif

//// kigu includes ////
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"

//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "core/assets.h"
#include "core/commands.h"
#include "core/console.h"
#include "core/graphing.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/imgui.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "core/io.h"
#include "math/math.h"

//// suugu includes ////
#define SUUGU_IMPLEMENTATION
#ifdef SUUGU_USE_GRAPHVIZ
#  include "graphviz/gvc.h"
#endif
#include "types.h"
#include "parser.cpp"
#include "solver.cpp"
#include "canvas.cpp"

//#include "kigu/deshi_utils_tests.cpp"
//#include "core/deshi_core_tests.cpp"
//#include "kigu/misc_testing.cpp"

local Canvas canvas;

typedef f64 (*MathFunc)(f64);

f64 SecantMethod(f64 x0, f64 x1, f64 tol, MathFunc func) {
	f64 x2 = 0;
	u32 it = 0;
	while (true) {
		x2 = x1 - func(x0) * (x1 - x0) / (func(x1) - func(x0));
		Log("", x2, " with func is ", func(x2));
		x0 = x1; x1 = x2;
		if (abs(func(x2)) < tol) return x2;
		if (it++ > 3000) return x2;
	}
}

void graph_testing(){
	persist b32 init = 0;
	persist const u32 res = 300;
	persist Graph g;
	persist array<vec2g> data;
	//TODO only actually resize when OSWindow size changes
	data.resize(DeshWinSize.x);
	if(!data.count)return;
	if(!init){
		g.xAxisLabel = cstr("x");
		g.yAxisLabel = cstr("y");
		init=1;
	}
	else{
		UI::Begin("graphe", vec2::ONE, vec2::ONE*600, UIWindowFlags_NoScroll);
		u32 res = UI::GetWindow()->width - UI::GetStyle().windowMargins.x*2;
		g.data={data.data,res};
		g.xShowMinorLines=false;
		g.yShowMinorLines=false;
		f64 t = DeshTotalTime;
		persist f64 floorer = 1/3.0; 
		persist f64 floored = 1;
		if(DeshInput->KeyPressed(Key::UP)) floorer += 0.1;
		if(DeshInput->KeyPressed(Key::DOWN)) floorer -= 0.1;
		
		UI::Text(toStr("  cpos", g.cameraPosition).str);
		UI::Text(toStr("czoom ", g.cameraZoom).str);
		
		forI(res){
			f64 alignment = (g.cameraPosition.x-g.cameraZoom)+f64(i)/res*g.cameraZoom*2;
			f64& x = data[i].x;
			f64& y = data[i].y;
			x = alignment;
			y = floor(x * floorer) / floorer;
		}
		
		
		draw_graph(g, UI::GetWindow()->dimensions-UI::GetStyle().windowMargins*2);
		UIItem* gr = UI::GetLastItem();
		
		
		static vec2 mp;
		static vec2 gcp;
		if(UI::IsLastItemHovered() && DeshInput->LMousePressed()){
			UI::SetPreventInputs();
			mp = DeshInput->mousePos;
			gcp = g.cameraPosition;
		}
		if(mp!=vec2::ONE*FLT_MAX && DeshInput->LMouseDown()){
			g.cameraPosition = gcp - (DeshInput->mousePos - mp) / g.dimensions_per_unit_length;
		}
		if(DeshInput->LMouseReleased()){
			UI::SetAllowInputs();
			mp=vec2::ONE*FLT_MAX;
		}
		UI::Circle(gr->position+gr->size/2, 4, 1, 20, Color_Cyan);
		
		g.cameraZoom -= 0.2*g.cameraZoom*DeshInput->scrollY;
		UI::Text("after the graph");
		UI::End();
		
	}
}

int main(){
	//init deshi
	Assets::enforceDirectories();
	memory_init(Gigabytes(1), Gigabytes(1));
	Logger::Init(5, true);
	DeshConsole->Init();
	DeshTime->Init();
	DeshWindow->Init("suugu", 1280, 720);
	Render::Init();
	DeshiImGui::Init();
	Storage::Init();
	UI::Init();
	Cmd::Init();
	DeshWindow->ShowWindow();
	Render::UseDefaultViewProjMatrix();
	
	//init suugu
	init_canvas();
	
	//init debug
	//TEST_deshi_core();
	//TEST_kigu();
	
	//start main loop
	TIMER_START(t_f);
	TIMER_START(fun);
	while(!DeshWindow->ShouldClose()){DPZoneScoped;
		DeshWindow->Update();
		DeshTime->Update();
		DeshInput->Update();
		DeshiImGui::NewFrame();
		update_canvas();
		{//update debug
			persist b32 show_metrics = false;
			if(DeshInput->KeyPressed(Key::M | InputMod_LctrlLshift)) ToggleBool(show_metrics);
			if(show_metrics) UI::ShowMetricsWindow();
			//graph_testing();
			//draw_pixels();
			//random_draw(200);
			//random_walk_avoid();
			//vector_field();
			//UI::DemoWindow();
			//Storage::StorageBrowserUI();
			//deshi__memory_draw(); //NOTE this is visually one frame behind for memory modified after it is called
		}
		DeshConsole->Update();
		UI::Update();
		Render::Update();
		memory_clear_temp();
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}
	
	//cleanup deshi
	Render::Cleanup();
	DeshWindow->Cleanup();
	Logger::Cleanup();
	memory_cleanup();
}