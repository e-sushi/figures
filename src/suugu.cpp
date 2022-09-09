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
scale graph numbers with zoom

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
[05/18/22](05/18/22) sometimes using + or - messes up the AST and causes it to return a very large number. it fixes after adding another literal
[05/18/22](05/18/22) default expression size should be the size of a character or something
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
#include "core/assets.h"
#include "core/commands.h"
#include "core/console.h"
#include "core/file.h"
#include "core/graphing.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui2.h"
#include "core/window.h"
#include "math/math.h"

//// suugu includes ////
#ifdef SUUGU_USE_GRAPHVIZ
#  include "graphviz/gvc.h"
#endif
#define SUUGU_IMPLEMENTATION
#include "types.h"
#include "functions.cpp"
#include "ast.cpp"
#include "solver.cpp"
#include "canvas.cpp"

#if BUILD_INTERNAL
void update_debug(){
	persist b32 show_metrics = false;
	if(key_pressed(Key_M | InputMod_LctrlLshift)) ToggleBool(show_metrics);
	if(show_metrics) UI::ShowMetricsWindow();
	
	//UI::DemoWindow();
}
#endif

int main(int args_count, char** args){
	//parse cmd line args
	b32 cmdline_solve = false;
	str8_builder cmdline_solve_input;
	if(args_count > 1){
		for(int arg_index = 1; arg_index < args_count; arg_index += 1){
			//solve without creating a window
			if(strcmp("-solve", args[arg_index]) == 0){
				if(args_count < 3){
					printf("Error: Using the -solve option in suugu expects an equation to follow it.\n  eg: suugu.exe -solve 1+2/3");
					return 1;
				}
				
				cmdline_solve = true;
				str8_builder_init(&cmdline_solve_input, str8{(u8*)args[arg_index+1], (s64)strlen(args[arg_index+1])});
				for(arg_index = arg_index+2; arg_index < args_count; arg_index += 1){
					str8_builder_append(&cmdline_solve_input, str8{(u8*)args[arg_index], (s64)strlen(args[arg_index])});
				}
				break;
			}
		}
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	// Headless Solve Mode
	if(cmdline_solve){
		Expression expr{};
		expr.term.type = TermType_Expression;
		expr.raw_cursor_start = 1;
		expr.raw = cmdline_solve_input;
		expr.valid = parse(&expr);
		solve(&expr.term);
		//debug_print_term(&expr.term);
		
		if(expr.equals){
			if(expr.solution == MAX_F64){
				printf("%*s ERROR\n", (int)cmdline_solve_input.count, (const char*)cmdline_solve_input.str);
			}else{
				printf("%*s %g\n", (int)cmdline_solve_input.count, (const char*)cmdline_solve_input.str, expr.solution);
			}
		}else if(expr.solution != MAX_F64){
			printf("%*s = %g\n", (int)cmdline_solve_input.count, (const char*)cmdline_solve_input.str, expr.solution);
		}
		fflush(stdout);
		
		_Exit(0); //dumb c++ and it's destructors
		return 0;
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	// Regular Mode
	//init deshi
	Stopwatch deshi_watch = start_stopwatch();
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	window_create(str8l("suugu"));
	render_init();
	assets_init();
	uiInit(g_memory,0);
	UI::Init();
	console_init();
	cmd_init();
	window_show(DeshWindow);
	render_use_default_camera();
	DeshThreadManager->init();
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");
	//init suugu
	init_canvas();
	
	//start main loop
	while(platform_update()){DPZoneScoped;
		update_canvas();
#if BUILD_INTERNAL
		update_debug();
#endif
		console_update();
		UI::Update();
		uiUpdate();
		render_update();
		logger_update();
		memory_clear_temp();
	}
	
	//cleanup deshi
	render_cleanup();
	logger_cleanup();
	memory_cleanup();
	return 0;
}