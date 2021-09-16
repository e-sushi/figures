////////////////
//// @tools ////
////////////////
enum CanvasTool_{
    CanvasTool_Navigation,
    CanvasTool_Context,
    CanvasTool_Expression,
    CanvasTool_Pencil,
}; typedef u32 CanvasTool;
const char* CanvasToolStrings[] = {
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
    CanvasBind_SetTool_Previous   = Key::MBFOUR  | InputMod_None,
    
    //[GLOBAL] Camera 
    CanvasBind_Camera_Pan     = Key::MBMIDDLE     | InputMod_None, //pressed, held
    CanvasBind_Camera_ZoomIn  = Key::MBSCROLLUP   | InputMod_None,
    CanvasBind_Camera_ZoomOut = Key::MBSCROLLDOWN | InputMod_None,
    
    //[LOCAL]  Navigation 
    CanvasBind_Navigation_Pan       = Key::MBLEFT  | InputMod_Any, //pressed, held
    CanvasBind_Navigation_ResetPos  = Key::NUMPAD0 | InputMod_None,
    CanvasBind_Navigation_ResetZoom = Key::NUMPAD0 | InputMod_AnyShift,
    
    //[LOCAL]  Context
    
    
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

/////////////////
//// @camera ////
/////////////////
local vec2f64 camera_pos{0,0};
local f64     camera_zoom = 5.0;
local vec2f64 camera_pan_start_pos;
local vec2    camera_pan_mouse_pos;
local bool    camera_pan_active = false;

///////////////
//// @grid ////
///////////////
//grid settings
local bool showGrid           = true;
local bool showGridMajorLines = true;
local bool showGridMinorLines = true;
local bool showGridAxisCoords = true;
//grid internals
local f64 grid_zoom_fit        = 5.0;
local s32 grid_major_count     = 12;
local f64 grid_major_increment = 1.0;
local s32 grid_minor_count     = 4;
local f64 grid_minor_increment = 0.2;
local f64 grid_zoom_fit_increments[3] = {2.0, 2.5, 2.0};
local u32 grid_zoom_fit_increment_index = 0;

/////////////////
//// @pencil ////
/////////////////
struct PencilStroke{
    f64   size;
    color color;
    array<vec2f64> pencil_points;
};
local array<PencilStroke> pencil_strokes;
local u32     pencil_stroke_idx  = 0;
local f64     pencil_stroke_size = 1;
local color   pencil_stroke_color = PackColorU32(249,195,69,255);
local vec2f64 pencil_stroke_start_pos;
local u32     pencil_draw_skip_amount = 4;

//////////////////
//// @utility ////
//////////////////
local vec2 
ToScreen(vec2 point){
	point.x -= camera_pos.x; point.y -= camera_pos.y;
	point /= camera_zoom;
	point.y *= -f32(DeshWindow->width) / f32(DeshWindow->height);
	point += vec2::ONE;
	point *= DeshWindow->dimensions / 2;
	return point;
}

inline local vec2 
ToScreen(f64 x, f64 y){
	return ToScreen(vec2(x,y));
}

local vec2f64 
ToWorld(f64 x, f64 y){
    vec2f64 point{x, y};
	point.x /= f64(DeshWindow->width); point.y /= f64(DeshWindow->height);
	point *= 2;
	point -= {1,1};
	point.y /= -f64(DeshWindow->width) / f64(DeshWindow->height);
	point *= camera_zoom;
	point += camera_pos;
	return point;
};

inline local vec2f64 
ToWorld(vec2 point){
	return ToWorld(point.x, point.y);
};

//////////////////
//// @element ////
//////////////////
void Element::
CalcSize() {
    string s = "";
    for (token& t : tokens) {
        s += t.str;
        t.strSize = UI::CalcTextSize(t.str);
    }
    size = UI::CalcTextSize(s);
}

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
        else if (t == tok_LogicalNOT || t == tok_BitwiseComplement || tok_Negation) {
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
            else if (t == tok_LogicalNOT || t == tok_BitwiseComplement || tok_Negation) {
                tokens.add(token(t));
                tokens.add(token(tok_Literal));
                cursor = tokens.count - 2; //position cursor inside first tok_Literal box 
            }
        }
    }
    
    CalcSize();
    statement = Parser::parse(tokens);
}

void Element::
Update() {
    using namespace UI;
    Font* font = UI::GetStyle().font;
    vec2 winpos = ToScreen(pos.x, pos.y);
    
    PushVar(UIStyleVar_WindowPadding, vec2{ 0,0 });
    PushVar(UIStyleVar_InputTextTextAlign, vec2{ 0, 0 });
    
    SetNextWindowPos(winpos);
    //NOTE: I dont think this way of dynamically naming actually works so
    BeginWindow(TOSTRING((char)this).str, vec2{ 0,0 }, vec2{ 300,300 }, UIWindowFlags_FitAllElements);
    
    UI::BeginRow(tokens.count, 30);
    for (int i = 0; i < tokens.count; i++) {
        token curt = tokens[i];
        
        if(curt.type != tok_Literal)
            RowSetupRelativeColumnWidth(i + 1, 2);
        else
            RowSetupRelativeColumnWidth(i + 1, 1);
        
        //cases where the user has the token selected
        if (i == cursor) {
            if (curt.type == tok_Literal) {
                SetNextItemActive();
                
                if(!curt.str[0])
                    SetNextItemSize(vec2{ (f32)font->height, (f32)font->height });
                
                if (InputText((char*)TOSTRING((char)this + tokens.count).str, tokens[cursor].str, 255, UIInputTextFlags_NoBackground | UIInputTextFlags_AnyChangeReturnsTrue | UIInputTextFlags_FitSizeToText | UIInputTextFlags_Numerical)) {
                    tokens[i].strSize = CalcTextSize(tokens[i].str);
                    statement = Parser::parse(tokens);
                }
                
                //selection outline
                RectFilled(GetLastItemScreenPos() - vec2::ONE, GetLastItemSize() + vec2::ONE, color{ 64, 64, 64, (u8)(175.f * (sinf(3 * DeshTotalTime) + 1) / 2) });
                
            }
            //underline anything else for now
            else {
                
                Text(tokens[i].str, UITextFlags_NoWrap);
                Line(vec2{ GetLastItemScreenPos().x + font->width, GetLastItemScreenPos().y + (f32)font->height + 1 }, vec2{ GetLastItemScreenPos().x, GetLastItemScreenPos().y + (f32)font->height + 1 }, 1);
            }
        }
        else {
            if (!curt.str[0])
                SetNextItemSize(vec2{ (f32)font->height, (f32)font->height });
            UI::Text(tokens[i].str, UITextFlags_NoWrap);
            
            
        }
    }
    UI::EndRow();
    
    EndWindow();
    PopVar(2);
    //UI::ShowDebugWindowOf(TOSTRING((char)this).str);
}


/////////////////
//// @pencil ////
/////////////////
local void 
DrawPencilStrokes(){
    UI::BeginWindow("pencil_canvas", vec2::ZERO, DeshWindow->dimensions,
                    UIWindowFlags_Invisible | UIWindowFlags_DontSetGlobalHoverFlag | UIWindowFlags_NoScroll);
    forE(pencil_strokes){
        for(u32 point_idx = pencil_draw_skip_amount; 
            point_idx < it->pencil_points.count; 
            point_idx += pencil_draw_skip_amount)
        {
            vec2f64 prev = it->pencil_points[point_idx-pencil_draw_skip_amount];
            vec2f64 curr = it->pencil_points[point_idx];
            UI::Line(ToScreen(prev.x, prev.y), ToScreen(curr.x, curr.y), it->size, it->color);
        }
    }
    UI::EndWindow();
}

///////////////
//// @grid ////
///////////////
local void 
DrawGridLines(){
    vec2 screen = DeshWindow->dimensions;
    vec2f64 tl = ToWorld({0,0});
    vec2f64 br = ToWorld(screen);
    //round to nearest multiple of major_increment (favoring away from zero)
    f64 gx = floor(f64(tl.x) / grid_major_increment) * grid_major_increment;
    f64 gy = ceil (f64(tl.y) / grid_major_increment) * grid_major_increment;
    f64 ex = ceil (f64(br.x) / grid_major_increment) * grid_major_increment;
    f64 ey = floor(f64(br.y) / grid_major_increment) * grid_major_increment;
    
    //draw grid lines
    for(f64 x = gx; x < ex; x += grid_major_increment){
        vec2 coord = ToScreen({f32(x),0});
        if(showGridAxisCoords && x != 0){
            UI::Text(to_string("%g",x).str, coord, PackColorU32(255,255,255,128), UITextFlags_NoWrap);
        }
        int minor_idx;
        if(showGridMajorLines){
            UI::Line({coord.x,0}, {coord.x,screen.y}, 1, PackColorU32(255,255,255,64));
            minor_idx = 1;
        }else{
            minor_idx = 0;
        }
        if(showGridMinorLines){
            for(; minor_idx<=grid_minor_count; minor_idx++){
                coord = ToScreen({f32(x+(minor_idx*grid_minor_increment)),0});
                UI::Line({coord.x,0}, {coord.x,screen.y}, 1, PackColorU32(255,255,255,32));
            }
        }
    }
    for(f64 y = gy; y > ey; y -= grid_major_increment){
        vec2 coord = ToScreen({0,f32(y)});
        if(showGridAxisCoords && y != 0){
            UI::Text(to_string("%g",y).str, coord, PackColorU32(255,255,255,128), UITextFlags_NoWrap);
        }
        int minor_idx;
        if(showGridMajorLines){
            UI::Line({0,coord.y}, {screen.x,coord.y}, 1, PackColorU32(255,255,255,64));
            minor_idx = 1;
        }else{
            minor_idx = 0;
        }
        if(showGridMinorLines){
            for(; minor_idx<=grid_minor_count; minor_idx++){
                coord = ToScreen({0,f32(y-(minor_idx*grid_minor_increment))});
                UI::Line({0,coord.y}, {screen.x,coord.y}, 1, PackColorU32(255,255,255,32));
            }
        }
    }
    
    //draw x and y axis
    UI::Line(ToScreen(tl.x,0), ToScreen(br.x,0), 1, Color_Red);
    UI::Line(ToScreen(0,tl.y), ToScreen(0,br.y), 1, Color_Green);
    UI::Text("0", ToScreen({0,0}), PackColorU32(255,255,255,128), UITextFlags_NoWrap);
#if 0 //debug text
    UI::Text(TOSTRING(  "pos: ",camera_pos,
                      "\nzoom:",camera_zoom,
                      "\ntl:  ",tl,
                      "\nbr:  ",br,
                      "\ngx:  ",gx,
                      "\ngy:  ",gy,
                      "\nex:  ",ex,
                      "\ney:  ",ey,
                      "\nzf:  ",grid_zoom_fit,
                      "\nmac: ",grid_major_count,
                      "\nmai: ",grid_major_increment,
                      "\nmic: ",grid_minor_count,
                      "\nmii: ",grid_minor_increment
                      )
             .str);
    UI::TextF("zoom:%g",camera_zoom);
#endif
}

/////////////////
//// @canvas ////
/////////////////
void Canvas::
HandleInput() {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    //// SetTool
    if     (DeshInput->KeyPressed(CanvasBind_SetTool_Navigation)){ previous_tool = active_tool; active_tool = CanvasTool_Navigation; }
    else if(DeshInput->KeyPressed(CanvasBind_SetTool_Context))   { previous_tool = active_tool; active_tool = CanvasTool_Context; }
    else if(DeshInput->KeyPressed(CanvasBind_SetTool_Expression)){ previous_tool = active_tool; active_tool = CanvasTool_Expression; }
    else if(DeshInput->KeyPressed(CanvasBind_SetTool_Pencil))    { previous_tool = active_tool; active_tool = CanvasTool_Pencil; }
    else if(DeshInput->KeyPressed(CanvasBind_SetTool_Previous))  { Swap(previous_tool, active_tool); }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    //// Camera
    if(DeshInput->KeyPressed(CanvasBind_Camera_Pan)){
        camera_pan_start_pos = camera_pos;
        camera_pan_mouse_pos = DeshInput->mousePos;
        camera_pan_active  = true;
    }
    if(DeshInput->KeyDown(CanvasBind_Camera_Pan)){
        camera_pos = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - ToWorld(DeshInput->mousePos));
    }
    if(DeshInput->KeyReleased(CanvasBind_Camera_Pan)){
        camera_pan_active = false;
    }
    //TODO(delle) fix zoom consistency: out -> in -> out should return to orig value
    if(DeshInput->KeyPressed(CanvasBind_Camera_ZoomIn) && !UI::AnyWinHovered()){
        camera_zoom -= camera_zoom / 10.0; 
        camera_zoom = ((camera_zoom < 1e-37) ? 1e-37 : ((camera_zoom > 1e37) ? 1e37 : (camera_zoom)));
        f64 prev_grid_zoom_fit = (grid_zoom_fit_increment_index == 0) ? 
            grid_zoom_fit/grid_zoom_fit_increments[2] : grid_zoom_fit/grid_zoom_fit_increments[grid_zoom_fit_increment_index-1];
        if(camera_zoom < (prev_grid_zoom_fit)+grid_major_increment){
            grid_zoom_fit = prev_grid_zoom_fit;
            //major_count = 12;
            grid_major_increment = grid_zoom_fit / 5.0;
            grid_minor_count = (grid_zoom_fit_increment_index == 2) ? 3 : 4;
            grid_minor_increment = grid_major_increment / f64(grid_minor_count + 1);
            grid_zoom_fit_increment_index -= 1;
            if(grid_zoom_fit_increment_index == -1) grid_zoom_fit_increment_index = 2;
            Assert(grid_zoom_fit_increment_index < 3);
        }
    }
    if(DeshInput->KeyPressed(CanvasBind_Camera_ZoomOut) && !UI::AnyWinHovered()){ 
        camera_zoom += camera_zoom / 10.0; 
        camera_zoom = ((camera_zoom < 1e-37) ? 1e-37 : ((camera_zoom > 1e37) ? 1e37 : (camera_zoom)));
        if(camera_zoom > grid_zoom_fit+grid_major_increment){
            grid_zoom_fit *= grid_zoom_fit_increments[grid_zoom_fit_increment_index];
            //major_count = 12;
            grid_major_increment = grid_zoom_fit / 5.0;
            grid_minor_count = (grid_zoom_fit_increment_index == 0) ? 3 : 4;
            grid_minor_increment = grid_major_increment / f64(grid_minor_count + 1);
            grid_zoom_fit_increment_index = (grid_zoom_fit_increment_index + 1) % 3;
            Assert(grid_zoom_fit_increment_index < 3);
        }
    }
    
#if 1
    if(active_tool == CanvasTool_Pencil){
        UI::BeginWindow("pencil_debug", {10,10}, {200,200}, UIWindowFlags_FitAllElements);
        UI::TextF("Stroke Size:   %f", pencil_stroke_size);
        UI::TextF("Stroke Color:  %x", pencil_stroke_color.rgba);
        UI::TextF("Stroke Start:  (%g,%g)", pencil_stroke_start_pos.x, pencil_stroke_start_pos.y);
        UI::TextF("Stroke Index:  %d", pencil_stroke_idx);
        UI::TextF("Stroke Skip:   %d", pencil_draw_skip_amount);
        if(pencil_stroke_idx > 0) UI::TextF("Stroke Points: %d", pencil_strokes[pencil_stroke_idx-1].pencil_points.count);
        u32 total_stroke_points = 0;
        forE(pencil_strokes) total_stroke_points += it->pencil_points.count;
        UI::TextF("Total Points:  %d", total_stroke_points);
        UI::EndWindow();
    }
    if(active_tool == CanvasTool_Expression){
        UI::BeginWindow("expression_debug", {10,10}, {200,200}, UIWindowFlags_FitAllElements);
        UI::TextF("Elements: %d", elements.count);
        if(activeElement){
            UI::TextF("Selected: %#x", activeElement);
            UI::TextF("Position: (%g,%g)", activeElement->pos.x,activeElement->pos.y);
            UI::TextF("Size:     (%g,%g)", activeElement->size.x,activeElement->size.y);
            UI::TextF("Cursor:   %d", activeElement->cursor);
            UI::TextF("Tokens:   %d", activeElement->tokens.count);
        }
        UI::EndWindow();
    }
#endif
    
    //skip the rest of input if a UI window is hovered
    if(UI::AnyWinHovered()) return;
    switch(active_tool){
        ///////////////////////////////////////////////////////////////////////////////////////////////
        //// Navigation
        case CanvasTool_Navigation:{
            if(DeshInput->KeyPressed(CanvasBind_Navigation_Pan)){
                camera_pan_start_pos = camera_pos;
                camera_pan_mouse_pos = DeshInput->mousePos;
                camera_pan_active    = true;
            }
            if(DeshInput->KeyDown(CanvasBind_Navigation_Pan)){
                camera_pos = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - ToWorld(DeshInput->mousePos));
            }
            if(DeshInput->KeyReleased(CanvasBind_Navigation_Pan)){
                camera_pan_active = false;
            }
            if(DeshInput->KeyPressed(CanvasBind_Navigation_ResetPos)){
                camera_pos = {0,0};
            }
            if(DeshInput->KeyPressed(CanvasBind_Navigation_ResetZoom)){
                camera_zoom = 5;
            }
        }break;
        ///////////////////////////////////////////////////////////////////////////////////////////////
        //// Context
        case CanvasTool_Context:{
            //TODO context binds
        }break;
        ///////////////////////////////////////////////////////////////////////////////////////////////
        //// Expression
        case CanvasTool_Expression:{
            if(DeshInput->KeyPressed(CanvasBind_Expression_Select)){
                activeElement = 0;
                forE(elements){
                    if(Math::PointInRectangle(DeshInput->mousePos, ToScreen(it->pos.x, it->pos.y), it->size)){
                        activeElement = it;
                    }
                }
            }
            
            if(DeshInput->KeyPressed(CanvasBind_Expression_Create)){
                elements.add(Element());
                activeElement = elements.last;
                activeElement->pos = ToWorld(DeshInput->mousePos);
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
                if(DeshInput->KeyPressed(Key::EQUALS | InputMod_AnyShift)) activeElement->AddToken(tok_Plus);
                if(DeshInput->KeyPressed(Key::K8 | InputMod_AnyShift))     activeElement->AddToken(tok_Multiplication);
                if(DeshInput->KeyPressed(Key::SLASH))                      activeElement->AddToken(tok_Division);
                if(DeshInput->KeyPressed(Key::MINUS))                      activeElement->AddToken(tok_Negation);
            }
        }break;
        ///////////////////////////////////////////////////////////////////////////////////////////////
        //// Pencil
        case CanvasTool_Pencil:{
            if(DeshInput->KeyPressed(CanvasBind_Pencil_Stroke)){
                PencilStroke new_stroke;
                new_stroke.size  = pencil_stroke_size;
                new_stroke.color = pencil_stroke_color;
                pencil_strokes.add(new_stroke);
                pencil_stroke_start_pos = ToWorld(DeshInput->mouseX, DeshInput->mouseY);
            }
            if(DeshInput->KeyDown(CanvasBind_Pencil_Stroke)){
                vec2f64 mouse_world = ToWorld(DeshInput->mouseX, DeshInput->mouseY);
                pencil_strokes[pencil_stroke_idx].pencil_points.add(mouse_world);
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

void Canvas::
Init(){
    elements.reserve(100);
}

void Canvas::
Update(){
    UI::SetNextWindowSize(DeshWindow->dimensions);
    UI::BeginWindow("main_canvas", vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_Invisible | UIWindowFlags_DontSetGlobalHoverFlag | UIWindowFlags_NoScroll);
    //if (DeshInput->RMousePressed() || gathering) GatherInput(DeshInput->mousePos);
    
    HandleInput();
    if(showGrid) DrawGridLines();
    DrawPencilStrokes();
    
    //draw canvas elements
    for (Element& e : elements) {
        e.Update();
    }
    
    UI::TextF("Active Tool:   %s", CanvasToolStrings[active_tool]);
    UI::TextF("Previous Tool: %s", CanvasToolStrings[previous_tool]);
    UI::TextF("%.3fms", DeshTime->frameTime);
    UI::TextF("(%g,%g)",camera_pos.x,camera_pos.y);
    UI::EndWindow();
}
