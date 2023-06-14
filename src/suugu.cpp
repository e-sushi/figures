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
expression mathobj selection with keybinds and mouse

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
#include "kigu/array_utils.h"
#include "kigu/array.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"

//// deshi includes ////
#include "core/assets.h"
#include "core/commands.h"
#include "core/console.h"
#include "core/file.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui_widgets.h"
#include "core/ui_graphing.h"
#include "core/window.h"
#include "math/math.h"

//// external includes ////
#include <stb/stb_ds.h>

//// suugu includes ////
#ifdef SUUGU_USE_GRAPHVIZ
#  include "graphviz/gvc.h"
#endif
#define SUUGU_IMPLEMENTATION
#include "types.h"
#include "mocompiler.cpp"
#include "mint.h"
#include "functions.cpp"
#include "library.cpp"
#include "ast.cpp"
#include "solver.cpp"
#include "canvas.cpp"
#include "suugu_commands.cpp" //NOTE(delle) this should be the last include so it can reference .cpp vars
#if DESHI_LINUX
#  include "unistd.h" // _exit on linux
#endif


int main(int args_count, char** args){
	profiler_init();


	//parse cmd line args
	b32 solve_mode = false;
	b32 interactive_mode = false;
	dstr8 cmdline_solve_input;
	if(args_count > 1){
		for(int arg_index = 1; arg_index < args_count; arg_index += 1){
			//solve without creating a window
			if(strcmp("-solve", args[arg_index]) == 0){
				if(args_count < 3){
					printf("Error: Using the -solve option in suugu expects an equation to follow it.\n  eg: suugu.exe -solve 1+2/3");
					return 1;
				}
				
				solve_mode = true;
				dstr8_init(&cmdline_solve_input, str8{(u8*)args[arg_index+1], (s64)strlen(args[arg_index+1])});
				for(arg_index = arg_index+2; arg_index < args_count; arg_index += 1){
					dstr8_append(&cmdline_solve_input, str8{(u8*)args[arg_index], (s64)strlen(args[arg_index])});
				}
				break;
			}else if(!strcmp("-console", args[arg_index])){
				interactive_mode = true;
				break;
			}
		}
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	// Headless Solve Mode
	if(solve_mode){
		memory_init(Kilobytes(50), Kilobytes(50));
		platform_init();
		logger_init();
		
		Expression expr{};
		expr.root.type = TermType_Expression;
		expr.raw_cursor_start = 1;
		expr.raw = cmdline_solve_input;
		expr.valid = parse(&expr);
		solve(&expr.root);
		//debug_print_term(&expr.mathobj);
		
		if(expr.equals){
			if(expr.unknown_vars){
				//TODO assuming very simple, one-op equations for now (5 = 1 + x; 1 + x = 5)
				//TODO hacky direct usage of solver vars
				Logf("","%.*s = %g", (int)solver_unknown_variables[0]->raw.buffer.count, (const char*)solver_unknown_variables[0]->raw.buffer.str, expr.solution);
			}else{
				if(expr.solution == MAX_F64){
					Logf("","%.*s ERROR", (int)cmdline_solve_input.count, (const char*)cmdline_solve_input.str);
				}else{
					Logf("","%.*s %g", (int)cmdline_solve_input.count, (const char*)cmdline_solve_input.str, expr.solution);
				}
			}
		}else if(expr.solution != MAX_F64){
			Logf("","%.*s = %g", (int)cmdline_solve_input.count, (const char*)cmdline_solve_input.str, expr.solution);
		}
		
		fflush(stdout);
		_exit(solver_error_code); //NOTE(delle) force exit without calling destructors
		return 0;
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	// Headless Interactive Mode
	if(interactive_mode){
		memory_init(Kilobytes(50), Kilobytes(50));
		platform_init();
		logger_init();
		//logger_expose()->auto_newline = false; NOTE(sushi) for some reason clang won't link to this function here, I'll figure it out later
		Log("", "----- ", CyanFormat("suugu"), " -----\n");
		
		char buffer[255];
		while(platform_update()){
			Log("", "> ");
			//TODO(sushi) this way of getting input sucks ass change it later
			char* buffer_stage = (char*)memalloc(1024);
			u32 buffer_size = 1024;
			if(!fgets(buffer_stage, 1024, stdin)){
				Log("", "fgets error pls fix");
			}
			
			str8 buffer = str8{(u8*)buffer_stage, s64(strlen(buffer_stage) - 1)};
			
			if(str8_equal(buffer, STR8("quit"))){
				Log("", "bye");
				platform_exit();
			}else{
				Expression expr{};
				expr.root.type = TermType_Expression;
				expr.raw_cursor_start = 1;
				dstr8_init(&expr.raw, buffer, deshi_temp_allocator);
				expr.valid = parse(&expr);
				// logger_expose()->auto_newline = 1;  NOTE(sushi) for some reason clang won't link to this function here, I'll figure it out later
				debug_print_term(&expr.root);
				// logger_expose()->auto_newline = 0;  NOTE(sushi) for some reason clang won't link to this function here, I'll figure it out later
				Log("",solve(&expr.root),"\n");
			}
			memory_clear_temp();
		}
		return 0;
	}

	//-///////////////////////////////////////////////////////////////////////////////////////////////
	// Regular Mode
	//init deshi
	Stopwatch deshi_watch = start_stopwatch();
	memory_init(Gigabytes(4), Gigabytes(1));
	platform_init();
	logger_init();

	Log("", "\n\n\n\n\n\n-------------------------------------");
	suugu_memory_init();
	
	window_create(str8l("suugu"));
	render_init();
	assets_init();
	ui_init();
	//console_init();
	cmd_init();
	// window_show(DeshWindow);
	render_use_default_camera();
	// threader_init();
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");
	
	//init suugu
	init_canvas();
	init_suugu_commands();

	math_objects = compile_math_objects(str8l("scratch"));

	forI(array_count(math_objects)) {
		Symbol s = math_objects[i];
		MathObject* mo = s.mathobj;
		Log("", 
			"name: ", mo->name, "\n",
			"desc: ", mo->description, "\n",
			"type: ", mo->type, "\n"
		);
	}

#if 0 //mint testing
	mint a = mint_init(20);
	mint b = mint_init(127);
	
	forI(0xffff){
		mint_add(&a, b);
		if     (a.count == 0) Log("", 0);
		else if(a.count == 1) Log("", *( u8*)&a.arr[0]);
		else if(a.count == 2) Log("", *(u16*)&a.arr[0]);
		else if(a.count == 3) Log("", *(u32*)&a.arr[0]);
		else if(a.count == 4) Log("", *(u64*)&a.arr[0]);
	}
#endif
	
	u32 input_idx = 0;
	//start main loop
	while(platform_update()){DPZoneScoped;
		//update suugu

		switch(input_idx) {
			case 0: simulate_key_press(CanvasBind_SetTool_Expression); break;
			case 1: simulate_key_press(CanvasBind_Expression_Create); break;
			case 2: simulate_key_press(Key_EQUALS|InputMod_AnyShift); break;
		}
		if(input_idx < 3) input_idx++;

		update_canvas();
		// //update deshi
		// console_update();
		ui_update();
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