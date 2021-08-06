#include "deshi/deshi.h"

int main() {
	deshi::init();

	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {

		DeshiImGui::NewFrame();                    //place imgui calls after this
		DengTime->Update();
		DengWindow->Update();
		DengInput->Update();
		DengConsole->Update(); Console2::Update();
		Render::Update();                          //place imgui calls before this


		DengTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}

	deshi::cleanup();
}