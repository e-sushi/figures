/* suugu
TODO Board
----------------------------------------------------
most math should be f64 instead of f32

`Canvas`
--------
add checks to skip draw calls if they arent on screen
workspaces
arbitrary text
custom float_to_string/str_to_float since we want to have custom precision types

`Config`
--------
decimal separator: comma vs point
thousand separator: space vs comma vs point vs apostrophe

`Input`
-------
add a simulator that reads in a file/string of inputs and adds an expression from it (for testing and rough raw text input)
copy/paste expressions
implicit right parenthesis (like desmos)
expression term selection with keybinds and mouse

`Parser`
--------
hotstrings
constants

`Serialization`
--------------
constants loader

`Solver`
--------
variable solving (NOTE ast.cpp@parse_valid OpType_ExpressionEquals needs to be modified for validity when variable solving is added)

Bug Board       //NOTE mark these with first-known active date [MM/DD/YY] and last-known active date (MM/DD/YY)
---------
[03/07/22](04/12/22) fix zoom in/out consistency (zoom in then zoom out should return to the same value)
[03/07/22](04/12/22) the program freezes if ALT is pressed and doesnt resume until the window is moved or ALT is pressed again (maybe a win32 issue?)
[03/15/22](04/12/22) element hitboxes are incorrect
[03/19/22](03/19/22) minimizing the window with screen based equation sampling causes deshi to freeze in BuildCommands() (vulkan) when you reopen the window
*/

#ifdef TRACY_ENABLE
#  include "Tracy.hpp"
#endif

//// kigu includes ////
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"

//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "core/commands.h"
#include "core/console.h"
#include "core/graphing.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "core/file.h"
#include "math/math.h"

//// suugu includes ////
#define SUUGU_IMPLEMENTATION
#ifdef SUUGU_USE_GRAPHVIZ
#  include "graphviz/gvc.h"
#endif
#include "types.h"
#include "functions.cpp"
#include "ast.cpp"
#include "solver.cpp"
#include "canvas.cpp"

#if BUILD_INTERNAL
#include "misc_testing.cpp"
#endif

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
		UI::Begin("graphe", vec2::ONE, vec2(600,500), UIWindowFlags_NoScroll);
		u32 res = Min(DeshWinSize.x, UI::GetWindow()->width - UI::GetStyle().windowMargins.x*2);
		g.data={data.data,res};
		g.dotsize = 3;
		g.xShowMinorLines = false;
		g.yShowMinorLines = false;
		
		f64 t = DeshTotalTime/1000;
		
		static Stopwatch timer = start_stopwatch();


		if(peek_stopwatch(timer) > 10){
			reset_stopwatch(&timer);
			forI(res){
				f64 alignment = (g.cameraPosition.x-g.cameraZoom)+f64(i)/res*g.cameraZoom*2;
				f64& x = data[i].x;
				f64& y = data[i].y;
				x = alignment;
				y = rng()%5000/5000.;
			}
		}

		
		
		draw_graph(&g, UI::GetWindow()->dimensions-UI::GetStyle().windowMargins*2);
		UIItem* gr = UI::GetLastItem();
		
		
		static vec2 mp;
		static vec2 gcp;
		if(UI::IsLastItemHovered() && input_lmouse_pressed()){
			UI::SetPreventInputs();
			mp = input_mouse_position();
			gcp = g.cameraPosition;
		}
		if(mp!=vec2::ONE*FLT_MAX && input_lmouse_down()){
			g.cameraPosition = gcp - (input_mouse_position() - mp) / (vec2g(g.dimensions_per_unit_length.x, g.dimensions_per_unit_length.x*g.aspect_ratio));
			//Log("test",g.cameraPosition.x," ",g.cameraPosition.y);
		}
		if(input_lmouse_released()){
			UI::SetAllowInputs();
			mp=vec2::ONE*FLT_MAX;
		}
		
		g.cameraZoom -= 0.2*g.cameraZoom*DeshInput->scrollY;
		UI::End();
		
	}
}

void update_debug(){
	persist b32 show_metrics = false;
	if(key_pressed(Key_M | InputMod_LctrlLshift)) ToggleBool(show_metrics);
	if(show_metrics) UI::ShowMetricsWindow();
	
	//graph_testing();
	using namespace UI;

	//repulsion();
	//random_draw(200);
	//random_walk_avoid();
	//vector_field();
	//UI::DemoWindow();
	//Storage::StorageBrowserUI();
	//deshi__memory_draw(); //NOTE this is visually one frame behind for memory modified after it is called
}

int main(){
	//init deshi
	memory_init(Gigabytes(1), Gigabytes(1));
	logger_init();
	console_init();
	DeshWindow->Init("suugu", 1280, 720);
	Render::Init();
	Storage::Init();
	UI::Init();
	cmd_init();
	DeshWindow->ShowWindow();
	Render::UseDefaultViewProjMatrix();
	DeshThreadManager->init();
	
	//init suugu
	init_canvas();
	
	//start main loop
	Stopwatch frame_stopwatch = start_stopwatch();
	while(!DeshWindow->ShouldClose()){DPZoneScoped;
		DeshWindow->Update();
		update_canvas();
		//update_debug();
		console_update();
		UI::Update();
		Render::Update();
		logger_update();
		memory_clear_temp();
		DeshTime->frameTime = reset_stopwatch(&frame_stopwatch);
	}
	
	//cleanup deshi
	Render::Cleanup();
	DeshWindow->Cleanup();
	logger_cleanup();
	memory_cleanup();
}