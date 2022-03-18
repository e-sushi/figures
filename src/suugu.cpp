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
	
	{//init debug
		//TEST_deshi_core();
		//TEST_kigu();
	}

	Graph g;
	g.xAxisLabel = cstr("x");
	g.yAxisLabel = cstr("y");
	const u32 res = 1000;
	vec2g data[res];
	g.data={data,res};

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
			if(DeshInput->KeyPressed(Key::F1 | InputMod_Lalt)) ToggleBool(show_metrics);
			if(show_metrics) UI::ShowMetricsWindow();
			UI::Begin("graphe", vec2::ONE, vec2::ONE*600, UIWindowFlags_NoScroll);
			//g.cameraZoom = (sin(DeshTotalTime/3) + 1) / 2 * 50;
			//g.cameraPosition=50*vec2(sin(DeshTotalTime/10), cos(DeshTotalTime/10));
			//g.xMajorLinesIncrement=BoundTimeOsc(0.1, 5);
			//g.yMajorLinesIncrement=BoundTimeOsc(0.1, 5);
			//UI::Text(toStr(g.cameraZoom).str);
			//if(DeshInput->KeyDown(Key::SPACE))
			
			g.xShowMinorLines=false;
			g.yShowMinorLines=false;
			f64 time = DeshTotalTime;
			forI(res){
				f64 alignment = (g.cameraPosition.x-g.cameraZoom)+f64(i)/res*g.cameraZoom*2;
				data[i].x = alignment;
				data[i].y = sin(data[i].x);
			}


			draw_graph(g, UI::GetWindow()->dimensions-UI::GetStyle().windowMargins*2);
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
			g.cameraZoom -= 0.2*g.cameraZoom*DeshInput->scrollY;
			UI::Text("after the graph");
			UI::End();
			//draw_pixels();
			//random_draw(200);
			//random_walk_avoid();
			//vector_field();
			//UI::DemoWindow();
			UI::ShowMetricsWindow();
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