#pragma once
#ifndef SUUGU_CANVAS_H
#define SUUGU_CANVAS_H

#include "types.h"
#include "utils/array.h"
#include "utils/string.h"
#include "math/Math.h"
#include "core/ui.h"

struct Element {
	vec2 pos; //NOTE world space //TODO maybe cache a screen position for elements 
	vec2 size; //world size
	s32 cursor = 0; //for tracking where in the token array we are editing
	array<token> tokens; //list of tokens the user has input and their strings to show 
	Expression statement;
	
	void AddToken(TokenType t);
	//draws input boxes and tokens
	//TODO(sushi) add parameter for if element is active
	void Update();
};

//maybe make it so the canvas can store its own windows as well
struct Canvas {
	array<Element> elements;
	Element* activeElement = 0;
	bool gathering = 0;
    
    void HandleInput();
	void Init();
	void Update();
};

#endif //SUUGU_CANVAS_H