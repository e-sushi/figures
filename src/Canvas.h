#pragma once
#ifndef SUUGU_CANVAS_H
#define SUUGU_CANVAS_H

#include "types.h"
#include "utils/array.h"
#include "utils/string.h"
#include "math/Math.h"
#include "core/ui.h"

struct Element{
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

struct Graph{ //TODO maybe inherit Element?
	vec2f64 position{0,0};
	vec2f64 dimensions{2,-1.25};
	vec2f64 cameraPosition{0,0}; //in graph space
	f64     cameraZoom = 5.0;
	
	f64 gridZoomFit               = 5.0;
	f64 gridZoomFitIncrements[3]  = {2.0, 2.5, 2.0};
	u32 gridZoomFitIncrementIndex = 0;
	u32 gridMajorLinesCount     = 12;
	f64 gridMajorLinesIncrement = 1.0;
	u32 gridMinorLinesCount     = 4;
	f64 gridMinorLinesIncrement = 0.2;
	b32 gridShowMajorLines = true;
	b32 gridShowMinorLines = true;
	b32 gridShowAxisCoords = true;
};

//maybe make it so the canvas can store its own windows as well
struct Canvas{
	Element* activeElement = 0;
	Graph*   activeGraph = 0;
	bool gathering = 0;
	
	array<Element> elements;
	array<Graph> graphs;
    
    void HandleInput();
	void Init();
	void Update();
};

#endif //SUUGU_CANVAS_H