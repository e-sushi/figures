#ifndef SUUGU_CANVAS_H
#define SUUGU_CANVAS_H

#include "utils/array.h"
#include "utils/string.h"
#include "math/Math.h"
#include "core/ui.h"

//placeholder struct representing an object on the canvas 
struct temp {
	vec2 pos;
	string input;
};

//maybe make it so the canvas can store its own windows as well
struct Canvas {
	array<temp> objs;
	bool gathering = 0;
	string buffer = "";


	vec2 cameraPos{ 0,0 };
	float cameraZoom = 5;

	void Init() {
		objs.reserve(100);
	}

	void AddObj(vec2 pos, string input) {
		objs.add(temp{ pos, input });
	}
	
	//playing around with a design for this, it will most likely be changed in the future
	void GatherInput(vec2 mousePos) {
		persist vec2 mp;

		UI::SetNextItemActive();
		if (gathering && UI::InputText("canvasinput", buffer, ToScreen(mp))) {
			AddObj(mp, buffer);
			buffer.clear();
			gathering = 0;
		}
		else if(!gathering) {
			gathering = 1;
			mp = ToWorld(mousePos);
		}

		//check for inputs that cancel input, currently only esc
		if (gathering) {
			if (DeshInput->KeyPressedAnyMod(Key::ESCAPE)) {
				gathering = 0;
				buffer.clear();
			}
		}
	}

	vec2 ToScreen(vec2 point) {
		point -= cameraPos;
		point /= cameraZoom;

		point.y *= -(f32)DeshWindow->width / DeshWindow->height;
		point += vec2(1, 1);
		point *= DeshWindow->dimensions / 2;

		return point;
	}

	vec2 ToWorld(vec2 point) {
		point /= DeshWindow->dimensions;
		point *= 2;
		point -= vec2::ONE;
		point.y /= -(f32)DeshWindow->width / DeshWindow->height;

		point *= cameraZoom;
		point += cameraPos;
		return point;
	};

	void DrawGridLines() {

		u32 lines = 30;

		f32 xp = floor(cameraPos.x) + lines;
		f32 xn = floor(cameraPos.x) - lines;
		f32 yp = floor(cameraPos.y) + lines;
		f32 yn = floor(cameraPos.y) - lines;

		for (int i = 0; i < lines * 2 + 1; i++) {
			vec2 v1 = ToScreen(vec2{ xn + i, yn });
			vec2 v2 = ToScreen(vec2{ xn + i, yp });
			vec2 v3 = ToScreen(vec2{ xn, yn + i });
			vec2 v4 = ToScreen(vec2{ xp, yn + i });

			UI::Line(v1, v2, 0.5, Color(255, 255, 255, 100));
			UI::Line(v3, v4, 0.5, Color(255, 255, 255, 100));
		}

		for (f32 i = -5; i <= 5; i++) {
			vec2 posx{ i, 0 };
			vec2 posy{ 0, i };

			UI::Text(TOSTRING(i), ToScreen(posx));
			UI::Text(TOSTRING(i), ToScreen(posy));
		}
	}

	void HandleCamera() {
		
		//zoomin
		if (DeshInput->ScrollDown()) { cameraZoom = Math::clamp(cameraZoom + cameraZoom / 10, 0.01, 10); }
		if (DeshInput->ScrollUp())   { cameraZoom = Math::clamp(cameraZoom - cameraZoom / 10, 0.01, 10); }

		//dragging camera
		static vec2 begin;
		static vec2 og;
		if (DeshInput->LMousePressed()) {
			og = cameraPos;
			begin = DeshInput->mousePos;
		}
		if (DeshInput->LMouseDown()) {
			vec2 mouseWorld = ToWorld(DeshInput->mousePos);
			vec2 beginWorld = ToWorld(begin);

			cameraPos = og + (beginWorld - mouseWorld);
			//UI::Text(TOSTRING(mouseWorld));
			//UI::Text(TOSTRING(beginWorld));

			vec2 size = vec2::ONE * 3;
			Render::FillRectUI(ToScreen(mouseWorld) - size / 2, size);

		}
	}

	void Update() {


		//begin main canvas
		UI::SetNextWindowSize(DeshWindow->dimensions);
		UI::BeginWindow("main_canvas", vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_Invisible);

		if (DeshInput->RMousePressed() || gathering) GatherInput(DeshInput->mousePos);
		
		HandleCamera();
		DrawGridLines();
		
		//draw canvas objs
		for (temp& t : objs) {
			UI::Text(t.input, ToScreen(t.pos), UITextFlags_NoWrap);
		}

		UI::Text(TOSTRING(cameraPos));
		UI::EndWindow();

	}

};


#endif