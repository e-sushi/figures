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
	change_parent_insert_last(&op->term, cursor);
	
	//remove cursor's old OpArg flags
	RemoveFlag(cursor->flags, OPARG_MASK);
	AddFlag(cursor->flags, TermFlag_OpArgLeft);
	
	return op;
}

//return false if the AST of the expression is invalid, true otherwise
b32 expression_is_valid(Expression2* expr){
	Term* term = &expr->term;
	while(term){
		switch(term->type){
			case TermType_Expression:{
				if(term->right == 0  || term->right->type == TermType_Operator || term->child_count > 1){
					//TODO unary operators
					//TODO nested expressions
					return false;
				}
				if(term->child_count == 1 && term->first_child->type == TermType_Literal) return false;
			}break;
			case TermType_Operator:{
				Operator* op = OperatorFromTerm(term);
				switch(op->type){
					//// two arg operators ////
					case OpType_ExplicitMultiplication:
					case OpType_Division:
					case OpType_Addition:
					case OpType_Subtraction:
					{
						if(term->child_count != 2) return false;
						if(term->left == 0  || term->left->type  != TermType_Literal) return false;
						if(term->right == 0 || term->right->type != TermType_Literal) return false;
					}break;
					
					//// special operators ////
					case OpType_ExpressionEquals:{
						//TODO right side of equals
						//NOTE case TermType_Expression above handles when = is first term
						if(term->left == 0) return false;
					}break;
				}
			}break;
		}
		term = term->right;
	}
	return true;
}

#define PRINT_AST true
#if PRINT_AST
local s32 debug_print_indent = -1;
void debug_print_term(Term* term, Term* cursor){
	debug_print_indent++;
	string indent(deshi_temp_allocator); forI(debug_print_indent) indent += "  ";
	char* arg = (HasFlag(term->flags, TermFlag_OpArgLeft)) ? " L"
		: (HasFlag(term->flags, TermFlag_OpArgRight) ) ? " R"
		: (HasFlag(term->flags, TermFlag_OpArgTop)   ) ? " T"
		: (HasFlag(term->flags, TermFlag_OpArgBottom)) ? " B"
		: "  ";
	char* cursor_str = (term == cursor) ? " <- ": "    ";
	
	switch(term->type){
		case TermType_Expression:{
			Expression2* expr = ExpressionFromTerm(term);
			if(term->child_count){
				for_node(term->first_child) debug_print_term(it, cursor);
				if(expr->valid){
					if(expr->equals){
						Log("ast", indent, to_string(expr->solution, true, deshi_temp_allocator).str, arg, cursor_str, term->left,",",term,",",term->right);
					}else if(expr->solution != MAX_F32){
						Log("ast", indent, (string("=", deshi_temp_allocator) + to_string(expr->solution, true, deshi_temp_allocator)).str, arg, cursor_str, term->left,",",term,",",term->right);
					}
				}
			}
		}break;
		
		case TermType_Operator:{
			switch(OperatorFromTerm(term)->type){
				case OpType_Addition:              { Log("ast", indent, "+", arg, cursor_str, term->left,",",term,",",term->right); }break;
				case OpType_Subtraction:           { Log("ast", indent, "-", arg, cursor_str, term->left,",",term,",",term->right); }break;
				case OpType_ExplicitMultiplication:{ Log("ast", indent, "*", arg, cursor_str, term->left,",",term,",",term->right); }break;
				case OpType_Division:              { Log("ast", indent, "/", arg, cursor_str, term->left,",",term,",",term->right); }break;
				case OpType_ExpressionEquals:      { Log("ast", indent, "=", arg, cursor_str, term->left,",",term,",",term->right); }break;
			}
			for_node(term->first_child) debug_print_term(it, cursor);
		}break;
		
		case TermType_Literal:{
			Log("ast", indent, to_string(LiteralFromTerm(term)->value, true, deshi_temp_allocator).str, arg, cursor_str, term->left,",",term,",",term->right);
		}break;
		
		//case TermType_Variable:{}break;
		//case TermType_FunctionCall:{}break;
	}
	debug_print_indent--;
}
#else
#  define debug_print_term(term,cursor) (void)0
#endif

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @canvas
local array<Element2*> elements(deshi_allocator);
local Element2* selected_element;
local GraphElement* activeGraph;
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
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @input
	mouse_pos_world = ToWorld(DeshInput->mousePos);
	
	//// @input_tool ////
	if     (DeshInput->KeyPressed(CanvasBind_SetTool_Navigation)){ previous_tool = active_tool; active_tool = CanvasTool_Navigation; }
	else if(DeshInput->KeyPressed(CanvasBind_SetTool_Context))   { previous_tool = active_tool; active_tool = CanvasTool_Context; }
	else if(DeshInput->KeyPressed(CanvasBind_SetTool_Expression)){ previous_tool = active_tool; active_tool = CanvasTool_Expression; }
	else if(DeshInput->KeyPressed(CanvasBind_SetTool_Pencil))    { previous_tool = active_tool; active_tool = CanvasTool_Pencil; }
	//else if(DeshInput->KeyPressed(CanvasBind_SetTool_Graph))     { activeGraph = (activeGraph) ? 0 : graphs.data; }
	else if(DeshInput->KeyPressed(CanvasBind_SetTool_Previous))  { Swap(previous_tool, active_tool); }
	
	//// @input_camera ////
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

	if(DeshInput->KeyDown(CanvasBind_Camera_ZoomOut | InputMod_None) && !UI::AnyWinHovered()){
		if(selected_element && selected_element->type != ElementType_Graph){
			camera_zoom -= camera_zoom / 10.0 * DeshInput->scrollY;
			camera_zoom = Clamp(camera_zoom, 1e-37, 1e37);
		}
		else{
			activeGraph->graph->cameraZoom -= 0.2*activeGraph->graph->cameraZoom*DeshInput->scrollY;
		}
	}
	if(DeshInput->KeyDown(CanvasBind_Camera_ZoomIn | InputMod_None) && !UI::AnyWinHovered()){
		if(selected_element && selected_element->type != ElementType_Graph){
			camera_zoom -= camera_zoom / 10.0 * DeshInput->scrollY;
			camera_zoom = Clamp(camera_zoom, 1e-37, 1e37);
		}
		else{
			activeGraph->graph->cameraZoom -= 0.2*activeGraph->graph->cameraZoom*DeshInput->scrollY;
		}
	}
	
#if 1 //NOTE temp ui
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
		//// @input_navigation ////
		case CanvasTool_Navigation: if(!UI::AnyWinHovered()){
			if(DeshInput->KeyPressed(CanvasBind_Navigation_Pan)){
				camera_pan_active = true;
				camera_pan_mouse_pos = DeshInput->mousePos;
				if(!activeGraph){
					camera_pan_start_pos = camera_pos;
				}else{
					camera_pan_start_pos = vec2f64{activeGraph->graph->cameraPosition.x,activeGraph->graph->cameraPosition.y};
				}
			}
			if(DeshInput->KeyDown(CanvasBind_Navigation_Pan)){
				if(!activeGraph){
					camera_pos = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - mouse_pos_world);
				}else{
					Graph*  g = activeGraph->graph;
					g->cameraPosition = vec2g{camera_pan_start_pos.x, camera_pan_start_pos.y} - (vec2g{mouse_pos_world.x, mouse_pos_world.y} - vec2g{camera_pan_mouse_pos.x, camera_pan_mouse_pos.y}) / vec2g{g->dimensions_per_unit_length.x, g->dimensions_per_unit_length.y*g->aspect_ratio};
				}
			}
			if(DeshInput->KeyReleased(CanvasBind_Navigation_Pan)){
				camera_pan_active = false;
			}
			if(DeshInput->KeyPressed(CanvasBind_Navigation_ResetPos)){
				if(!activeGraph){
					camera_pos = {0,0};
				}else{
					activeGraph->graph->cameraPosition = {0,0};
				}
			}
			if(DeshInput->KeyPressed(CanvasBind_Navigation_ResetZoom)){
				if(!activeGraph){
					camera_zoom = 1.0;
				}else{
					activeGraph->graph->cameraZoom = 5.0;
				}
			}
		}break;
		
		//// @input_context ////
		case CanvasTool_Context:{
			//if(UI::BeginContextMenu("canvas_context_menu")){
			//UI::EndContextMenu();
			//}
		}break;
		
		//// @input_expression ////
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
				expr->term_count = 1;
				
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
						case TermType_Operator:{ //TODO non-binary/non-linear operators
							ast_changed = true;
							Operator* op = OperatorFromTerm(expr->cursor);
							
							if(expr->cursor->right){
								b32 double_operator = false;
								
								//change parents of operator children
								if(expr->cursor->child_count){
									//TODO handle 3+ operators in a row
									//if cursor operator is next to another operator, change child's parent to greater precedence operator
									if(expr->cursor->left->type == TermType_Operator){
										double_operator = true;
										if(expr->cursor->right->right && expr->cursor->right->right->type == TermType_Operator){
											if(*OperatorFromTerm(expr->cursor->right->right) > OperatorFromTerm(expr->cursor->left)){
												change_parent_insert_last(expr->cursor->right->right, expr->cursor->right);
												RemoveFlag(expr->cursor->right->flags, OPARG_MASK);
												AddFlag(expr->cursor->right->flags, TermFlag_OpArgLeft);
												
												//left op's parentmost changes parent to right op
												Term* it = expr->cursor->left;
												while(it->parent && it->parent != &expr->term && it->parent != expr->cursor && it->parent != expr->cursor->right->right) it = it->parent;
												change_parent_insert_last(expr->cursor->right->right, it);
												RemoveFlag(it->flags, OPARG_MASK);
												AddFlag(it->flags, TermFlag_OpArgLeft);
											}else{
												change_parent_insert_first(expr->cursor->left, expr->cursor->right);
												RemoveFlag(expr->cursor->right->flags, OPARG_MASK);
												AddFlag(expr->cursor->right->flags, TermFlag_OpArgRight);
											}
										}else{
											change_parent_insert_first(expr->cursor->left, expr->cursor->right);
											RemoveFlag(expr->cursor->right->flags, OPARG_MASK);
											AddFlag(expr->cursor->right->flags, TermFlag_OpArgRight);
										}
									}
									if(expr->cursor->right->type == TermType_Operator){
										double_operator = true;
										if(expr->cursor->left->left && expr->cursor->left->left->type == TermType_Operator){
											if(*OperatorFromTerm(expr->cursor->left->left) >= OperatorFromTerm(expr->cursor->right)){
												change_parent_insert_last(expr->cursor->left->left, expr->cursor->left);
												RemoveFlag(expr->cursor->left->flags, OPARG_MASK);
												AddFlag(expr->cursor->left->flags, TermFlag_OpArgRight);
											}else{
												change_parent_insert_first(expr->cursor->right, expr->cursor->left);
												RemoveFlag(expr->cursor->left->flags, OPARG_MASK);
												AddFlag(expr->cursor->left->flags, TermFlag_OpArgLeft);
												
												//right op's parentmost changes parent to left op
												Term* it = expr->cursor->right;
												while(it->parent && it->parent != &expr->term && it->parent != expr->cursor && it->parent != expr->cursor->left->left) it = it->parent;
												change_parent_insert_last(expr->cursor->left->left, it);
												RemoveFlag(it->flags, OPARG_MASK);
												AddFlag(it->flags, TermFlag_OpArgRight);
											}
										}else{
											change_parent_insert_first(expr->cursor->right, expr->cursor->left);
											RemoveFlag(expr->cursor->left->flags, OPARG_MASK);
											AddFlag(expr->cursor->left->flags, TermFlag_OpArgLeft);
										}
									}
									
									if(!double_operator){
										if(   expr->cursor->left == expr->cursor->first_child
										   && expr->cursor->left->left
										   && expr->cursor->left->left->type == TermType_Operator
										   && *op >= OperatorFromTerm(expr->cursor->left->left)){
											change_parent_insert_first(expr->cursor->left->left, expr->cursor->left);
											RemoveFlag(expr->cursor->left->flags, OPARG_MASK);
											AddFlag(expr->cursor->left->flags, TermFlag_OpArgRight);
										}
										if(   expr->cursor->right == expr->cursor->last_child
										   && expr->cursor->right->right 
										   && expr->cursor->right->right->type == TermType_Operator 
										   && *op >= OperatorFromTerm(expr->cursor->right->right)){
											change_parent_insert_first(expr->cursor->right->right, expr->cursor->right);
											RemoveFlag(expr->cursor->right->flags, OPARG_MASK);
											AddFlag(expr->cursor->right->flags, TermFlag_OpArgLeft);
										}
									}
								}
								
								//separate the sides of the expression
								if(!double_operator){
									if(expr->cursor->left){
										Term* it = expr->cursor->left;
										while(it->parent && it->parent->linear < expr->cursor->linear) it = it->parent;
										if(it != &expr->term){
											change_parent_insert_last(&expr->term, it);
											RemoveFlag(it->flags, OPARG_MASK);
										}
									}
									Term* it = expr->cursor->right;
									while(it->parent && it->parent->linear > expr->cursor->linear) it = it->parent;
									if(it != &expr->term){
										change_parent_insert_last(&expr->term, it);
										RemoveFlag(it->flags, OPARG_MASK);
									}
								}
							}
							
							//remove this operator from AST
							remove(expr->cursor);
							expr->cursor = expr->cursor->left;
							remove_leftright(expr->cursor->right);
							
							//update expression
							if(&op->term == expr->equals) expr->equals = 0;
							for(Term* it = expr->cursor->right; it != 0; it = it->right) it->linear--;
							expr->term_count--;
							
							memory_zfree(op);
						}break;
						case TermType_Literal:{
							ast_changed = true;
							Literal* lit = LiteralFromTerm(expr->cursor);
							
							//if right or left edge, no need to reorganize things
							if(expr->cursor->right && expr->cursor->left->type != TermType_Expression){
								if(expr->cursor->left->type == TermType_Operator){
									if(expr->cursor->right->type == TermType_Literal){
										if(expr->cursor->right->parent->type == TermType_Operator){
											if(*OperatorFromTerm(expr->cursor->left) >= OperatorFromTerm(expr->cursor->right->parent)){
												//if left op is greater/equal to right op, left op steals right literal and becomes left child of right op
												Term* right_op = expr->cursor->right->parent;
												change_parent_insert_last(expr->cursor->parent, expr->cursor->right);
												RemoveFlag(expr->cursor->right->flags, OPARG_MASK);
												AddFlag(expr->cursor->right->flags, TermFlag_OpArgRight);
												
												change_parent_insert_first(right_op, expr->cursor->left);
												RemoveFlag(expr->cursor->left->flags, OPARG_MASK);
												AddFlag(expr->cursor->left->flags, TermFlag_OpArgLeft);
											}else{
												//if left op is less than right op, right op's parentmost becomes right child of left op
												Term* it = expr->cursor->right;
												while(it->parent != &expr->term) it = it->parent;
												change_parent_insert_last(expr->cursor->parent, it);
												RemoveFlag(it->flags, OPARG_MASK);
												AddFlag(it->flags, TermFlag_OpArgRight);
											}
										}else{
											//if right's parent is not an operator, its dangling, so just change replace cursor with it
											change_parent_insert_last(expr->cursor->parent, expr->cursor->right);
											RemoveFlag(expr->cursor->right->flags, OPARG_MASK);
											AddFlag(expr->cursor->right->flags, TermFlag_OpArgRight);
										}
									}
								}else{
									//left term is not an operator/expression, so its a literal/var/func
									if(expr->cursor->right->type == TermType_Operator){
										if(expr->cursor->left->parent->type == TermType_Operator){
											if(*OperatorFromTerm(expr->cursor->left->parent) >= OperatorFromTerm(expr->cursor->right)){
												//if left op is greater/equal to right op, make it a child of right op
												change_parent_insert_first(expr->cursor->right, expr->cursor->left->parent);
												RemoveFlag(expr->cursor->left->parent->flags, OPARG_MASK);
												AddFlag(expr->cursor->left->parent->flags, TermFlag_OpArgLeft);
											}else{
												//if left op is less than right op, right op steals left literal and becomes right child of left op
												Term* left_op = expr->cursor->left->parent;
												change_parent_insert_first(expr->cursor->parent, expr->cursor->left);
												RemoveFlag(expr->cursor->left->flags, OPARG_MASK);
												AddFlag(expr->cursor->left->flags, TermFlag_OpArgLeft);
												
												change_parent_insert_last(left_op, expr->cursor->right);
												RemoveFlag(expr->cursor->right->flags, OPARG_MASK);
												AddFlag(expr->cursor->right->flags, TermFlag_OpArgRight);
											}
										}else{
											//if left's parent is not an operator, its dangling, so just change replace cursor with it
											change_parent_insert_first(expr->cursor->parent, expr->cursor->left);
											RemoveFlag(expr->cursor->left->flags, OPARG_MASK);
											AddFlag(expr->cursor->left->flags, TermFlag_OpArgLeft);
										}
									}
								}
							}
							
							//remove this literal from AST
							remove(expr->cursor);
							expr->cursor = expr->cursor->left;
							remove_leftright(expr->cursor->right);
							
							//update expression
							for(Term* it = expr->cursor->right; it != 0; it = it->right) it->linear--;
							expr->term_count--;
							memory_zfree(lit);
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
				b32 first_term = (expr->cursor == &expr->term);
				forI(DeshInput->charCount){
					char input = DeshInput->charIn[i];
					
					//// @input_expression_literals ////
					//TODO remove duplication
					//TODO in-the-middle insertion
					if(isdigit(input)){
						if(first_term){
							ast_changed = true;
							Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
							lit->term.type   = TermType_Literal;
							lit->term.linear = expr->term_count++;
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
						}else if(expr->cursor->type == TermType_Operator){ //right side of operator //TODO non-binary/non-linear operators
							ast_changed = true;
							Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
							lit->term.type   = TermType_Literal;
							lit->term.flags  = TermFlag_OpArgRight;
							lit->term.linear = expr->term_count++;
							insert_last(expr->cursor, &lit->term);
							insert_right(expr->cursor, &lit->term);
							expr->cursor = &lit->term;
							
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
							lit->term.type   = TermType_Literal;
							lit->term.linear = expr->term_count++;
							insert_first(&expr->term, &lit->term);
							insert_right(&expr->term, &lit->term);
							expr->cursor = &lit->term;
						}
						
						if(expr->cursor->type == TermType_Literal){
							Literal* lit = LiteralFromTerm(expr->cursor);
							if(lit->decimal == 0) lit->decimal = 1;
						}else if(expr->cursor->type == TermType_Operator){ //right side of operator //TODO non-binary/non-linear operators
							ast_changed = true;
							Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
							lit->term.type   = TermType_Literal;
							lit->term.flags  = TermFlag_OpArgRight;
							lit->term.linear = expr->term_count++;
							insert_last(expr->cursor, &lit->term);
							insert_right(expr->cursor, &lit->term);
							expr->cursor = &lit->term;
							
							if(lit->decimal == 0) lit->decimal = 1;
						}
					}
					else if(input == 'e' || input == 'E'){
						//TODO exponential literal input
					}
					
					//// @input_expression_operators ////
					//TODO in-the-middle insertion
					else if(input == '+'){
						if(!first_term && expr->cursor->type == TermType_Literal){
							ast_changed = true;
							Operator* op = make_operator(OpType_Addition, expr->cursor);
							op->term.linear = expr->term_count++;
							insert_right(expr->cursor, &op->term);
							expr->cursor = &op->term;
						}
					}
					else if(input == '-'){
						if(!first_term && expr->cursor->type == TermType_Literal){
							ast_changed = true;
							Operator* op = make_operator(OpType_Subtraction, expr->cursor);
							op->term.linear = expr->term_count++;
							insert_right(expr->cursor, &op->term);
							expr->cursor = &op->term;
						}
					}
					else if(input == '*'){
						if(!first_term && expr->cursor->type == TermType_Literal){
							ast_changed = true;
							Operator* op = make_operator(OpType_ExplicitMultiplication, expr->cursor);
							op->term.linear = expr->term_count++;
							insert_right(expr->cursor, &op->term);
							expr->cursor = &op->term;
						}
					}
					else if(input == '/'){
						if(!first_term && expr->cursor->type == TermType_Literal){
							ast_changed = true;
							Operator* op = make_operator(OpType_Division, expr->cursor);
							op->term.linear = expr->term_count++;
							insert_right(expr->cursor, &op->term);
							expr->cursor = &op->term;
						}
					}
					else if(input == '='){
						if(!first_term && expr->cursor->type == TermType_Literal){
							ast_changed = true;
							Operator* op = make_operator(OpType_ExpressionEquals, expr->cursor);
							op->term.linear = expr->term_count++;
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
					expr->valid  = expression_is_valid(expr);
					solve(&expr->term);
#if PRINT_AST
					debug_print_term(&expr->term, expr->cursor);
					Log("ast","---------------------------------");
#endif
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
	
	//// @draw_elements ////
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
			
				//draw terms from left to right
				Expression2* expr = ElementToExpression(el);
				Term* term = &expr->term;
				vec2 cursor_start;
				f32 cursor_y;
				while(term){
					switch(term->type){
						case TermType_Expression:{
							UI::Text(" ", UITextFlags_NoWrap); UI::SameLine();
						}break;
						
						case TermType_Operator:{
							Operator* op = OperatorFromTerm(term);
							switch(op->type){
								case OpType_ExplicitMultiplication:{
									UI::Text("*", UITextFlags_NoWrap); UI::SameLine();
								}break;
								case OpType_Division:{
									UI::Text("/", UITextFlags_NoWrap); UI::SameLine();
								}break;
								case OpType_Addition:{
									UI::Text("+", UITextFlags_NoWrap); UI::SameLine();
								}break;
								case OpType_Subtraction:{
									UI::Text("-", UITextFlags_NoWrap); UI::SameLine();
								}break;
								case OpType_ExpressionEquals:{
									UI::Text("=", UITextFlags_NoWrap); UI::SameLine();
								}break;
							}
						}break;
						
						case TermType_Literal:{
							if(term->left && term->left->type == TermType_Literal){
								UI::Text(" ", UITextFlags_NoWrap); UI::SameLine();
							}
							
							Literal* lit = LiteralFromTerm(term);
							UI::Text(to_string(lit->value, true, deshi_temp_allocator).str, UITextFlags_NoWrap); UI::SameLine();
							if(lit->decimal == 1){
								UI::Text(".", UITextFlags_NoWrap); UI::SameLine(); //TODO decimal config here
							}
						}break;
					}
					
					if(selected_element == el && term == expr->cursor){
						cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(); cursor_y = UI::GetLastItemSize().y;
						UI::Line(cursor_start, cursor_start - vec2{0,cursor_y}, 2, Color_White * abs(sin(DeshTime->totalTime)));
					}
					term = term->right;
				}
				
				//draw solution if its valid
				if(expr->valid && expr->term.child_count){
					UI::PushColor(UIStyleCol_Text, Color_Grey);
					if(expr->equals){
						UI::Text((expr->solution == MAX_F32) ? "ERROR" : to_string(expr->solution, true, deshi_temp_allocator).str, UITextFlags_NoWrap);
						UI::SameLine();
					}else if(expr->solution != MAX_F32 && expr->term.first_child->type != TermType_Literal){
						UI::Text("=", UITextFlags_NoWrap); UI::SameLine();
						UI::Text(to_string(expr->solution, true, deshi_temp_allocator).str, UITextFlags_NoWrap);
						UI::SameLine();
					}
					UI::PopColor();
				}
				UI::Text(" ", UITextFlags_NoWrap);
				
				UI::PopFont();
			}break;
			
			///////////////////////////////////////////////////////////////////////////////////////////////
			//// @draw_elements_graph
			case ElementType_Graph:{
				GraphElement* ge = ElementToGraphElement(el);
				Graph* e = ge->graph;
				draw_graph(e, vec2g{ge->element.x, ge->element.y});
			}break;
			
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
	
	//// @draw_pencil ////
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
	
	//// @draw_canvas_info ////
	UI::TextF("%.3f FPS", F_AVG(50, 1 / (DeshTime->frameTime / 1000)));
	UI::TextF("Active Tool:   %s", canvas_tool_strings[active_tool]);
	UI::TextF("Previous Tool: %s", canvas_tool_strings[previous_tool]);
	UI::TextF("Selected Element: %d", u64(selected_element));
	UI::TextF("campos:  (%g, %g)",camera_pos.x,camera_pos.y);
	UI::TextF("camzoom: %g", camera_zoom);
	UI::TextF("camarea: (%g, %g)", WorldViewArea().x, WorldViewArea().y);
	
	UI::End();
	UI::PopVar();
}