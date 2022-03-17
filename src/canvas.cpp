/* Index:
@tools
@binds
@pencil
@camera
@context
@utility
@graph
@ast
@canvas
@input
@input_tool
@input_navigation
@input_context
@input_expression
@input_expression_cursor
@input_expression_literals
@input_expression_operators
@input_expression_letters
@input_pencil
@draw_elements
@draw_elements_expression
@draw_elements_graph
@draw_elements_workspace
@draw_elements_text
@draw_pencil
@draw_canvas_info
*/

////////////////
//// @tools ////
////////////////
enum CanvasTool : u32{
	CanvasTool_Navigation,
	CanvasTool_Context,
	CanvasTool_Expression,
	CanvasTool_Pencil,
};
local const char* canvas_tool_strings[] = {
	"Navigation", "Context", "Expression", "Pencil",
};

local CanvasTool active_tool   = CanvasTool_Navigation;
local CanvasTool previous_tool = CanvasTool_Navigation;

////////////////
//// @binds ////
////////////////
enum CanvasBind_{
	//[GLOBAL] SetTool
	CanvasBind_SetTool_Navigation = Key::ESCAPE  | InputMod_Any,
	CanvasBind_SetTool_Context    = Key::MBRIGHT | InputMod_AnyCtrl,
	CanvasBind_SetTool_Expression = Key::E       | InputMod_AnyCtrl, //NOTE temp making this CTRL+E for simplicity
	CanvasBind_SetTool_Pencil     = Key::P       | InputMod_AnyCtrl,
	CanvasBind_SetTool_Graph      = Key::G       | InputMod_AnyCtrl,
	CanvasBind_SetTool_Previous   = Key::MBFOUR  | InputMod_None,
	
	//[GLOBAL] Camera 
	CanvasBind_Camera_Pan     = Key::MBMIDDLE     | InputMod_None, //pressed, held
	CanvasBind_Camera_ZoomIn  = Key::MBSCROLLUP   | InputMod_None,
	CanvasBind_Camera_ZoomOut = Key::MBSCROLLDOWN | InputMod_None,
	
	//[LOCAL]  Navigation 
	CanvasBind_Navigation_Pan       = Key::MBLEFT  | InputMod_Any, //pressed, held
	CanvasBind_Navigation_ResetPos  = Key::NUMPAD0 | InputMod_None,
	CanvasBind_Navigation_ResetZoom = Key::NUMPAD0 | InputMod_None,
	
	//[LOCAL]  Expression
	CanvasBind_Expression_Select      = Key::MBLEFT    | InputMod_None, //pressed
	CanvasBind_Expression_Create      = Key::MBRIGHT   | InputMod_None,
	CanvasBind_Expression_CursorLeft  = Key::LEFT      | InputMod_None,
	CanvasBind_Expression_CursorRight = Key::RIGHT     | InputMod_None,
	CanvasBind_Expression_CursorUp    = Key::UP        | InputMod_None,
	CanvasBind_Expression_CursorDown  = Key::DOWN      | InputMod_None,
	CanvasBind_Expression_CursorDeleteLeft = Key::BACKSPACE | InputMod_None,
	
	//[LOCAL]  Pencil
	CanvasBind_Pencil_Stroke             = Key::MBLEFT       | InputMod_Any, //pressed, held
	CanvasBind_Pencil_SizeIncrementBy1   = Key::MBSCROLLUP   | InputMod_AnyShift,
	CanvasBind_Pencil_SizeIncrementBy5   = Key::MBSCROLLUP   | InputMod_AnyCtrl,
	CanvasBind_Pencil_SizeDecrementBy1   = Key::MBSCROLLDOWN | InputMod_AnyShift,
	CanvasBind_Pencil_SizeDecrementBy5   = Key::MBSCROLLDOWN | InputMod_AnyCtrl,
	CanvasBind_Pencil_DeletePrevious     = Key::Z            | InputMod_AnyCtrl,
	CanvasBind_Pencil_DetailIncrementBy1 = Key::EQUALS       | InputMod_None,
	CanvasBind_Pencil_DetailIncrementBy5 = Key::EQUALS       | InputMod_AnyShift,
	CanvasBind_Pencil_DetailDecrementBy1 = Key::MINUS        | InputMod_None,
	CanvasBind_Pencil_DetailDecrementBy5 = Key::MINUS        | InputMod_AnyShift,
}; typedef Key::Key CanvasBind;


/////////////////
//// @pencil ////
/////////////////
struct PencilStroke{
	f32   size;
	color color;
	array<vec2f64> pencil_points;
};

local array<PencilStroke> pencil_strokes;
local u32     pencil_stroke_idx  = 0;
local f32     pencil_stroke_size = 1;
local color   pencil_stroke_color = PackColorU32(249,195,69,255);
local vec2f64 pencil_stroke_start_pos;
local u32     pencil_draw_skip_amount = 4;


/////////////////
//// @camera ////
/////////////////
local vec2f64 camera_pos{0,0};
local f64     camera_zoom = 1.0;
local vec2f64 camera_pan_start_pos;
local vec2    camera_pan_mouse_pos;
local b32     camera_pan_active = false;


//////////////////
//// @context ////
//////////////////
local char context_input_buffer[256] = {};
local u32 context_dropdown_selected_index = 0;
local const char* context_dropdown_option_strings[] = {
	"Tool: Navigation", "Tool: Expression", "Tool: Pencil",
	"Add: Graph",
};


//////////////////
//// @utility ////
//////////////////
local vec2 
ToScreen(vec2f64 point){
	point -= camera_pos;
	point /= camera_zoom;
	point.y *= -f64(DeshWindow->width) / f64(DeshWindow->height);
	point += vec2f64{1.0, 1.0};
	point.x *= f64(DeshWindow->dimensions.x); point.y *= f64(DeshWindow->dimensions.y);
	point /= 2.0;
	return vec2(point.x, point.y);
}FORCE_INLINE vec2 ToScreen(f64 x, f64 y){ return ToScreen({x,y}); }

local vec2f64 
ToWorld(vec2 _point){
	vec2f64 point{_point.x, _point.y};
	point.x /= f64(DeshWindow->dimensions.x); point.y /= f64(DeshWindow->dimensions.y);
	point *= 2.0;
	point -= vec2f64{1.0, 1.0};
	point.y /= -f64(DeshWindow->width) / f64(DeshWindow->height);
	point *= camera_zoom;
	point += camera_pos;
	return point;
}FORCE_INLINE vec2f64 ToWorld(f32 x, f32 y){ return ToWorld({x,y}); }

//returns the width and height of the area in world space that the user can currently see as a vec2
local vec2 
WorldViewArea(){
	return vec2(2 * camera_zoom, 2 * camera_zoom * (float)DeshWindow->height / DeshWindow->width);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graph
//local vec2f64
//WorldToGraph(vec2f64 point, Graph* graph){
//	point += graph->cameraPosition;
//	point *= graph->cameraZoom;
//	return point;
//}FORCE_INLINE vec2f64 WorldToGraph(f64 x, f64 y, Graph* graph){ return WorldToGraph({x,y},graph); }
//
//local vec2f64
//GraphToWorld(vec2f64 point, Graph* graph){
//	point /= graph->cameraZoom;
//	point -= graph->cameraPosition;
//	return point;
//}FORCE_INLINE vec2f64 GraphToWorld(f64 x, f64 y, Graph* graph){ return GraphToWorld({x,y},graph); }
//
//inline vec2
//GraphToScreen(vec2f64 point, Graph* graph){
//	return ToScreen(GraphToWorld(point, graph));
//}FORCE_INLINE vec2 GraphToScreen(f64 x, f64 y, Graph* graph){ return GraphToScreen({x,y},graph); }
//
//local void 
//DrawGraphGrid(Graph* graph){
//	//draw border
//	vec2f64 tl_world = graph->position - (graph->dimensions/2.f); // -x, +y
//	vec2f64 br_world = graph->position + (graph->dimensions/2.f); // +x, -y
//	UI::Line(ToScreen(tl_world.x,tl_world.y), ToScreen(br_world.x,tl_world.y), 1, PackColorU32(255,255,255,128));
//	UI::Line(ToScreen(br_world.x,tl_world.y), ToScreen(br_world.x,br_world.y), 1, PackColorU32(255,255,255,128));
//	UI::Line(ToScreen(br_world.x,br_world.y), ToScreen(tl_world.x,br_world.y), 1, PackColorU32(255,255,255,128));
//	UI::Line(ToScreen(tl_world.x,br_world.y), ToScreen(tl_world.x,tl_world.y), 1, PackColorU32(255,255,255,128));
//	
//	vec2f64 tl = WorldToGraph(tl_world, graph);
//	vec2f64 br = WorldToGraph(br_world, graph);
//	//round to nearest multiple of major_increment (favoring away from zero)
//	f64 tl_x = floor(f64(tl.x) / graph->gridMajorLinesIncrement) * graph->gridMajorLinesIncrement;
//	f64 tl_y = ceil (f64(tl.y) / graph->gridMajorLinesIncrement) * graph->gridMajorLinesIncrement;
//	f64 br_x = ceil (f64(br.x) / graph->gridMajorLinesIncrement) * graph->gridMajorLinesIncrement;
//	f64 br_y = floor(f64(br.y) / graph->gridMajorLinesIncrement) * graph->gridMajorLinesIncrement;
//	
//	//draw grid lines //TODO try to combine these loops
//	//TODO fix the graph lines overlapping the border
//	for(f64 x = tl_x; x < br_x; x += graph->gridMajorLinesIncrement){
//		int minor_idx = 0;
//		
//		if(x >= tl.x && x <= br.x){
//			if(x == 0){
//				UI::Line(GraphToScreen(0,tl.y,graph), GraphToScreen(0,br.y,graph), 1, Color_Green);
//			}else if(0 >= br.y && 0 <= tl.y){
//				if(graph->gridShowMajorLines){
//					UI::Line(GraphToScreen(x,tl.y,graph), GraphToScreen(x,br.y,graph), 1, PackColorU32(255,255,255,64));
//					minor_idx = 1;
//				}
//				if(graph->gridShowAxisCoords){
//					UI::PushColor(UIStyleCol_Text, color(255, 255, 255, 128));
//					UI::Text(stringf("%g",x).str, GraphToScreen(x,0,graph), UITextFlags_NoWrap);
//					UI::PopColor();
//				}
//			}
//		}
//		
//		if(graph->gridShowMinorLines){
//			for(; minor_idx <= graph->gridMinorLinesCount; minor_idx++){
//				f64 minor_x = x + (minor_idx * graph->gridMinorLinesIncrement);
//				if(minor_x <= tl.x || minor_x >= br.x) continue;
//				UI::Line(GraphToScreen(minor_x,tl.y,graph), GraphToScreen(minor_x,br.y,graph), 1, PackColorU32(255,255,255,32));
//			}
//		}
//	}
//	for(f64 y = tl_y; y > br_y; y -= graph->gridMajorLinesIncrement){
//		int minor_idx = 0;
//		
//		if(y >= br.y && y <= tl.y){
//			if(y == 0){
//				UI::Line(GraphToScreen(tl.x,0,graph), GraphToScreen(br.x,0,graph), 1, Color_Red);
//			}else if(0 >= tl.x && 0 <= br.x){
//				if(graph->gridShowMajorLines){
//					UI::Line(GraphToScreen(tl.x,y,graph), GraphToScreen(br.x,y,graph), 1, PackColorU32(255,255,255,64));
//					minor_idx = 1;
//				}
//				if(graph->gridShowAxisCoords){
//					UI::PushColor(UIStyleCol_Text, color(255, 255, 255, 128));
//					UI::Text(stringf("%g",y).str, GraphToScreen(0,y,graph), UITextFlags_NoWrap);
//					UI::PopColor();
//				}
//			}
//		}
//		
//		if(graph->gridShowMinorLines){
//			for(; minor_idx <= graph->gridMinorLinesCount; minor_idx++){
//				f64 minor_y = y - (minor_idx * graph->gridMinorLinesIncrement);
//				if(minor_y <= br.y || minor_y >= tl.y) continue;
//				UI::Line(GraphToScreen(tl.x,minor_y,graph), GraphToScreen(br.x,minor_y,graph), 1, PackColorU32(255,255,255,32));
//			}
//		}
//	}
//	
//	//draw zero text
//	if(0 >= tl.x && 0 <= br.x && 0 >= br.y && 0 <= tl.y){
//		UI::PushColor(UIStyleCol_Text, color(255, 255, 255, 128));
//		UI::Text("0", GraphToScreen(0,0,graph), UITextFlags_NoWrap);
//		UI::PopColor();
//	}
//}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @ast
#define PRINT_AST true
local s32 debug_print_indent = -1;
local b32 debug_print_toggle = false;
#if PRINT_AST
void debug_print_term(const char* symbol, b32 is_cursor, Term* term){
	if(debug_print_toggle){
		string indent(deshi_temp_allocator); forI(debug_print_indent) indent += "  ";
		char* arg = (HasFlag(term->flags, TermFlag_OpArgLeft)) ? " L"
			: (HasFlag(term->flags, TermFlag_OpArgRight) ) ? " R"
			: (HasFlag(term->flags, TermFlag_OpArgTop)   ) ? " T"
			: (HasFlag(term->flags, TermFlag_OpArgBottom)) ? " B"
			: "  ";
		char* cursor = (is_cursor) ? " <- ": "    ";
		Log("ast", indent, symbol, arg, cursor, term->left,",",term,",",term->right);
	}
}
#else
#  define debug_print_term(symbol,is_cursor,left) (void)0
#endif

//TODO this makes bad assumptions about the order of child terms to operators
//  fix this when we support left-dangling operators and term deletion
//TODO remove duplication
b32 active_expression = false;
void draw_term(Term* term, Term* cursor){
	vec2 cursor_start;
	f32 cursor_y;
	
	debug_print_indent++;
	switch(term->type){
		case TermType_Expression:{
			Expression2* expr = ExpressionFromTerm(term);
			UI::Text(" ", UITextFlags_NoWrap);
			cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(); cursor_y = UI::GetLastItemSize().y; 
			UI::SameLine();
			if(term->child_count){
				draw_term(term->first_child, cursor);
				UI::PushColor(UIStyleCol_Text, Color_Grey);
				if(expr->equals && expr->valid){
					debug_print_term(to_string(expr->solution, true, deshi_temp_allocator).str, term == cursor, term);
					UI::Text((expr->solution == MAX_F32) ? "ERROR" : to_string(expr->solution, true, deshi_temp_allocator).str, UITextFlags_NoWrap);
					UI::SameLine();
				}else if(expr->solution != MAX_F32 && term->first_child->type != TermType_Literal && expr->valid){
					debug_print_term((string("=", deshi_temp_allocator) + to_string(expr->solution, true, deshi_temp_allocator)).str, term == cursor, term);
					UI::Text("=", UITextFlags_NoWrap); UI::SameLine();
					UI::Text(to_string(expr->solution, true, deshi_temp_allocator).str, UITextFlags_NoWrap);
					UI::SameLine();
				}
				UI::PopColor();
			}
			UI::Text(" ", UITextFlags_NoWrap);
		}break;
		
		case TermType_Operator:{
			Operator* op = OperatorFromTerm(term);
			switch(op->type){
				case OpType_Addition:{
					debug_print_term("+", term == cursor, term);
					draw_term(term->first_child, cursor);
					UI::Text("+", UITextFlags_NoWrap);
					cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(); cursor_y = UI::GetLastItemSize().y;
					UI::SameLine();
					if(term->child_count > 1){
						draw_term(term->last_child, cursor);
					}
				}break;
				
				case OpType_Subtraction:{
					debug_print_term("-", term == cursor, term);
					draw_term(term->first_child, cursor);
					UI::Text("-", UITextFlags_NoWrap);
					cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(); cursor_y = UI::GetLastItemSize().y;
					UI::SameLine();
					if(term->child_count > 1){
						draw_term(term->last_child, cursor);
					}
				}break;
				
				case OpType_ExplicitMultiplication:{
					debug_print_term("*", term == cursor, term);
					draw_term(term->first_child, cursor);
					UI::Text("*", UITextFlags_NoWrap);
					cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(); cursor_y = UI::GetLastItemSize().y;
					UI::SameLine();
					if(term->child_count > 1){
						draw_term(term->last_child, cursor);
					}
				}break;
				
				case OpType_Division:{
					debug_print_term("/", term == cursor, term);
					draw_term(term->first_child, cursor);
					UI::Text("/", UITextFlags_NoWrap);
					cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(); cursor_y = UI::GetLastItemSize().y;
					UI::SameLine();
					if(term->child_count > 1){
						draw_term(term->last_child, cursor);
					}
				}break;
				
				case OpType_ExpressionEquals:{
					debug_print_term("=", term == cursor, term);
					draw_term(term->first_child, cursor);
					UI::Text("=", UITextFlags_NoWrap);
					cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(); cursor_y = UI::GetLastItemSize().y;
					UI::SameLine();
					if(term->child_count > 1){
						draw_term(term->last_child, cursor);
					}
				}break;
			}
		}break;
		
		case TermType_Literal:{
			Literal* lit = LiteralFromTerm(term);
			debug_print_term(to_string(lit->value, true, deshi_temp_allocator).str, term == cursor, term);
			UI::Text(to_string(lit->value, true, deshi_temp_allocator).str, UITextFlags_NoWrap);
			if(lit->decimal == 1){
				UI::SameLine();
				UI::Text(".", UITextFlags_NoWrap); //TODO decimal config here
			}
			UI::SameLine();
			cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(); cursor_y = UI::GetLastItemSize().y;
		}break;
		
		//case TermType_Variable:{}break;
		//case TermType_FunctionCall:{}break;
	}
	debug_print_indent--;
	
	if(active_expression && term == cursor){
		UI::Line(cursor_start, cursor_start - vec2{0,cursor_y}, 2, Color_White * abs(sin(DeshTime->totalTime)));
	}
}

Operator* make_operator(OpType type, Term* cursor){
	Operator* op = (Operator*)memory_alloc(sizeof(Operator)); //TODO expression arena
	op->type = type;
	op->term.type = TermType_Operator;
	
	//loop until we find a lesser precedence operator since it should be higher vertically in the tree
	while((cursor->parent->type == TermType_Operator) && (*op <= OperatorFromTerm(cursor->parent))){
		cursor = cursor->parent;
	}
	
	//operator inherits cursor's OpArg flags
	op->term.flags = (cursor->flags & OPARG_MASK);
	
	//cursor's parent is not a greater precedence operator, so make cursor a child of us
	insert_after(cursor, &op->term);
	cursor->next = &op->term;
	op->term.prev = cursor;
	
	op->term.parent = cursor->parent;
	if(cursor == cursor->parent->last_child) cursor->parent->last_child = &op->term;
	cursor->parent->child_count++;
	change_parent(&op->term, cursor);
	
	//remove cursor's old OpArg flags
	RemoveFlag(cursor->flags, OPARG_MASK);
	AddFlag(cursor->flags, TermFlag_OpArgLeft);
	
	return op;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @canvas
local array<Element2*> elements(deshi_allocator);
local Element2* selected_element;
local vec2f64 mouse_pos_world;
local Font* math_font;

void init_canvas(){
	//TODO default graph
	
	math_font = Storage::CreateFontFromFileTTF("STIXTwoMath-Regular.otf", 100).second;
	Assert(math_font != Storage::NullFont(), "Canvas math font failed to load");
}

void update_canvas(){
	UI::PushVar(UIStyleVar_WindowMargins, vec2::ZERO);
	UI::SetNextWindowSize(DeshWindow->dimensions);
	UI::Begin("canvas", vec2::ZERO, vec2::ZERO, UIWindowFlags_Invisible | UIWindowFlags_NoInteract);
	
	{//// @input ////
		mouse_pos_world = ToWorld(DeshInput->mousePos);
		
		///////////////////////////////////////////////////////////////////////////////////////////////
		//// @input_tool
		if     (DeshInput->KeyPressed(CanvasBind_SetTool_Navigation)){ previous_tool = active_tool; active_tool = CanvasTool_Navigation; }
		else if(DeshInput->KeyPressed(CanvasBind_SetTool_Context))   { previous_tool = active_tool; active_tool = CanvasTool_Context; }
		else if(DeshInput->KeyPressed(CanvasBind_SetTool_Expression)){ previous_tool = active_tool; active_tool = CanvasTool_Expression; }
		else if(DeshInput->KeyPressed(CanvasBind_SetTool_Pencil))    { previous_tool = active_tool; active_tool = CanvasTool_Pencil; }
		//else if(DeshInput->KeyPressed(CanvasBind_SetTool_Graph))     { activeGraph = (activeGraph) ? 0 : graphs.data; }
		else if(DeshInput->KeyPressed(CanvasBind_SetTool_Previous))  { Swap(previous_tool, active_tool); }
		
		///////////////////////////////////////////////////////////////////////////////////////////////
		//// @input_camera
		if(DeshInput->KeyPressed(CanvasBind_Camera_Pan)){
			camera_pan_active = true;
			camera_pan_mouse_pos = DeshInput->mousePos;
			camera_pan_start_pos = camera_pos;
		}
		if(DeshInput->KeyDown(CanvasBind_Camera_Pan)){
			camera_pos = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - mouse_pos_world);
		}
		if(DeshInput->KeyReleased(CanvasBind_Camera_Pan)){
			camera_pan_active = false;
		}
		//TODO(delle) fix zoom consistency: out -> in -> out should return to orig value
		//TODO(delle) combine zoom in and out checks and reimplement graph
		
		if(DeshInput->scrollY != 0 && DeshInput->ModsDown(InputMod_None) && !UI::AnyWinHovered()){ //TEMP until graph is reimplemented
			camera_zoom -= (camera_zoom / 10.0) * DeshInput->scrollY;
			camera_zoom = Clamp(camera_zoom, 1e-37, 1e37);
		}
		/*if(DeshInput->KeyDown(CanvasBind_Camera_ZoomOut | InputMod_None) && !UI::AnyWinHovered()){
			if(selected_element && selected_element->type != ElementType_Graph){
				camera_zoom -= camera_zoom / 10.0 * DeshInput->scrollY;
				camera_zoom = Clamp(camera_zoom, 1e-37, 1e37);
			}
			else{
				activeGraph->cameraZoom -= activeGraph->cameraZoom / 10.0; 
				activeGraph->cameraZoom  = Clamp(activeGraph->cameraZoom, 1e-37, 1e37);
				
				f32 prev_grid_zoom_fit = 0;
				if(activeGraph->gridZoomFitIncrementIndex == 0){
					prev_grid_zoom_fit = activeGraph->gridZoomFit / activeGraph->gridZoomFitIncrements[2];
				}else{
					prev_grid_zoom_fit = activeGraph->gridZoomFit / activeGraph->gridZoomFitIncrements[activeGraph->gridZoomFitIncrementIndex - 1];
				}
				
				if(activeGraph->cameraZoom < (prev_grid_zoom_fit + activeGraph->gridMajorLinesIncrement)){
					activeGraph->gridZoomFit                = prev_grid_zoom_fit;
					activeGraph->gridMajorLinesIncrement    = activeGraph->gridZoomFit / 5.0;
					activeGraph->gridMinorLinesCount        = (activeGraph->gridZoomFitIncrementIndex == 2) ? 3 : 4;
					activeGraph->gridMinorLinesIncrement    = activeGraph->gridMajorLinesIncrement / f32(activeGraph->gridMinorLinesCount + 1);
					activeGraph->gridZoomFitIncrementIndex -= 1;
					if(activeGraph->gridZoomFitIncrementIndex == -1) activeGraph->gridZoomFitIncrementIndex = 2;
					Assert(activeGraph->gridZoomFitIncrementIndex < 3);
				}
			}
		}
		if(DeshInput->KeyDown(CanvasBind_Camera_ZoomIn | InputMod_None) && !UI::AnyWinHovered()){
			if(selected_element && selected_element->type != ElementType_Graph){
				camera_zoom -= camera_zoom / 10.0 * DeshInput->scrollY;
				camera_zoom = Clamp(camera_zoom, 1e-37, 1e37);
			}
			else{
				activeGraph->cameraZoom += activeGraph->cameraZoom / 10.0; 
				activeGraph->cameraZoom  = Clamp(activeGraph->cameraZoom, 1e-37, 1e37);
				
				if(activeGraph->cameraZoom > (activeGraph->gridZoomFit + activeGraph->gridMajorLinesIncrement)){
					activeGraph->gridZoomFit              *= activeGraph->gridZoomFitIncrements[activeGraph->gridZoomFitIncrementIndex];
					activeGraph->gridMajorLinesIncrement   = activeGraph->gridZoomFit / 5.0;
					activeGraph->gridMinorLinesCount       = (activeGraph->gridZoomFitIncrementIndex == 0) ? 3 : 4;
					activeGraph->gridMinorLinesIncrement   = activeGraph->gridMajorLinesIncrement / f32(activeGraph->gridMinorLinesCount + 1);
					activeGraph->gridZoomFitIncrementIndex = (activeGraph->gridZoomFitIncrementIndex + 1) % 3;
					Assert(activeGraph->gridZoomFitIncrementIndex < 3);
				}
			}
	}*/
		
#if 1
		if(active_tool == CanvasTool_Pencil){
			UI::Begin("pencil_debug", {200,10}, {200,200}, UIWindowFlags_FitAllElements);
			UI::TextF("Stroke Size:   %f", pencil_stroke_size);
			UI::TextF("Stroke Color:  %x", pencil_stroke_color.rgba);
			UI::TextF("Stroke Start:  (%g,%g)", pencil_stroke_start_pos.x, pencil_stroke_start_pos.y);
			UI::TextF("Stroke Index:  %d", pencil_stroke_idx);
			UI::TextF("Stroke Skip:   %d", pencil_draw_skip_amount);
			if(pencil_stroke_idx > 0) UI::TextF("Stroke Points: %d", pencil_strokes[pencil_stroke_idx-1].pencil_points.count);
			u32 total_stroke_points = 0;
			forE(pencil_strokes) total_stroke_points += it->pencil_points.count;
			UI::TextF("Total Points:  %d", total_stroke_points);
			UI::End();
		}
		if(active_tool == CanvasTool_Expression){
			UI::Begin("expression_debug", {200,10}, {200,200}, UIWindowFlags_FitAllElements);
			UI::TextF("Elements: %d", elements.count);
			if(selected_element){
				UI::TextF("Selected: %#x", selected_element);
				UI::TextF("Position: (%g,%g)", selected_element->x,selected_element->y);
				UI::TextF("Size:     (%g,%g)", selected_element->width,selected_element->height);
				UI::TextF("Cursor:   %#x", (selected_element) ? ((Expression2*)selected_element)->cursor : 0);
			}
			UI::End();
		}
#endif
		
		switch(active_tool){
			////////////////////////////////////////////////////////////////////////////////////////////////
			//// @input_navigation
			case CanvasTool_Navigation: if(!UI::AnyWinHovered()){
				if(DeshInput->KeyPressed(CanvasBind_Navigation_Pan)){
					camera_pan_active = true;
					camera_pan_mouse_pos = DeshInput->mousePos;
					
					camera_pan_start_pos = camera_pos; //TEMP until graph is reimplemented
					/*if(!activeGraph){
						camera_pan_start_pos = camera_pos;
					}else{
						camera_pan_start_pos = activeGraph->cameraPosition;
					}*/
				}
				if(DeshInput->KeyDown(CanvasBind_Navigation_Pan)){
					camera_pos = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - mouse_pos_world); //TEMP until graph is reimplemented
					/*if(!activeGraph){
						camera_pos = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - mouse_pos_world);
					}else{
						activeGraph->cameraPosition = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - mouse_pos_world);
					}*/
				}
				if(DeshInput->KeyReleased(CanvasBind_Navigation_Pan)){
					camera_pan_active = false;
				}
				if(DeshInput->KeyPressed(CanvasBind_Navigation_ResetPos)){
					camera_pos = {0,0}; //TEMP until graph is reimplemented
					/*if(!activeGraph){
						camera_pos = {0,0};
					}else{
						activeGraph->cameraPosition = {0,0};
					}*/
				}
				if(DeshInput->KeyPressed(CanvasBind_Navigation_ResetZoom)){
					camera_zoom = 1.0; //TEMP until graph is reimplemented
					/*if(!activeGraph){
						camera_zoom = 1.0;
					}else{
						activeGraph->cameraZoom = 1.0;
					}*/
				}
			}break;
			
			////////////////////////////////////////////////////////////////////////////////////////////////
			//// @input_context
			case CanvasTool_Context:{
				//if(UI::BeginContextMenu("canvas_context_menu")){
				//UI::EndContextMenu();
				//}
			}break;
			
			////////////////////////////////////////////////////////////////////////////////////////////////
			//// @input_expression
			case CanvasTool_Expression: if(!UI::AnyWinHovered()){
				if(DeshInput->KeyPressed(CanvasBind_Expression_Select)){
					selected_element = 0;
					//TODO inverse the space transformation here since mouse pos is screen space, which is less precise being
					//  elevated to higher precision, instead of higher precision world space getting transformed to screen space
					for(Element2* it : elements){
						if(   mouse_pos_world.x >= it->x
						   && mouse_pos_world.y >= it->y
						   && mouse_pos_world.x <= it->x + it->width
						   && mouse_pos_world.y <= it->y + it->height){
							selected_element = it;
							break;
						}
					}
				}
				
				if(DeshInput->KeyPressed(CanvasBind_Expression_Create)){
					Expression2* expr = (Expression2*)memory_alloc(sizeof(Expression2)); //TODO expression arena
					expr->element.x      = mouse_pos_world.x;
					expr->element.y      = mouse_pos_world.y;
					expr->element.height = (320*camera_zoom) / (f32)DeshWindow->width;
					expr->element.width  = expr->element.height / 2.0;
					expr->element.type   = ElementType_Expression;
					expr->term.type = TermType_Expression;
					expr->cursor = &expr->term;
					
					elements.add(&expr->element);
					selected_element = &expr->element;
				}
				
				if(selected_element && selected_element->type == ElementType_Expression){
					Expression2* expr = ElementToExpression(selected_element);
					b32 ast_changed = false;
					
					//// @input_expression_cursor ////
					if(expr->cursor && DeshInput->KeyPressed(CanvasBind_Expression_CursorDeleteLeft)){
						switch(expr->cursor->type){
							case TermType_Expression:{
								//TODO expression deletion
							}break;
							case TermType_Operator:{
								ast_changed = true;
								Operator* op = OperatorFromTerm(expr->cursor);
								Term* left = expr->cursor->left;
								remove(expr->cursor);
								remove_leftright(expr->cursor);
								memory_zfree(op);
								//TODO cursor movement will mean non-right dangling terms, so its not guarenteed a valid AST then
								if(expr->cursor == expr->equals){
									expr->equals = 0;
								}else{
									expr->valid = true;
								}
								expr->cursor = left;
							}break;
							case TermType_Literal:{
								ast_changed = true;
								Literal* lit = LiteralFromTerm(expr->cursor);
								Term* left = expr->cursor->left;
								remove(expr->cursor);
								remove_leftright(expr->cursor);
								memory_zfree(lit);
								expr->valid = false;
								expr->cursor = left;
							}break;
						}
					}
					
					if(expr->cursor && expr->cursor->left && DeshInput->KeyPressed(CanvasBind_Expression_CursorLeft)){
						ast_changed = true;
						expr->cursor = expr->cursor->left;
					}
					
					if(expr->cursor && expr->cursor->right && DeshInput->KeyPressed(CanvasBind_Expression_CursorRight)){
						ast_changed = true;
						expr->cursor = expr->cursor->right;
					}
					
					//TODO support Unicode using iswdigit()/iswalpha() once we handle it in DeshInput->charIn
					//TODO maybe make the default cursor point to expression?
					b32 first_term = (expr->cursor == &expr->term);
					forI(DeshInput->charCount){
						char input = DeshInput->charIn[i];
						
						//// @input_expression_literals ////
						//TODO remove duplication
						//TODO left-to-right and precedence is invalid currently when placing operator after a literal which is part of another operator
						//  fix this by checking if a literal is a child of an existing operator, then check precedence in order to rearrage the AST
						if(isdigit(input)){
							if(first_term){
								ast_changed = true;
								Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
								lit->term.type = TermType_Literal;
								insert_first(&expr->term, &lit->term);
								insert_right(&expr->term, &lit->term);
								expr->cursor = &lit->term;
							}
							
							if(expr->cursor->type == TermType_Literal){ //appending to a literal
								Literal* lit = LiteralFromTerm(expr->cursor);
								if(lit->decimal){ //we are appending as decimal values
									lit->value = lit->value + (input-48)/pow(10,lit->decimal);
									lit->decimal++;
								}else{            //we are appending as integer values
									lit->value = 10*lit->value + (input-48);
								}
							}else if(expr->cursor->type == TermType_Operator){ //right side of operator //TODO non-binary operators
								ast_changed = true;
								Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
								lit->term.type  = TermType_Literal;
								lit->term.flags = TermFlag_OpArgRight;
								insert_last(expr->cursor, &lit->term);
								insert_right(expr->cursor, &lit->term);
								expr->cursor = &lit->term;
								expr->valid = true;
								
								if(lit->decimal){ //we are appending as decimal values
									lit->value = lit->value + (input-48)/pow(10,lit->decimal);
									lit->decimal++;
								}else{            //we are appending as integer values
									lit->value = 10*lit->value + (input-48);
								}
							}
						}
						else if(input == '.'){ //TODO comma in EU input mode
							if(first_term){
								ast_changed = true;
								Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
								lit->term.type = TermType_Literal;
								insert_first(&expr->term, &lit->term);
								insert_right(&expr->term, &lit->term);
								expr->cursor = &lit->term;
							}
							
							if(expr->cursor->type == TermType_Literal){
								Literal* lit = LiteralFromTerm(expr->cursor);
								if(lit->decimal == 0) lit->decimal = 1;
							}else if(expr->cursor->type == TermType_Operator){ //right side of operator //TODO non-binary operators
								ast_changed = true;
								Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
								lit->term.type  = TermType_Literal;
								lit->term.flags = TermFlag_OpArgRight;
								insert_last(expr->cursor, &lit->term);
								insert_right(expr->cursor, &lit->term);
								expr->cursor = &lit->term;
								expr->valid = true;
								
								if(lit->decimal == 0) lit->decimal = 1;
							}
						}
						
						//// @input_expression_operators ////
						else if(input == '+'){
							if(!first_term && expr->cursor->type == TermType_Literal){
								ast_changed = true;
								Operator* op = make_operator(OpType_Addition, expr->cursor);
								insert_right(expr->cursor, &op->term);
								expr->cursor = &op->term;
								expr->valid = false;
							}
						}
						else if(input == '-'){
							if(!first_term && expr->cursor->type == TermType_Literal){
								ast_changed = true;
								Operator* op = make_operator(OpType_Subtraction, expr->cursor);
								insert_right(expr->cursor, &op->term);
								expr->cursor = &op->term;
								expr->valid = false;
							}
						}
						else if(input == '*'){
							if(!first_term && expr->cursor->type == TermType_Literal){
								ast_changed = true;
								Operator* op = make_operator(OpType_ExplicitMultiplication, expr->cursor);
								insert_right(expr->cursor, &op->term);
								expr->cursor = &op->term;
								expr->valid = false;
							}
						}
						else if(input == '/'){
							if(!first_term && expr->cursor->type == TermType_Literal){
								ast_changed = true;
								Operator* op = make_operator(OpType_Division, expr->cursor);
								insert_right(expr->cursor, &op->term);
								expr->cursor = &op->term;
								expr->valid = false;
							}
						}
						else if(input == '='){
							if(!first_term && expr->cursor->type == TermType_Literal){
								ast_changed = true;
								Operator* op = make_operator(OpType_ExpressionEquals, expr->cursor);
								insert_right(expr->cursor, &op->term);
								expr->cursor = &op->term;
								expr->equals = &op->term;
							}
						}
						
						//// @input_expression_letters ////
						else if(isalpha(input)){
							//TODO variables
						}
					}
					
					if(ast_changed){
						debug_print_toggle = true;
						solve(&expr->term);
					}
				}
			}break;
			
			////////////////////////////////////////////////////////////////////////////////////////////////
			//// @input_pencil
			case CanvasTool_Pencil: if(!UI::AnyWinHovered()){
				if(DeshInput->KeyPressed(CanvasBind_Pencil_Stroke)){
					PencilStroke new_stroke;
					new_stroke.size  = pencil_stroke_size;
					new_stroke.color = pencil_stroke_color;
					pencil_strokes.add(new_stroke);
					pencil_stroke_start_pos = mouse_pos_world;
				}
				if(DeshInput->KeyDown(CanvasBind_Pencil_Stroke)){
					pencil_strokes[pencil_stroke_idx].pencil_points.add(mouse_pos_world);
				}
				if(DeshInput->KeyReleased(CanvasBind_Pencil_Stroke)){
					pencil_stroke_idx += 1;
				}
				if(DeshInput->KeyPressed(CanvasBind_Pencil_DeletePrevious)){ 
					if(pencil_strokes.count){
						pencil_strokes.pop();
						if(pencil_stroke_idx) pencil_stroke_idx -= 1;
					}
				}
				if     (DeshInput->KeyPressed(CanvasBind_Pencil_SizeIncrementBy1)){ pencil_stroke_size += 1; }
				else if(DeshInput->KeyPressed(CanvasBind_Pencil_SizeIncrementBy5)){ pencil_stroke_size += 5; }
				else if(DeshInput->KeyPressed(CanvasBind_Pencil_SizeDecrementBy1)){ pencil_stroke_size -= 1; }
				else if(DeshInput->KeyPressed(CanvasBind_Pencil_SizeDecrementBy5)){ pencil_stroke_size -= 5; }
				pencil_stroke_size = ((pencil_stroke_size < 1) ? 1 : ((pencil_stroke_size > 100) ? 100 : (pencil_stroke_size)));
				if     (DeshInput->KeyPressed(CanvasBind_Pencil_DetailIncrementBy1)){ pencil_draw_skip_amount -= 1; }
				else if(DeshInput->KeyPressed(CanvasBind_Pencil_DetailIncrementBy5)){ pencil_draw_skip_amount -= 5; }
				else if(DeshInput->KeyPressed(CanvasBind_Pencil_DetailDecrementBy1)){ pencil_draw_skip_amount += 1; }
				else if(DeshInput->KeyPressed(CanvasBind_Pencil_DetailDecrementBy5)){ pencil_draw_skip_amount += 5; }
				pencil_draw_skip_amount = Clamp(pencil_draw_skip_amount, 1, 100);
			}break;
		}
	}
	
	{//// @draw_elements ////
		for(Element2* el : elements){
			UI::PushColor(UIStyleCol_Border, (el == selected_element) ? Color_Yellow : Color_White);
			UI::PushVar(UIStyleVar_FontHeight,       80);
			UI::PushVar(UIStyleVar_WindowMargins,    vec2{5,5});
			UI::PushScale(vec2::ONE * el->height / camera_zoom * 2.0);
			UI::SetNextWindowPos(ToScreen(el->x, el->y));
			UI::Begin(toStr("element_",u64(el)).str, vec2::ZERO, vec2(el->x,el->y) * f32(DeshWindow->width) / (4 * el->y), UIWindowFlags_NoInteract | UIWindowFlags_FitAllElements);
			
			switch(el->type){
				///////////////////////////////////////////////////////////////////////////////////////////////
				//// @draw_elements_expression
				case ElementType_Expression:{
					UI::PushFont(math_font);
					
					Expression2* expr = ElementToExpression(el);
					if(selected_element == el) active_expression = true;
					draw_term(&expr->term, expr->cursor);
					if(debug_print_toggle) Log("ast","---------------------------------");
					active_expression  = false;
					debug_print_toggle = false;
					
					UI::PopFont();
				}break;
				
				///////////////////////////////////////////////////////////////////////////////////////////////
				//// @draw_elements_graph
				//case ElementType_Graph:{}break;
				
				///////////////////////////////////////////////////////////////////////////////////////////////
				//// @draw_elements_workspace
				//case ElementType_Workspace:{}break;
				
				///////////////////////////////////////////////////////////////////////////////////////////////
				//// @draw_elements_text
				//case ElementType_Text:{}break;
			}
			
			UI::End();
			UI::PopScale();
			UI::PopVar(2);
			UI::PopColor();
		}
	}
	
	{//// @draw_pencil ////
		UI::Begin("pencil_layer", vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_Invisible | UIWindowFlags_NoInteract);
		forE(pencil_strokes){
			if(it->pencil_points.count > 1){
				//array<vec2> pps(it->pencil_points.count);
				//forI(it->pencil_points.count) pps.add(ToScreen(it->pencil_points[i]));
				//Render::DrawLines2D(pps, it->size / camera_zoom, it->color, 4, vec2::ZERO, DeshWindow->dimensions);
				
				//TODO smooth line drawing
				for(int i = 1; i < it->pencil_points.count; ++i){
					UI::Line(ToScreen(it->pencil_points[i-1]), ToScreen(it->pencil_points[i]), it->size, it->color);
				}
			}
		}
		UI::End();
	}
	
	{//// @draw_canvas_info ////
		UI::TextF("%.3f FPS", F_AVG(50, 1 / (DeshTime->frameTime / 1000)));
		UI::TextF("Active Tool:   %s", canvas_tool_strings[active_tool]);
		UI::TextF("Previous Tool: %s", canvas_tool_strings[previous_tool]);
		UI::TextF("Selected Element: %d", u64(selected_element));
		UI::TextF("campos:  (%g, %g)",camera_pos.x,camera_pos.y);
		UI::TextF("camzoom: %g", camera_zoom);
		UI::TextF("camarea: (%g, %g)", WorldViewArea().x, WorldViewArea().y);
	}
	
	UI::End();
	UI::PopVar();
}