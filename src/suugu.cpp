#include "deshi.h"

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
		static float userZoom = 5;
		
		if (DeshInput->ScrollDown()) { userZoom = Math::clamp(userZoom + userZoom / 10, 0.01, 10); }
		if (DeshInput->ScrollUp())   { userZoom = Math::clamp(userZoom - userZoom / 10, 0.01, 10); }

		UI::SetNextWindowSize(DeshWindow->dimensions);
		UI::BeginWindow("main_canvas", vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_Invisible);

		auto toScreen = [&](vec2 point) {
			point -= userPos;
			point /= userZoom;

			point.y *= -(f32)DeshWindow->width / DeshWindow->height;
			point += vec2(1, 1);
			point *= DeshWindow->dimensions / 2;

			return point;
		};

		auto toWorld = [&](vec2 point) {
			point /= DeshWindow->dimensions;
			point *= 2;
			point -= vec2::ONE;
			point.y /= -(f32)DeshWindow->width / DeshWindow->height;

			point *= userZoom;
			point += userPos;
			return point;
		};

		//dragging camera
		static vec2 begin;
		static vec2 og;
		if (DeshInput->KeyPressedAnyMod(MouseButton::LEFT)) {
			
			
			og = userPos;

			//og = ((DeshInput->mousePos / DeshWindow->dimensions) - vec2::ONE) * userZoom - userPos;
			begin = DeshInput->mousePos;

		}
		if (DeshInput->KeyDownAnyMod(MouseButton::LEFT)) {
			vec2 mouseWorld = toWorld(DeshInput->mousePos);//((DeshInput->mousePos / DeshWindow->dimensions * 2) - vec2::ONE) * userZoom - userPos;
			vec2 beginWorld = toWorld(begin);//((begin / DeshWindow->dimensions * 2) - vec2::ONE) * userZoom - userPos;
			
			userPos = og + (beginWorld - mouseWorld);
			UI::Text(TOSTRING(mouseWorld));
			UI::Text(TOSTRING(beginWorld));

			vec2 size = vec2::ONE * 3;
			Render::FillRectUI(toScreen(mouseWorld) - size / 2, size);

		}

		UI::Text(TOSTRING(userPos));

		u32 lines = 30;
		
		f32 xp = floor(userPos.x) + lines;
		f32 xn = floor(userPos.x) - lines;
		f32 yp = floor(userPos.y) + lines;
		f32 yn = floor(userPos.y) - lines;
		
		for (int i = 0; i < lines * 2 + 1; i++) {
			vec2 v1 = toScreen(vec2{ xn + i, yn });
			vec2 v2 = toScreen(vec2{ xn + i, yp });
			vec2 v3 = toScreen(vec2{ xn, yn + i});
			vec2 v4 = toScreen(vec2{ xp, yn + i});
			


			UI::Line(v1, v2, 0.5, Color(255, 255, 255, 100));
			UI::Line(v3, v4, 0.5, Color(255, 255, 255, 100));
		}

		for (f32 i = -5; i <= 5; i++) {
			vec2 posx{ i, 0 };
			vec2 posy{ 0, i };

			UI::Text(TOSTRING(i), toScreen(posx));
			UI::Text(TOSTRING(i), toScreen(posy));
		}

		UI::EndWindow();

		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}

	deshi::cleanup();
}