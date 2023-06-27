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
// #include "mocompiler.cpp"
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


void test_single_addition() {
	Element* element = create_element();
	*array_push(canvas.element.arr) = element;
	element->type = ElementType_Expression;
	element->x      = canvas.world.mouse_pos.x;
	element->y      = canvas.world.mouse_pos.y;
	element->height = (320*canvas.camera.zoom) / (f32)canvas.ui.root->width;
	element->width  = element->height / 2.0;
	element->type   = ElementType_Expression;
	ui_push_item(canvas.ui.root);
	element->item = ui_make_item(0);
	// TODO(sushi) fix the error with deshi_ui_allocator's resize and then use it here
	element->item->id = to_dstr8v(deshi_allocator, "suugu.canvas.element", array_count(canvas.element.arr)).fin; 
	element->item->style = element_default_style;
	element->item->style.font_height = 30;
	element->item->style.font = canvas.ui.font.debug;
	element->item->style.pos = {100,100}; //{element->x, element->y};
	element->item->__generate = render_element;
	element->item->__evaluate = evaluate_element;
	element->item->style.background_color = Color_Grey;
	element->item->userVar = (u64)element;
	ui_pop_item(1);
	element->expression.handle.term_cursor_start = &element->expression.handle.root;
	element->expression.handle.raw_cursor_start  = 1;
	dstr8_init(&element->expression.handle.raw, str8l(""), deshi_allocator);
	element->expression.handle.root.raw = text_init(str8l(""), deshi_allocator);
	array_init(element->expression.position_map.x, 1, deshi_allocator);
	array_init(element->expression.position_map.y, 1, deshi_allocator);
	array_init(element->expression.rendered_parts, 1, deshi_allocator);
	*array_push(canvas.element.arr) = element;
	canvas.element.selected = element;

	element->expression.handle.root.mathobj = &math_objects.addition;
	text_clear_and_replace(&element->expression.handle.root.raw, str8l("+"));

	Term* lhs = create_term(), *rhs = create_term();
	lhs->mathobj = &math_objects.number;
	rhs->mathobj = &math_objects.number;
	text_clear_and_replace(&rhs->raw, str8l("456"));
	text_clear_and_replace(&lhs->raw, str8l("123"));
	ast_insert_last(&element->expression.handle.root, lhs);
	ast_insert_last(&element->expression.handle.root, rhs);
}

void test_double_addition() {
	Element* element = create_element();
	*array_push(canvas.element.arr) = element;
	element->type = ElementType_Expression;
	element->x      = canvas.world.mouse_pos.x;
	element->y      = canvas.world.mouse_pos.y;
	element->height = (320*canvas.camera.zoom) / (f32)canvas.ui.root->width;
	element->width  = element->height / 2.0;
	element->type   = ElementType_Expression;
	ui_push_item(canvas.ui.root);
	element->item = ui_make_item(0);
	// TODO(sushi) fix the error with deshi_ui_allocator's resize and then use it here
	element->item->id = to_dstr8v(deshi_allocator, "suugu.canvas.element", array_count(canvas.element.arr)).fin; 
	element->item->style = element_default_style;
	element->item->style.font_height = 11;
	element->item->style.font = canvas.ui.font.debug;
	element->item->style.pos = {100,100}; //{element->x, element->y};
	element->item->__generate = render_element;
	element->item->__evaluate = evaluate_element;
	element->item->style.background_color = Color_Grey;
	element->item->userVar = (u64)element;
	ui_pop_item(1);
	element->expression.handle.term_cursor_start = &element->expression.handle.root;
	element->expression.handle.raw_cursor_start  = 1;
	dstr8_init(&element->expression.handle.raw, str8l(""), deshi_allocator);
	element->expression.handle.root.raw = text_init(str8l(""), deshi_allocator);
	array_init(element->expression.position_map.x, 1, deshi_allocator);
	array_init(element->expression.position_map.y, 1, deshi_allocator);
	array_init(element->expression.rendered_parts, 1, deshi_allocator);
	*array_push(canvas.element.arr) = element;
	canvas.element.selected = element;

	element->expression.handle.root.mathobj = &math_objects.addition;
	text_clear_and_replace(&element->expression.handle.root.raw, str8l("+"));

	Term* lhs = create_term(), *rhs = create_term();
	lhs->mathobj = &math_objects.number;
	rhs->mathobj = &math_objects.addition;
	text_clear_and_replace(&rhs->raw, str8l("+"));
	Term* rhslhs = create_term(), *rhsrhs = create_term();
	rhslhs->mathobj = rhsrhs->mathobj = &math_objects.number;
	text_clear_and_replace(&lhs->raw, str8l("123"));
	text_clear_and_replace(&rhslhs->raw, str8l("456"));
	text_clear_and_replace(&rhsrhs->raw, str8l("789"));
	ast_insert_last(&element->expression.handle.root, lhs);
	ast_insert_last(&element->expression.handle.root, rhs);
	ast_insert_last(rhs, rhslhs);
	ast_insert_last(rhs, rhsrhs);
}

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

	

	{
		MathObject& placeholder = math_objects.placeholder;
		placeholder.name = str8l("Placeholder");
		placeholder.description = str8l("suugu's placeholder object, anything may go here.");
		placeholder.type = MathObject_Placeholder;
		placeholder.display.text = str8l("□");

		placeholder.display.instruction_tokens = tokenize_instructions(str8l(
			"render text '□'"
		));
	}

    {
		MathObject& num = math_objects.number;
		num.name = str8l("Number");
    	num.description = str8l("A mathematical object used to count, measure, and label.");
    	num.type = MathObject_Number;

		num.display.instruction_tokens = tokenize_instructions(str8l(
			"render text term_raw"
		));
	}

	{
		MathObject& add = math_objects.addition;
		add.name = str8l("Addition");
		add.description = str8l("The result of taking two groups of objects, putting them together, and then counting the result.");
		add.type = MathObject_Function;
		add.func.arity = 2;

		add.display.instruction_tokens = tokenize_instructions(str8l(
			"render text '+'\n" // stack: 0
			"render child 0\n"  // stack: 1
			"render child 1\n"  // stack: 2
			"align `1 origin_y `0 center_y\n"
			"align `0 left `1 right\n"
			"align `2 origin_y `0 center_y\n"
			"align `2 left `0 right\n"
		));

		add.display.text = str8l("$1 + $2");
		add.display.s_expression = str8l("(+ $1 $2)");
	}

	{
		MathObject& sub = math_objects.subtraction;
		sub.name = str8l("Subtraction");
		sub.description = str8l("The result of taking a group of objects, taking some amount of them away, then counting the group that was taken from.");
		sub.type = MathObject_Function;
		sub.func.arity = 2;

		sub.display.instruction_tokens = tokenize_instructions(str8l(
			"render text '-'\n" // stack: 0
			"render child 0\n"  // stack: 1
			"render child 1\n"  // stack: 2
			"align `1 origin_y `0 center_y\n"
			"align `0 left `1 right\n"
			"align `2 origin_y `0 center_y\n"
			"align `2 left `0 right\n"
		));

		sub.display.text = str8l("$1 - $2");
		sub.display.s_expression = str8l("(- $1 $2)");
	}

	{
		MathObject& mul = math_objects.multiplication;
		mul.name = str8l("Multiplication");
		mul.description = str8l("The result of taking a group of objects, adding some amount of groups of the same size to it, and then counting the result group.");
		mul.type = MathObject_Function;
		mul.func.arity = 2;

		mul.display.instruction_tokens = tokenize_instructions(str8l(
			"render text '*'\n" // stack: 0
			"render child 0\n"  // stack: 1
			"render child 1\n"  // stack: 2
			"align `1 origin_y `0 center_y\n"
			"align `0 left `1 right\n"
			"align `2 origin_y `0 center_y\n"
			"align `2 left `0 right\n"
		));

		mul.display.text = str8l("$1 * $2");
		mul.display.s_expression = str8l("(* $1 $2)");
	}

	{
		MathObject& div = math_objects.division;
		div.name = str8l("Division");
		div.description = str8l("The result of attempting to equally distribute a group of objects into some amount of groups.");
		div.type = MathObject_Function;
		div.func.arity = 2;


		// TODO(sushi) div rendering instructions
		div.display.instruction_tokens = tokenize_instructions(str8l(
			"render child 0\n" // stack: 0
			"render child 1\n" // stack: 1
			"align `1 top `0 bottom\n"
			"align min(`0 center_x, `1 center_x) max(`0 center_x, `1 center_x)\n"
			"render shape line \n"
			"		(min(`0 left, `1 left), avg(`0 bottom, `1 top))\n" 
			"	    (max(`0 right, `1 right), avg(`0 bottom, `1 top))\n"
		));

		div.display.text = str8l("$1 / $2");
		div.display.s_expression = str8l("(/ $1 $2)");
	}

	//compile_math_objects(str8l("scratch"));

	// forI(array_count(math_objects)) {
	// 	Part s = math_objects[i];
	// 	MathObject* mo = s.mathobj;
	// 	Log("", 
	// 		"name: ", mo->name, "\n",
	// 		"desc: ", mo->description, "\n",
	// 		"type: ", mo->type, "\n"
	// 	);
	// }

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

	KeyCode inputs[] = {
		// CanvasBind_SetTool_Expression,
		// CanvasBind_Expression_Create,
		// Key_1,
		// Key_2,
		// Key_3,
		// Key_4,
		// Key_5,
		// Key_6,
		// Key_7,
		// Key_8,
		// Key_9,
		// Key_0,
		// CanvasBind_Expression_CursorLeft,
		// Key_NONE,
		// CanvasBind_Expression_CursorLeft,
		// Key_NONE,
		// CanvasBind_Expression_CursorLeft,
		// Key_NONE,
		// CanvasBind_Expression_CursorLeft,
		// Key_NONE,
		// CanvasBind_Expression_CursorLeft,
		// Key_NONE,
		// CanvasBind_Expression_CursorLeft,
		// Key_NONE,
		// CanvasBind_Expression_CursorLeft,
		// Key_NONE,
		// CanvasBind_Expression_CursorLeft,
		// Key_NONE,
		// CanvasBind_Expression_CursorLeft,
		// Key_NONE,
		// Key_EQUALS|InputMod_AnyShift,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// Key_8|InputMod_AnyShift,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// Key_MINUS,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// Key_EQUALS|InputMod_AnyShift,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// Key_8|InputMod_AnyShift,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// Key_MINUS,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// Key_EQUALS|InputMod_AnyShift,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// Key_8|InputMod_AnyShift,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// CanvasBind_Expression_CursorRight,
		// Key_NONE,
		// Key_MINUS,

		CanvasBind_SetTool_Expression,
		CanvasBind_Expression_Create,
		Key_FORWARDSLASH,
		Key_1,
		Key_DOWN,
		Key_2,
	};

	//test_single_addition();
	//test_double_addition();

	//start main loop
	while(platform_update()){DPZoneScoped;
		//update suugu
		if(DeshTime->frame < ArrayCount(inputs)+1) {
			Log("", "key: ", input_keycode_to_str8(inputs[DeshTime->frame-1]));
			simulate_key_press(inputs[DeshTime->frame-1]);
		}

		update_canvas();
		Element* selected = canvas.element.selected;
		if(selected && array_count(selected->expression.rendered_parts)) {
			static u32 iter = 0;
			if(key_pressed(Key_LEFT|InputMod_AnyCtrl) && iter) iter--;
			if(key_pressed(Key_RIGHT|InputMod_AnyCtrl) && iter < array_count(selected->expression.rendered_parts)-1) iter++; 

			static Stopwatch watch = start_stopwatch();

			vec2 pos = selected->item->pos_screen;
			render_start_cmd2(5, 0, vec2::ZERO, DeshWindow->dimensions.toVec2());
			RenderPart part = selected->expression.rendered_parts[iter];
			render_quad2(pos + part.position, part.bbx, Color_Red);
			ui_begin_immediate_branch(selected->item); {
				uiItem* item = ui_make_text(part.term->raw.buffer.fin, 0);
				item->style.positioning = pos_relative;
				item->style.font_height = 11;
				item->style.pos = part.position.yAdd(part.bbx.y);
			} ui_end_immediate_branch();
		}

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