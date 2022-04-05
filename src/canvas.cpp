/* Index:
@tools
@binds
@pencil
@camera
@context
@utility
@graph
@canvas
@input
@input_tool
@input_navigation
@input_context
@input_expression
@input_expression_deletion
@input_expression_cursor
@input_expression_insertion
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
enum CanvasBind_{ //TODO ideally support multiple keybinds per action
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
	//TODO CanvasBind_Expression_CursorHome
	//TODO CanvasBind_Expression_CursorEnd
	CanvasBind_Expression_CursorDeleteLeft  = Key::BACKSPACE | InputMod_None,
	CanvasBind_Expression_CursorDeleteRight = Key::DELETE    | InputMod_None,
	
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


//////////////////
//// @context ////
//////////////////
local char context_input_buffer[256] = {};
local u32 context_dropdown_selected_index = 0;
local const char* context_dropdown_option_strings[] = {
	"Tool: Navigation", "Tool: Expression", "Tool: Pencil",
	"Add: Graph",
};


////////////////////
//// @draw_term ////
////////////////////
void draw_term(Expression* expr, Term* term, vec2& cursor_start, f32& cursor_y){
	if(term == 0) return;
	
	switch(term->type){
		case TermType_Expression:{
			Expression* expr = ExpressionFromTerm(term);
			if(term->child_count){
				for_node(term->first_child){
				UI::Text(" ", UITextFlags_NoWrap); UI::SameLine();
				draw_term(expr, it, cursor_start, cursor_y);
				}
			}else{
				UI::Text(" ", UITextFlags_NoWrap); UI::SameLine();
			}
			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
				UI::Text(")", UITextFlags_NoWrap); UI::SameLine();
				if(expr->cursor_start == 1){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
			}
			if(expr->cursor_start == expr->raw.count){ cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(); cursor_y = -UI::GetLastItemSize().y; }
			
			//draw solution if its valid
			if(expr->valid){
				UI::PushColor(UIStyleCol_Text, Color_Grey);
				if(expr->equals){
					UI::Text((expr->solution == MAX_F32) ? "ERROR" : to_string(expr->solution, true, deshi_temp_allocator).str, UITextFlags_NoWrap);
					UI::SameLine();
				}else if(expr->solution != MAX_F32){
					UI::Text("=", UITextFlags_NoWrap); UI::SameLine();
					UI::Text(to_string(expr->solution, true, deshi_temp_allocator).str, UITextFlags_NoWrap);
					UI::SameLine();
				}
				UI::PopColor();
			}
			UI::Text(" ", UITextFlags_NoWrap);
		}break;
		
		case TermType_Operator:{
			switch(term->op_type){
				case OpType_Parentheses:{
					UI::Text("(", UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					for_node(term->first_child) draw_term(expr, it, cursor_start, cursor_y);
					if(HasFlag(term->flags, TermFlag_LeftParenHasMatchingRightParen)){
						UI::Text(")", UITextFlags_NoWrap); UI::SameLine();
						if(expr->raw.str + expr->cursor_start == term->raw.str + term->raw.count){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					}
				}break;
				
				case OpType_Negation:{
					UI::Text("-", UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					for_node(term->first_child) draw_term(expr, it, cursor_start, cursor_y);
				}break;
				
				case OpType_ExplicitMultiplication:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term(expr, term->first_child, cursor_start, cursor_y);
					UI::Text("*", UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term(expr, term->last_child, cursor_start, cursor_y);
					if(term->last_child) for_node(term->last_child->next) draw_term(expr, it, cursor_start, cursor_y);
				}break;
				case OpType_Division:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term(expr, term->first_child, cursor_start, cursor_y);
					UI::Text("/", UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term(expr, term->last_child, cursor_start, cursor_y);
					if(term->last_child) for_node(term->last_child->next) draw_term(expr, it, cursor_start, cursor_y);
				}break;
				
				case OpType_Addition:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term(expr, term->first_child, cursor_start, cursor_y);
					UI::Text("+", UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term(expr, term->last_child, cursor_start, cursor_y);
					if(term->last_child) for_node(term->last_child->next) draw_term(expr, it, cursor_start, cursor_y);
				}break;
				case OpType_Subtraction:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term(expr, term->first_child, cursor_start, cursor_y);
					UI::Text("-", UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term(expr, term->last_child, cursor_start, cursor_y);
					if(term->last_child) for_node(term->last_child->next) draw_term(expr, it, cursor_start, cursor_y);
				}break;
				
				case OpType_ExpressionEquals:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term(expr, term->first_child, cursor_start, cursor_y);
					UI::Text("=", UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term(expr, term->last_child, cursor_start, cursor_y);
					if(term->last_child) for_node(term->last_child->next) draw_term(expr, it, cursor_start, cursor_y);
				}break;
			}
			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
				UI::Text(")", UITextFlags_NoWrap); UI::SameLine();
				if(expr->raw.str + expr->cursor_start == term->raw.str + term->raw.count){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
			}
		}break;
		
		case TermType_Literal:{
			UI::Text(term->raw, UITextFlags_NoWrap); UI::SameLine();
			if((term->raw.str <= expr->raw.str + expr->cursor_start) && (expr->raw.str + expr->cursor_start < term->raw.str + term->raw.count)){
				f32 x_offset = UI::CalcTextSize(cstring{term->raw.str, upt(expr->raw.str + expr->cursor_start - term->raw.str)}).x;
				cursor_start = UI::GetLastItemPos() + vec2{x_offset,0}; cursor_y = UI::GetLastItemSize().y;
			}
			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
				UI::Text(")", UITextFlags_NoWrap); UI::SameLine();
				if(expr->raw.str + expr->cursor_start == term->raw.str + term->raw.count){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
			}
		}break;
	}
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @canvas
local array<Element*> elements(deshi_allocator);
local Element* selected_element;
local GraphElement* activeGraph; //TODO remove this and use selected_element instead
local vec2f64 mouse_pos_world;
local Font* math_font;
local GraphElement defgraph; //temp default graph

void init_canvas(){
	f32 world_height  = WorldViewArea().y;
	defgraph.element.height = world_height / 2.f;
	defgraph.element.width  = world_height / 2.f;
	defgraph.element.y      =  defgraph.element.height / 2.f;
	defgraph.element.x      = -defgraph.element.width  / 2.f;
	defgraph.element.type   = ElementType_Graph;
	defgraph.graph = (Graph*)memory_alloc(sizeof(Graph));
	Graph g; memcpy(defgraph.graph, &g, sizeof(Graph));
	defgraph.graph->xAxisLabel.str   = "x";
	defgraph.graph->xAxisLabel.count = 1;
	defgraph.graph->yAxisLabel.str   = "y";
	defgraph.graph->yAxisLabel.count = 1;
	elements.add(&defgraph.element);
	
	//load_constants();

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
	if      (DeshInput->KeyPressed(CanvasBind_SetTool_Navigation) && active_tool != CanvasTool_Navigation){
		previous_tool = active_tool;
		active_tool   = CanvasTool_Navigation;
	}else if(DeshInput->KeyPressed(CanvasBind_SetTool_Context)    && active_tool != CanvasTool_Context){
		previous_tool = active_tool;
		active_tool   = CanvasTool_Context;
	}else if(DeshInput->KeyPressed(CanvasBind_SetTool_Expression) && active_tool != CanvasTool_Expression){
		previous_tool = active_tool;
		active_tool   = CanvasTool_Expression;
	}else if(DeshInput->KeyPressed(CanvasBind_SetTool_Pencil)     && active_tool != CanvasTool_Pencil){
		previous_tool = active_tool;
		active_tool   = CanvasTool_Pencil;
	}else if(DeshInput->KeyPressed(CanvasBind_SetTool_Graph)){
		activeGraph   = (activeGraph) ? 0 : &defgraph;
	}else if(DeshInput->KeyPressed(CanvasBind_SetTool_Previous)){
		Swap(previous_tool, active_tool);
	}
	
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
	if(DeshInput->scrollY != 0 && DeshInput->ModsDown(InputMod_None) && !UI::AnyWinHovered()){
		if(!activeGraph){
			camera_zoom -= camera_zoom / 10.0 * DeshInput->scrollY;
			camera_zoom = Clamp(camera_zoom, 1e-37, 1e37);
		}else{
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
			UI::TextF("Cursor:   %d", (selected_element) ? ((Expression*)selected_element)->cursor_start : 0);
		}
		UI::End();
	}
	if(activeGraph){
		UI::Begin("graph_debug", {200,10}, {200,200}, UIWindowFlags_FitAllElements);
		UI::Text("Graph Info");
		UI::TextF("Element Pos:   (%g,%g)", activeGraph->element.x,activeGraph->element.y);
		UI::TextF("Element Size:  (%g,%g)", activeGraph->element.width,activeGraph->element.height);
		UI::TextF("Camera Pos:    (%g,%g)", activeGraph->graph->cameraPosition.x,activeGraph->graph->cameraPosition.y);
		UI::TextF("Camera View:   (%g,%g)", activeGraph->graph->cameraZoom);
		UI::TextF("Camera Zoom:   %g", activeGraph->graph->cameraView.x,activeGraph->graph->cameraView.y);
		UI::TextF("Dims per Unit: (%g,%g)", activeGraph->graph->dimensions_per_unit_length.x, activeGraph->graph->dimensions_per_unit_length.y);
		UI::TextF("Aspect Ratio:  %g", activeGraph->graph->aspect_ratio);
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
					camera_pan_start_pos = vec2f64{activeGraph->graph->cameraPosition.x, activeGraph->graph->cameraPosition.y};
				}
			}
			if(DeshInput->KeyDown(CanvasBind_Navigation_Pan)){
				if(!activeGraph){
					camera_pos = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - mouse_pos_world);
				}else{
					activeGraph->graph->cameraPosition.x = (camera_pan_start_pos.x + (camera_pan_mouse_pos.x - DeshInput->mouseX) / activeGraph->graph->dimensions_per_unit_length.x);
					activeGraph->graph->cameraPosition.y = (camera_pan_start_pos.y + (camera_pan_mouse_pos.y - DeshInput->mouseY) / activeGraph->graph->dimensions_per_unit_length.y);
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
				for(Element* it : elements){
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
				Expression* expr = (Expression*)memory_alloc(sizeof(Expression)); //TODO expression arena
				expr->element.x       = mouse_pos_world.x;
				expr->element.y       = mouse_pos_world.y;
				expr->element.height  = (320*camera_zoom) / (f32)DeshWindow->width;
				expr->element.width   = expr->element.height / 2.0;
				expr->element.type    = ElementType_Expression;
				expr->term.type       = TermType_Expression;
				expr->terms.allocator = deshi_allocator;
				expr->raw             = string(" ", deshi_allocator);
				expr->cursor_start    = 1;
				
				elements.add(&expr->element);
				selected_element = &expr->element;
			}
			
			if(selected_element && selected_element->type == ElementType_Expression){
				Expression* expr = ElementToExpression(selected_element);
				b32 ast_changed = false;
				
				//// @input_expression_cursor ////
				if(expr->cursor_start > 1 && DeshInput->KeyPressed(CanvasBind_Expression_CursorLeft)){
						expr->cursor_start -= 1;
				}
				if(expr->cursor_start < expr->raw.count && DeshInput->KeyPressed(CanvasBind_Expression_CursorRight)){
					expr->cursor_start += 1;
				}
				
				//character based input (letters, numbers, symbols)
				//// @input_expression_insertion ////
				forI(DeshInput->charCount){
					ast_changed = true;
					expr->raw.insert(DeshInput->charIn[i], expr->cursor_start);
					expr->cursor_start += 1;
				}
				
				//// @input_expression_deletion ////
				if(expr->cursor_start > 1 && DeshInput->KeyPressed(CanvasBind_Expression_CursorDeleteLeft)){
					ast_changed = true;
					expr->raw.erase(expr->cursor_start-1);
					expr->cursor_start -= 1;
				}
				if(expr->cursor_start < expr->raw.count-1 && DeshInput->KeyPressed(CanvasBind_Expression_CursorDeleteRight)){
					ast_changed = true;
					expr->raw.erase(expr->cursor_start);
				}
				
				if(ast_changed){
					expr->valid = parse(expr);
					solve(&expr->term);
					debug_print_term(&expr->term);
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
	for(Element* el : elements){
		switch(el->type){
			///////////////////////////////////////////////////////////////////////////////////////////////
			//// @draw_elements_expression
			case ElementType_Expression:{
				UI::PushColor(UIStyleCol_Border, (el == selected_element) ? Color_Yellow : Color_White);
				UI::PushVar(UIStyleVar_FontHeight,       80);
				UI::PushVar(UIStyleVar_WindowMargins,    vec2{5,5});
				UI::PushScale(vec2::ONE * el->height / camera_zoom * 2.0);
				UI::SetNextWindowPos(ToScreen(el->x, el->y));
				UI::PushFont(math_font);
				UI::Begin(stringf(deshi_temp_allocator, "expression_0x%p",el).str, vec2::ZERO, vec2(el->x,el->y) * f32(DeshWindow->width) / (4 * el->y), UIWindowFlags_NoInteract | UIWindowFlags_FitAllElements);
				
				Expression* expr = ElementToExpression(el);
				vec2 cursor_start; f32 cursor_y;
				draw_term(expr, &expr->term, cursor_start, cursor_y);
				if(selected_element == el){
					UI::Line(cursor_start, cursor_start + vec2{0,cursor_y}, 2, Color_White * abs(sin(DeshTime->totalTime)));
				}
				
				UI::End();
				UI::PopFont();
				UI::PopScale();
				UI::PopVar(2);
				UI::PopColor();
			}break;
			
			///////////////////////////////////////////////////////////////////////////////////////////////
			//// @draw_elements_graph
			case ElementType_Graph:{
				GraphElement* ge = ElementToGraphElement(el);
				vec2 tl = ToScreen(ge->element.x, ge->element.y);
				vec2 br = ToScreen(ge->element.x + ge->element.width, ge->element.y - ge->element.height);
				draw_graph(ge->graph, vec2g{tl.x, tl.y}, vec2g{br.x - tl.x, br.y - tl.y});
			}break;
			
			///////////////////////////////////////////////////////////////////////////////////////////////
			//// @draw_elements_workspace
			//case ElementType_Workspace:{}break;
			
			///////////////////////////////////////////////////////////////////////////////////////////////
			//// @draw_elements_text
			//case ElementType_Text:{}break;
		}
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
	UI::TextF("graph active: %s", (activeGraph) ? "true" : "false");
	
	UI::End();
	UI::PopVar();
}