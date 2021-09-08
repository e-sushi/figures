////////////////
//// @state ////
////////////////
//camera internals
local vec2 camera_pos{0,0};
local f64  camera_zoom = 5.0;

//grid settings //TODO(delle) make settings menu/struct
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

//////////////////
//// @utility ////
//////////////////
local vec2 
ToScreen(vec2 point){
	point -= camera_pos;
	point /= camera_zoom;
	point.y *= -f32(DeshWindow->width) / f32(DeshWindow->height);
	point += vec2::ONE;
	point *= DeshWindow->dimensions / 2;
	return point;
}

local vec2 
ToWorld(vec2 point){
	point /= DeshWindow->dimensions;
	point *= 2;
	point -= vec2::ONE;
	point.y /= -f32(DeshWindow->width) / f32(DeshWindow->height);
	point *= camera_zoom;
	point += camera_pos;
	return point;
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
    vec2 winpos = ToScreen(pos);
    
    PushVar(UIStyleVar_WindowPadding, vec2{ 0,0 });
    PushVar(UIStyleVar_InputTextTextAlign, vec2{ 0, 0 });
    
    SetNextWindowPos(winpos);
    //NOTE: I dont think this way of dynamically naming actually works so
    BeginWindow(TOSTRING((char)this).str, vec2{ 0,0 }, vec2{ 300,300 }, UIWindowFlags_FitAllElements);;
    
    UI::BeginRow(tokens.count, 30);
    for (int i = 0; i < tokens.count; i++) {
        token curt = tokens[i];
        
        //cases where the user has the token selected
        if (i == cursor) {
            if (curt.type == tok_Literal) {
                SetNextItemActive();
                
                if(!curt.str[0] == '\0')
                    SetNextItemSize(vec2{ (f32)font->height, (f32)font->height });
                
                if (InputText((char*)TOSTRING((char)this + tokens.count).str, tokens[cursor].str, 255, UIInputTextFlags_NoBackground | UIInputTextFlags_AnyChangeReturnsTrue | UIInputTextFlags_FitSizeToText)) {
                    tokens[i].strSize = CalcTextSize(tokens[i].str);
                    //statement = Parser::parse(tokens);
                }
                
                //selection outline
                Rect(GetLastItemPos() - vec2::ONE, GetLastItemSize() + vec2::ONE, color{ 64, 64, 64, (u8)(255.f * (sinf(DeshTotalTime) + 1) / 2) });
            }
            //underline anything else for now
            else {
                
                Text(tokens[i].str, UITextFlags_NoWrap);
                Line(vec2{ GetLastItemScreenPos().x + font->width, GetLastItemScreenPos().y + (f32)font->height + 1 }, vec2{ GetLastItemScreenPos().x, GetLastItemScreenPos().y + (f32)font->height + 1 }, 1);
            }
        }
        else {
            if (!curt.str[0] == '\0')
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
//// @canvas ////
/////////////////
void Canvas::
HandleInput() {
    persist TIMER_START(dblClickTimer);
    f32 dblClickTime = 200;
    
    if (DeshInput->LMousePressed()) {
        //check that we're not clicking on an element that already exists
        bool selected = false;
        for (Element& e : elements) {
            if (Math::PointInRectangle(DeshInput->mousePos, ToScreen(e.pos), e.size)) {
                selected = true; activeElement = &e;
            }
        }
        
        
        if (!selected && TIMER_END(dblClickTimer) > dblClickTime) {
            TIMER_RESET(dblClickTimer);
            activeElement = 0;
        }
        else if (!selected && TIMER_END(dblClickTimer) < dblClickTime) {
            TIMER_RESET(dblClickTimer);
            
            elements.add(Element());
            activeElement = elements.last;
            
            activeElement->pos = ToWorld(DeshInput->mousePos);
        }
    }
    
    //handle token inputs
    if (activeElement) {
        //moving cursor
        if (DeshInput->KeyPressed(Key::LEFT  | InputMod_AnyCtrl) && activeElement->cursor >= 0) 
            activeElement->cursor--;
        else if (DeshInput->KeyPressed(Key::RIGHT | InputMod_AnyCtrl) && activeElement->cursor < activeElement->tokens.count || activeElement->cursor == -1) 
            activeElement->cursor++;
        
        //check for token inputs
        if (DeshInput->KeyPressed(Key::EQUALS | InputMod_AnyShift)) activeElement->AddToken(tok_Plus);
        if (DeshInput->KeyPressed(Key::K8 | InputMod_AnyShift))     activeElement->AddToken(tok_Multiplication);
        if (DeshInput->KeyPressed(Key::BACKSLASH))                  activeElement->AddToken(tok_Division);
        if (DeshInput->KeyPressed(Key::MINUS))                      activeElement->AddToken(tok_Negation);
    }
    
    //// camera inputs ////
    persist bool dragging = false;
    if(dragging || !UI::AnyWinHovered()){
        //TODO(delle) fix zoom consistency: out -> in -> out should return to orig value
        //zoom out
        if(DeshInput->ScrollDown()){ 
            if(DeshInput->ShiftDown()){
                camera_zoom *= 2.0;
            }else{
                camera_zoom += camera_zoom / 10.0; 
            }
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
        //zoom in
        if(DeshInput->ScrollUp()){
            if(DeshInput->ShiftDown()){ //TODO(delle) fix zoom shift
                camera_zoom /= 2.0;
            }else{
                camera_zoom -= camera_zoom / 10.0; 
            }
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
        
        //dragging camera
        static vec2 begin;
        static vec2 og;
        if (DeshInput->LMousePressed()) {
            og = camera_pos;
            begin = DeshInput->mousePos;
            dragging = true;
        }
        if (DeshInput->LMouseDown()) {
            camera_pos = og + (ToWorld(begin) - ToWorld(DeshInput->mousePos));
        }
        if (DeshInput->LMouseReleased()) dragging = false;
        
        //reset
        if(DeshInput->KeyPressed(Key::NUMPAD0)){
            //zoom
            if(DeshInput->ShiftDown()){
                camera_zoom = 5;
            }
            //position
            else{
                camera_pos = {0,0};
            }
        }
    }
}

void 
DrawGridLines(){
    vec2 screen = DeshWindow->dimensions;
    vec2 tl = ToWorld({0,0});
    vec2 br = ToWorld(screen);
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
    UI::Line(ToScreen({tl.x,0}), ToScreen({br.x,0}), 1, Color_Red);
    UI::Line(ToScreen({0,tl.y}), ToScreen({0,br.y}), 1, Color_Green);
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
#endif
}

void Canvas::
Init() {
    elements.reserve(100);
}

void Canvas::
Update() {
    //begin main canvas
    UI::SetNextWindowSize(DeshWindow->dimensions);
    UI::BeginWindow("main_canvas", vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_Invisible | UIWindowFlags_DontSetGlobalHoverFlag | UIWindowFlags_NoScroll);
    //if (DeshInput->RMousePressed() || gathering) GatherInput(DeshInput->mousePos);
    
    HandleInput();
    if(showGrid) DrawGridLines();
    
    //draw canvas elements
    for (Element& e : elements) {
        //string send = "";
        //for (token& t : e.tokens)
        //	send += t.str;
        if (activeElement == &e) {
            //UI::RectFilled(ToScreen(e.pos, camera_pos, camera_zoom), (e.size.x == 0 ? vec2{ 11, 11 } : e.size), color{ 100, 100, 155, 150 });
            //LOG(e.size);
        }
        e.Update();
        //UI::Text(send, ToScreen(e.pos, camera_pos, camera_zoom), UITextFlags_NoWrap);
        //send.clear();
    }
    
    UI::Text(to_string("(%g,%g)",camera_pos.x,camera_pos.y).str);
    UI::EndWindow();
}
