#include "deshi/deshi.h"

int main() {
	deshi::init();

	Render::UseDefaultViewProjMatrix();


	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {

		DeshiImGui::NewFrame();                    //place imgui calls after this
		DeshTime->Update();
		DeshWindow->Update();
		DeshInput->Update();
		DeshConsole->Update(); Console2::Update();
		Render::Update();                          //place imgui calls before this
		UI::Update();

		static vec2 userPos = vec2::ZERO;
		
		vec2 c1 = vec2(0.5, 0);
		vec2 c2 = vec2(0, 0.5);

		Render::DrawTextUI("testing this wow", DeshWindow->dimensions / 2);

		UI::BeginWindow("test", vec2(100, 100), vec2(100, 100));

		static string buffer = "";

		UI::Text("wow");
		UI::InputText("label", buffer);

		UI::EndWindow();

		//UI::ShowDebugWindowOf("test");

		UI::SetNextWindowSize(DeshWindow->dimensions);
		UI::BeginWindow("main_canvas", vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_Invisible);
		
		

		//world to camera
		c1 += userPos; c2 += userPos;

		//camera to world
		c1 += vec2::ONE; c2 += vec2::ONE;
		c1 *= DeshWindow->dimensions / 2;
		c2 *= DeshWindow->dimensions / 2;

		UI::RectFilled(c1, vec2(2, 2));
		UI::RectFilled(c2, vec2(2, 2));

		//dragging camera
		static vec2 begin;
		static vec2 og;
		if (DeshInput->KeyPressedAnyMod(MouseButton::LEFT)) {
			og = userPos;
			begin = DeshInput->mousePos;
		}
		if (DeshInput->KeyDownAnyMod(MouseButton::LEFT)) {
			userPos = og + (DeshInput->mousePos - begin) / DeshWindow->dimensions;
		}

		UI::Text(TOSTRING(userPos));


		//u32 lines = 1000;
		//
		//f32 spacingx = DeshWindow->width / lines;
		//f32 spacingy = spacingx;
		//
		//f32 xp = floor(userPos.x - DeshWindow->width / 2) + lines;
		//f32 xn = floor(userPos.x - DeshWindow->width / 2) - lines;
		//f32 yp = floor(userPos.y - DeshWindow->height / 2) + lines;
		//f32 yn = floor(userPos.y - DeshWindow->height / 2) - lines;
		//
		//for (int i = 0; i < lines * 2 + 1; i++) {
		//	vec2 v1 = vec2{ xn + i * spacingx, yn };
		//	vec2 v2 = vec2{ xn + i * spacingx, yp };
		//	vec2 v3 = vec2{ xn, yn + i * spacingy };
		//	vec2 v4 = vec2{ xp, yn + i * spacingy };
		//
		//	UI::Line(v1, v2);
		//	UI::Line(v3, v4);
		//}

		UI::EndWindow();

		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}

	deshi::cleanup();
}