#pragma once
#ifndef SUUGU_CANVAS_H
#define SUUGU_CANVAS_H

#include "utils/array.h"
#include "utils/string.h"
#include "math/Math.h"
#include "core/ui.h"

#include "types.h"

local vec2 ToScreen(vec2 point, vec2 cameraPos, f32 cameraZoom) {
	point -= cameraPos;
	point /= cameraZoom;
    
	point.y *= -(f32)DeshWindow->width / DeshWindow->height;
	point += vec2(1, 1);
	point *= DeshWindow->dimensions / 2;
    
	return point;
}

local vec2 ToWorld(vec2 point, vec2 cameraPos, f32 cameraZoom) {
	point /= DeshWindow->dimensions;
	point *= 2;
	point -= vec2::ONE;
	point.y /= -(f32)DeshWindow->width / DeshWindow->height;
    
	point *= cameraZoom;
	point += cameraPos;
	return point;
};

u32 expandtokcount = 0;

struct Element {
	vec2 pos; //maybe cache a screen position for elements 
	vec2 size;
    
	s32 cursor = 0; //for tracking where in the token array we are editing

	//list of tokens the user has input and their strings to show 
	array<token> tokens;
    
	Statement statement;
	
	void AddToken(TokenType t) {

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
	
	//temporarily text based, but when we get to custom positioning of
	//glyphs this will have to be different
	void CalcSize() {
		string s = "";
		for (token& t : tokens) {
			s += t.str;
			t.strSize = UI::CalcTextSize(t.str);
		}
		size = UI::CalcTextSize(s);
        
	}

	//draws input boxes and tokens
	//TODO(sushi) add parameter for if element is active
	void Update(vec2 cameraPos, float cameraZoom) {
		
		//temp way of drawing a background
		//maybe save size info on each token and only calculate that when they change
		CalcSize();
		UI::RectFilled(ToScreen(pos, cameraPos, cameraZoom), size, color{ 15, 100, 110, 125 });
		UI::PushVar(UIStyleVar_InputTextTextAlign, vec2(0, 0));


		f32 posOffset = 0;
		for (int i = 0; i < tokens.count; i++) {
			vec2 lastTokSize = (i != 0 ? tokens[i - 1].strSize : vec2(0,0));
			vec2 tokSize = tokens[i].strSize;
			posOffset += (i != 0 ? lastTokSize.x : 0);

			vec2 screenPos = ToScreen(pos, cameraPos, cameraZoom);
			vec2 placement = vec2{ screenPos.x + posOffset, screenPos.y };
			//special cases for when we are drawing and reach the location of the cursor
			if (i == cursor) {
				//the cursor is in a literal's edit box so we let it edit the literal
				UI::Rect(placement, tokSize, color::DARK_CYAN);
				if (tokens[cursor].type == tok_Literal) {
					UI::SetNextItemActive();
					if (UI::InputText(TOSTRING((char)this + tokens.count), tokens[cursor].str, -1, placement, UIInputTextFlags_NoBackground | UIInputTextFlags_AnyChangeReturnsTrue)) {
						tokens[i].strSize = UI::CalcTextSize(tokens[i].str);
						statement = Parser::parse(tokens);
					}
				}
				//underline anything else for now
				else {
					UI::Text(tokens[i].str, placement, UITextFlags_NoWrap);
					UI::Line(vec2{ placement.x + 6, screenPos.y + 12 }, vec2{ placement.x, screenPos.y + 12 }, 1);
				}
			}
			else {
				UI::Text(tokens[i].str, placement, UITextFlags_NoWrap);
			}
		}

		UI::PopVar();
		
	}
    
};

//maybe make it so the canvas can store its own windows as well
struct Canvas {
	array<Element> elements;
	bool gathering = 0;
    
	Element* activeElement = 0;
    
	vec2 cameraPos{ 0,0 };
	float cameraZoom = 5;
    
	void Init() {
		elements.reserve(100);
	}
    
	void HandleInput() {
		persist TIMER_START(dblClickTimer);
		f32 dblClickTime = 200;
        
		if (DeshInput->LMousePressed()) {
			//check that we're not clicking on an element that already exists
			bool selected = false;
			for (Element& e : elements) {
				if (Math::PointInRectangle(DeshInput->mousePos, ToScreen(e.pos, cameraPos, cameraZoom), e.size)) {
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
                
				activeElement->pos = ToWorld(DeshInput->mousePos, cameraPos, cameraZoom);
			}
		}
        
		//handle token inputs
		//this is only for non-literal tokens
		//this may also be better handled entirely within element, rather than partially
		//this should only do this if a key is pressed but for some reason 
		//AnyKeyPressed() doesnt work with mods rn
		if (activeElement) {
			//moving cursor
			if (DeshInput->KeyPressed(Key::LEFT  | InputMod_AnyCtrl) && activeElement->cursor >= 0) 
				activeElement->cursor--;
			else if (DeshInput->KeyPressed(Key::RIGHT | InputMod_AnyCtrl) && activeElement->cursor < activeElement->tokens.count || activeElement->cursor == -1) 
				activeElement->cursor++;
            
			//use UI::InputText to get an input 
			string buffer = "";
			UI::SetNextItemActive();
			UI::InputText("dummy_input", buffer, -1, vec2{ -100, -100 });
            
			if (buffer.size > 0) {
				if (TokenType* t = strToTok.at(buffer)) {
					activeElement->AddToken(*t);
				}
			}
		}
	}
    
	
    
	void DrawGridLines() {
        
		u32 lines = 30;
        
		f32 xp = floor(cameraPos.x) + lines;
		f32 xn = floor(cameraPos.x) - lines;
		f32 yp = floor(cameraPos.y) + lines;
		f32 yn = floor(cameraPos.y) - lines;
        
		for (int i = 0; i < lines * 2 + 1; i++) {
			vec2 v1 = ToScreen(vec2{ xn + i, yn }, cameraPos, cameraZoom);
			vec2 v2 = ToScreen(vec2{ xn + i, yp }, cameraPos, cameraZoom);
			vec2 v3 = ToScreen(vec2{ xn, yn + i }, cameraPos, cameraZoom);
			vec2 v4 = ToScreen(vec2{ xp, yn + i }, cameraPos, cameraZoom);
            
			UI::Line(v1, v2, 1, color(255, 255, 255, 100));
			UI::Line(v3, v4, 1, color(255, 255, 255, 100));
		}
        
		for (f32 i = -5; i <= 5; i++) {
			vec2 posx{ i, 0 };
			vec2 posy{ 0, i };
            
			UI::Text(TOSTRING(i), ToScreen(posx, cameraPos, cameraZoom), UITextFlags_NoWrap);
			UI::Text(TOSTRING(i), ToScreen(posy, cameraPos, cameraZoom), UITextFlags_NoWrap);
		}
	}
    
    
    
	void HandleCamera() {
		persist bool dragging = false;
		if (dragging || !UI::AnyWinHovered()) {
			//zoomin
			if (DeshInput->ScrollDown()) { cameraZoom = Math::clamp(cameraZoom + cameraZoom / 10, 0.01, 10); }
			if (DeshInput->ScrollUp()) { cameraZoom = Math::clamp(cameraZoom - cameraZoom / 10, 0.01, 10); }
            
			//dragging camera
			static vec2 begin;
			static vec2 og;
			if (DeshInput->LMousePressed()) {
				og = cameraPos;
				begin = DeshInput->mousePos;
				dragging = true;
			}
			if (DeshInput->LMouseDown()) {
				cameraPos = og + (ToWorld(begin, cameraPos, cameraZoom) - ToWorld(DeshInput->mousePos, cameraPos, cameraZoom));
			}
			if (DeshInput->LMouseReleased()) dragging = false;
		}
	}
    
	void Update() {
        
        
		//begin main canvas
		UI::SetNextWindowSize(DeshWindow->dimensions);
		UI::BeginWindow("main_canvas", vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_Invisible | UIWindowFlags_DontSetGlobalHoverFlag);
        
		//if (DeshInput->RMousePressed() || gathering) GatherInput(DeshInput->mousePos);
		
		HandleInput();
		HandleCamera();
		DrawGridLines();
		
		//draw canvas elements
		for (Element& e : elements) {
			//string send = "";
			//for (token& t : e.tokens)
			//	send += t.str;
			if (activeElement == &e) {
				//UI::RectFilled(ToScreen(e.pos, cameraPos, cameraZoom), (e.size.x == 0 ? vec2{ 11, 11 } : e.size), color{ 100, 100, 155, 150 });
				//LOG(e.size);
			}
			e.Update(cameraPos, cameraZoom);
			//UI::Text(send, ToScreen(e.pos, cameraPos, cameraZoom), UITextFlags_NoWrap);
			//send.clear();
		}
        
		UI::Text(TOSTRING(cameraPos));
		UI::EndWindow();
        
	}
    
};

#endif //SUUGU_CANVAS_H