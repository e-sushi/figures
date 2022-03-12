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

`Parser`
--------
implement a system for adding to an already existing AST tree
parser is copying tokens from elements rather than simply viewing them

`Serialization`
--------------
constants loader

`Solver`
--------

`Config`
--------
decimal separator: comma vs point
thousand separator: space vs comma vs point

Bug Board       //NOTE mark these with a last-known active date (MM/DD/YY)
---------
(03/07/22) fix zoom in/out consistency
(03/07/22) the program freezes if ALT is pressed and doesnt resume until the window is moved
(03/07/22) trying to create a new expression when the cursor of another expression is not at the edge causes double input
(03/07/22) element hitbox does not match UI window size
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

void RandBusyWork(u32 time){
	DPZoneScoped;
	PRINTLN("working for " << time << " ms");
	WaitFor(time);
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
	
	//	Window* child = DeshWindow->MakeChild("haha", 500, 500, 10, 10);
	//	Render::RegisterChildWindow(1, child);
	//	child->ShowWindow();
	
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