////////////////
//// @tools ////
////////////////
enum CanvasTool_{
	CanvasTool_Navigation,
	CanvasTool_Context,
	CanvasTool_Expression,
	CanvasTool_Pencil,
}; typedef u32 CanvasTool;
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
	CanvasBind_Expression_Select = Key::MBLEFT  | InputMod_None, //pressed
	CanvasBind_Expression_Create = Key::MBRIGHT | InputMod_None, //pressed
	
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


////////////////
//// @fonts ////
////////////////
local Font* mathfont;
local Font* mathfontitalic;


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


//////////////////
//// @utility ////
//////////////////
local vec2f64 mouse_pos_world;

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
//// @element ////
//////////////////
void Element::
AddToken(TokenType t) {
	//special initial case
	if (tokens.count == 0) {
		//check if we are inserting a literal
		if (t == tok_Literal) {
			tokens.add(token(tok_Literal));
			cursor = 0; //position cursor in literal's box
		}
		//if we are dealing with a binop make a binop case
		else if (t >= tok_Plus && t <= tok_Modulo) {
			tokens.add(token(tok_Literal));
			tokens.add(token(t));
			tokens.add(token(tok_Literal));
			cursor = 0; //position cursor inside first tok_Literal box 
			
		}
		//unary op ditto
		else if (t == tok_LogicalNOT || t == tok_BitwiseComplement || t == tok_Negation) {
			tokens.add(token(t));
			tokens.add(token(tok_Literal));
			cursor = 1; //position cursor in the literals box
		}
		
	}
	else {
		//we must deal with the cursor and only if its in a position to add a new token
		//other wise input is handled in Update()
		//TODO(sushi) handle inputting operators when we are within
		if (cursor == -1) {
			//insert new token at beginning 
			tokens.insert(token(t), 0);
			tokens.insert(token(tok_Literal), 0);
			cursor = 0;
		}
		else if (cursor == tokens.count) {
			//keeping these 2 cases separate for now
			//if its a long time after 08/15/2021 and i havent merged them u can do that
			if (t >= tok_Plus && t <= tok_Modulo) {
				tokens.add(token(t));
				tokens.add(token(tok_Literal));
				cursor = tokens.count - 2; //position cursor inside first tok_Literal box 
				
			}
			else if (t == tok_LogicalNOT || t == tok_BitwiseComplement || t==tok_Negation) {
				tokens.add(token(t));
				tokens.add(token(tok_Literal));
				cursor = tokens.count - 2; //position cursor inside first tok_Literal box 
			}
		}
	}
	
	// CalcSize();
	statement = Parser::parse(tokens);
}

void Element::
Update() {
	using namespace UI;
	
	Parser::pretty_print(statement);
	
	PushFont(mathfont);
	
	Font* font = mathfont;
	vec2 winpos = ToScreen(pos.x, pos.y);
	
	PushVar(UIStyleVar_WindowPadding,      vec2{ 0,0 });
	PushVar(UIStyleVar_InputTextTextAlign, vec2{ 0, 0 });
	PushVar(UIStyleVar_RowItemAlign,       vec2{ 0.5, 0.5 });
	PushVar(UIStyleVar_FontHeight,         80);
	
	PushScale(vec2::ONE * size.y / camera_zoom * 2);
	
	SetNextWindowPos(winpos);
	Begin(toStr("canvas_element_",u64(this)).str, vec2{ 0,0 }, size * f32(DeshWindow->width) / (4 * size.y), UIWindowFlags_NoMove | UIWindowFlags_NoResize | UIWindowFlags_DontSetGlobalHoverFlag);
	
	if (tokens.count) {
		
		UI::BeginRow(tokens.count, 30);
		for (int i = 0; i < tokens.count; i++) {
			token curt = tokens[i];
			
			if (curt.type != tok_Literal)
				RowSetupRelativeColumnWidth(i + 1, 2);
			else
				RowSetupRelativeColumnWidth(i + 1, 1);
			
			//cases where the user has the token selected
			if (i == cursor) {
				if (curt.type == tok_Literal) {
					SetNextItemActive();
					
					if (!curt.str[0])
						SetNextItemSize(vec2{ (f32)font->max_height, (f32)font->max_height });
					
					if (InputText((char*)toStr((char*)this + tokens.count).str, tokens[cursor].str, 255, "", UIInputTextFlags_NoBackground | UIInputTextFlags_AnyChangeReturnsTrue | UIInputTextFlags_FitSizeToText | UIInputTextFlags_Numerical)) {
						tokens[i].strSize = CalcTextSize(tokens[i].str);
						statement = Parser::parse(tokens);
					}
					
					//selection outline
					Rect(GetLastItemScreenPos() - vec2::ONE, GetLastItemSize() + vec2::ONE, color{ 64, 64, 64, (u8)(175.f * (sinf(3 * DeshTotalTime) + 1) / 2) });
					
				}
				//underline anything else for now
				else {
					
					Text(tokens[i].str, UITextFlags_NoWrap);
					Line(vec2{ GetLastItemScreenPos().x + font->max_width, GetLastItemScreenPos().y + (f32)font->max_height + 1 }, vec2{ GetLastItemScreenPos().x, GetLastItemScreenPos().y + (f32)font->max_height + 1 }, 1);
				}
			}
			else {
				if (!curt.str[0])
					SetNextItemSize(vec2{ (f32)font->max_height, (f32)font->max_height });
				UI::Text(tokens[i].str, UITextFlags_NoWrap);
				
				
			}
		}
		UI::EndRow();
	}
	else {
		//draw initial statement
		PushFont(mathfontitalic);
		UI::Text("type initial statement...", UITextFlags_NoWrap);
		PopFont();
	}
	
	End();
	UI::PopVar(4);
	UI::PopFont();
	UI::PopScale();
}


/////////////////
//// @pencil ////
/////////////////
local void 
DrawPencilStrokes(){
    UI::Begin("pencil_canvas", vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_Invisible | UIWindowFlags_NoInteract);
    forE(pencil_strokes){
        if(it->pencil_points.count > 1){
            array<vec2> pps(it->pencil_points.count);
            forI(it->pencil_points.count) pps.add(ToScreen(it->pencil_points[i]));
            Render::DrawLines2D(pps, it->size / camera_zoom, it->color, 4, vec2::ZERO, DeshWindow->dimensions);
        }
    }
    UI::End();
}


////////////////
//// @graph ////
////////////////
local vec2f64
WorldToGraph(vec2f64 point, Graph* graph){
	point += graph->cameraPosition;
	point *= graph->cameraZoom;
	return point;
}FORCE_INLINE vec2f64 WorldToGraph(f64 x, f64 y, Graph* graph){ return WorldToGraph({x,y},graph); }

local vec2f64
GraphToWorld(vec2f64 point, Graph* graph){
	point /= graph->cameraZoom;
	point -= graph->cameraPosition;
	return point;
}FORCE_INLINE vec2f64 GraphToWorld(f64 x, f64 y, Graph* graph){ return GraphToWorld({x,y},graph); }

inline vec2
GraphToScreen(vec2f64 point, Graph* graph){
	return ToScreen(GraphToWorld(point, graph));
}FORCE_INLINE vec2 GraphToScreen(f64 x, f64 y, Graph* graph){ return GraphToScreen({x,y},graph); }

local void 
DrawGraphGrid(Graph* graph){
	//draw border
	vec2f64 tl_world = graph->position - (graph->dimensions/2.f); // -x, +y
	vec2f64 br_world = graph->position + (graph->dimensions/2.f); // +x, -y
	UI::Line(ToScreen(tl_world.x,tl_world.y), ToScreen(br_world.x,tl_world.y), 1, PackColorU32(255,255,255,128));
	UI::Line(ToScreen(br_world.x,tl_world.y), ToScreen(br_world.x,br_world.y), 1, PackColorU32(255,255,255,128));
	UI::Line(ToScreen(br_world.x,br_world.y), ToScreen(tl_world.x,br_world.y), 1, PackColorU32(255,255,255,128));
	UI::Line(ToScreen(tl_world.x,br_world.y), ToScreen(tl_world.x,tl_world.y), 1, PackColorU32(255,255,255,128));
	
	vec2f64 tl = WorldToGraph(tl_world, graph);
	vec2f64 br = WorldToGraph(br_world, graph);
	//round to nearest multiple of major_increment (favoring away from zero)
	f64 tl_x = floor(f64(tl.x) / graph->gridMajorLinesIncrement) * graph->gridMajorLinesIncrement;
	f64 tl_y = ceil (f64(tl.y) / graph->gridMajorLinesIncrement) * graph->gridMajorLinesIncrement;
	f64 br_x = ceil (f64(br.x) / graph->gridMajorLinesIncrement) * graph->gridMajorLinesIncrement;
	f64 br_y = floor(f64(br.y) / graph->gridMajorLinesIncrement) * graph->gridMajorLinesIncrement;
	
	//draw grid lines //TODO try to combine these loops
	//TODO fix the graph lines overlapping the border
	for(f64 x = tl_x; x < br_x; x += graph->gridMajorLinesIncrement){
		int minor_idx = 0;
		
		if(x >= tl.x && x <= br.x){
			if(x == 0){
				UI::Line(GraphToScreen(0,tl.y,graph), GraphToScreen(0,br.y,graph), 1, Color_Green);
			}else if(0 >= br.y && 0 <= tl.y){
				if(graph->gridShowMajorLines){
					UI::Line(GraphToScreen(x,tl.y,graph), GraphToScreen(x,br.y,graph), 1, PackColorU32(255,255,255,64));
					minor_idx = 1;
				}
				if(graph->gridShowAxisCoords){
                    UI::PushColor(UIStyleCol_Text, color(255, 255, 255, 128));
					UI::Text(to_string("%g",x).str, GraphToScreen(x,0,graph), UITextFlags_NoWrap);
                    UI::PopColor();
				}
			}
		}
		
		if(graph->gridShowMinorLines){
			for(; minor_idx <= graph->gridMinorLinesCount; minor_idx++){
				f64 minor_x = x + (minor_idx * graph->gridMinorLinesIncrement);
				if(minor_x <= tl.x || minor_x >= br.x) continue;
				UI::Line(GraphToScreen(minor_x,tl.y,graph), GraphToScreen(minor_x,br.y,graph), 1, PackColorU32(255,255,255,32));
			}
		}
	}
	for(f64 y = tl_y; y > br_y; y -= graph->gridMajorLinesIncrement){
		int minor_idx = 0;
		
		if(y >= br.y && y <= tl.y){
			if(y == 0){
				UI::Line(GraphToScreen(tl.x,0,graph), GraphToScreen(br.x,0,graph), 1, Color_Red);
			}else if(0 >= tl.x && 0 <= br.x){
				if(graph->gridShowMajorLines){
					UI::Line(GraphToScreen(tl.x,y,graph), GraphToScreen(br.x,y,graph), 1, PackColorU32(255,255,255,64));
					minor_idx = 1;
				}
				if(graph->gridShowAxisCoords){
                    UI::PushColor(UIStyleCol_Text, color(255, 255, 255, 128));
					UI::Text(to_string("%g",y).str, GraphToScreen(0,y,graph), UITextFlags_NoWrap);
                    UI::PopColor();
				}
			}
		}
		
		if(graph->gridShowMinorLines){
			for(; minor_idx <= graph->gridMinorLinesCount; minor_idx++){
				f64 minor_y = y - (minor_idx * graph->gridMinorLinesIncrement);
				if(minor_y <= br.y || minor_y >= tl.y) continue;
				UI::Line(GraphToScreen(tl.x,minor_y,graph), GraphToScreen(br.x,minor_y,graph), 1, PackColorU32(255,255,255,32));
			}
		}
	}
	
	//draw zero text
	if(0 >= tl.x && 0 <= br.x && 0 >= br.y && 0 <= tl.y){
        UI::PushColor(UIStyleCol_Text, color(255, 255, 255, 128));
		UI::Text("0", GraphToScreen(0,0,graph), UITextFlags_NoWrap);
        UI::PopColor();
	}
}


/////////////////
//// @canvas ////
/////////////////
void Canvas::
HandleInput(){
	mouse_pos_world = ToWorld(DeshInput->mousePos);
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	//// SetTool
	if     (DeshInput->KeyPressed(CanvasBind_SetTool_Navigation)){ previous_tool = active_tool; active_tool = CanvasTool_Navigation; }
	else if(DeshInput->KeyPressed(CanvasBind_SetTool_Context))   { previous_tool = active_tool; active_tool = CanvasTool_Context; }
	else if(DeshInput->KeyPressed(CanvasBind_SetTool_Expression)){ previous_tool = active_tool; active_tool = CanvasTool_Expression; }
	else if(DeshInput->KeyPressed(CanvasBind_SetTool_Pencil))    { previous_tool = active_tool; active_tool = CanvasTool_Pencil; }
	else if(DeshInput->KeyPressed(CanvasBind_SetTool_Graph))     { activeGraph = (activeGraph) ? 0 : graphs.data; }
	else if(DeshInput->KeyPressed(CanvasBind_SetTool_Previous))  { Swap(previous_tool, active_tool); }
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Camera
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
	if(DeshInput->KeyPressed(CanvasBind_Camera_ZoomIn) && !UI::AnyWinHovered()){
		if(!activeGraph){
			camera_zoom -= camera_zoom / 10.0; 
			camera_zoom  = Clamp(camera_zoom, 1e-37, 1e37);
		}else{
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
	if(DeshInput->KeyPressed(CanvasBind_Camera_ZoomOut) && !UI::AnyWinHovered()){ 
		if(!activeGraph){
			camera_zoom += camera_zoom / 10.0; 
			camera_zoom  = Clamp(camera_zoom, 1e-37, 1e37);
		}else{
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
	}
	
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
		if(activeElement){
			UI::TextF("Selected: %#x", activeElement);
			UI::TextF("Position: (%g,%g)", activeElement->pos.x,activeElement->pos.y);
			UI::TextF("Size:     (%g,%g)", activeElement->size.x,activeElement->size.y);
			UI::TextF("Cursor:   %d", activeElement->cursor);
			UI::TextF("Tokens:   %d", activeElement->tokens.count);
		}
		UI::End();
	}
#endif
	
	switch(active_tool){
		///////////////////////////////////////////////////////////////////////////////////////////////
		//// Navigation
		case CanvasTool_Navigation:{
			if(UI::AnyWinHovered()) return;
			if(DeshInput->KeyPressed(CanvasBind_Navigation_Pan)){
				camera_pan_active = true;
				camera_pan_mouse_pos = DeshInput->mousePos;
				if(!activeGraph){
					camera_pan_start_pos = camera_pos;
				}else{
					camera_pan_start_pos = activeGraph->cameraPosition;
				}
			}
			if(DeshInput->KeyDown(CanvasBind_Navigation_Pan)){
				if(!activeGraph){
					camera_pos = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - mouse_pos_world);
				}else{
					activeGraph->cameraPosition = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - mouse_pos_world);
				}
			}
			if(DeshInput->KeyReleased(CanvasBind_Navigation_Pan)){
				camera_pan_active = false;
			}
			if(DeshInput->KeyPressed(CanvasBind_Navigation_ResetPos)){
				if(!activeGraph){
					camera_pos = {0,0};
				}else{
					activeGraph->cameraPosition = {0,0};
				}
			}
			if(DeshInput->KeyPressed(CanvasBind_Navigation_ResetZoom)){
				if(!activeGraph){
					camera_zoom = 1.0;
				}else{
					activeGraph->cameraZoom = 1.0;
				}
			}
		}break;
		///////////////////////////////////////////////////////////////////////////////////////////////
		//// Context
		case CanvasTool_Context:{
			//if(UI::BeginContextMenu("canvas_context_menu")){
			//UI::EndContextMenu();
			//}
		}break;
		///////////////////////////////////////////////////////////////////////////////////////////////
		//// Expression
		case CanvasTool_Expression:{
			if(UI::AnyWinHovered()) return;
			if(DeshInput->KeyPressed(CanvasBind_Expression_Select)){
				activeElement = 0;
				forE(elements){
					if(Math::PointInRectangle(DeshInput->mousePos, ToScreen(it->pos.x, it->pos.y), it->size / (2 * camera_zoom) * (f32)DeshWindow->width)){
						activeElement = it;
					}
				}
			}
			
			if(DeshInput->KeyPressed(CanvasBind_Expression_Create)){
				elements.add(Element());
				activeElement = elements.last;
				activeElement->pos = vec2(mouse_pos_world.x, mouse_pos_world.y);
				activeElement->size = vec2(2, 1);
			}
			
			//handle token inputs
			if(activeElement){
				//moving cursor
				if      (DeshInput->KeyPressed(Key::LEFT) 
						 && activeElement->cursor >= 0){
					activeElement->cursor--;
				}else if(DeshInput->KeyPressed(Key::RIGHT)
						 && (activeElement->cursor < activeElement->tokens.count || activeElement->cursor == -1)){
					activeElement->cursor++;
				}
				
				//check for token inputs
				if(DeshInput->KeyPressed(Key::EQUALS | InputMod_AnyShift)) 
					activeElement->AddToken(tok_Plus);
				if(DeshInput->KeyPressed(Key::K8 | InputMod_AnyShift))     
					activeElement->AddToken(tok_Multiplication);
				if(DeshInput->KeyPressed(Key::SLASH))                      
					activeElement->AddToken(tok_Division);
				if(DeshInput->KeyPressed(Key::MINUS))                      
					activeElement->AddToken(tok_Negation);
			}
		}break;
		///////////////////////////////////////////////////////////////////////////////////////////////
		//// Pencil
		case CanvasTool_Pencil:{
			if(UI::AnyWinHovered()) return;
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
	
	mouse_pos_world = ToWorld(DeshInput->mousePos);
}

void Canvas::
Init(){
	elements.reserve(100);
	graphs.reserve(8);
	Graph graph; 
	graph.dimensions = ToWorld(DeshWindow->dimensions);
	graphs.add(graph);
	
	mathfontitalic = Storage::CreateFontFromFileTTF("STIXTwoText-Italic.otf", 100).second;
	mathfont = Storage::CreateFontFromFileTTF("STIXTwoMath-Regular.otf", 100).second;
	Assert((mathfont != Storage::NullFont()) && (mathfontitalic != Storage::NullFont()), "math fonts failed to load");
}

void Canvas::
Update(){
	UI::Begin("main_canvas", vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_Invisible | UIWindowFlags_NoInteract );
	
	HandleInput();
	DrawPencilStrokes();
	
	//draw canvas elements
	forE(graphs) DrawGraphGrid(it);
	for(Element& e : elements){
		e.Update();
	}						
	
	UI::TextF("Active Tool:   %s", canvas_tool_strings[active_tool]);
	UI::TextF("Previous Tool: %s", canvas_tool_strings[previous_tool]);
	UI::TextF("Selected Graph: %d", (activeGraph) ? u32(activeGraph-graphs.data) : -1);
	UI::TextF("%.3ffps", F_AVG(50, 1 / (DeshTime->frameTime / 1000)));
	UI::TextF("campos:  (%g, %g)",camera_pos.x,camera_pos.y);
	UI::TextF("camzoom: %g", camera_zoom);
	UI::TextF("camarea: (%g, %g)", WorldViewArea().x, WorldViewArea().y);
	
	UI::End();
}
