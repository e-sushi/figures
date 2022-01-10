/* suugu

current brakus naur idea
this is basically the grammar we expect on input
and also helps determine precedence
which we will turn into our own internal format
only basic arithmetic like +-/* and bitwise stuff for now

<input>         :: = <exp>
<exp>           :: = <bitwise or>
<bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
<bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
<bitwise and>   :: = <equality> { "&" <equality> }
<equality>      :: = <relational> { ("!=" | "==") <relational> }
<relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
<bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
<additive>      :: = <term> { ("+" | "-") <term> }
<term>          :: = <factor> { ("*" | "/" | "%") <factor> }
<factor>        :: = "(" <exp> ")" | <unary> <factor> | <literal>
<unary>         :: = "!" | "~" | "-"

ideas for parsing itself:
- we only parse when a key is input 
- we only indicate errors when we cant make a tree from what we've parsed
  for ex.
  we do not indicate an error on "3 + " because we are expecting the user to input something
  we do indicate an error on "3 + sin()" because the user missed inputting a literal into the fucntion
  or instead of erroring, we put an empty box where we expect something thats missing
  and only error in cases where the user uses invalid syntax such as "3 <<- 5"
  or if they use an undefined identifier, etc.


TODO Board
----------------------------------------------------
- most math should be f64 instead of f32

Parser TODOs
------------
- implement a system for adding to an already existing AST tree

Bug Board       //NOTE mark these with a last-known active date (MM/DD/YY)
---------
*/


//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "defines.h"
#include "core/assets.h"
#include "core/commands.h"
#include "core/console.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/imgui.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "math/math.h"
#include "utils/string.h"
#include "utils/array.h"
#include "utils/map.h"


//// suugu includes ////
#define SUUGU_IMPLEMENTATION
#include "types.h"
#include "canvas.h"
#include "lexer.cpp"
#include "parser.cpp"
#include "solver.cpp"
#include "canvas.cpp"

//#include "utils/deshi_utils_tests.cpp"
//#include "core/deshi_core_tests.cpp"
//#include "utils/misc_testing.cpp"

local Canvas canvas;

int main(){
	//init deshi
	Assets::enforceDirectories();
	memory_init(Gigabytes(1), Gigabytes(4));
	u8* some = (u8*)memalloc(20 * 20 * u8size);

	Logger::Init(5, true);
	DeshConsole->Init();
	DeshTime->Init();
	DeshWindow->Init("deshi", 1280, 720);
	Render::Init();
	//DeshiImGui::Init();
	Storage::Init();
	UI::Init();
	Cmd::Init();
	DeshWindow->ShowWindow();
	Render::UseDefaultViewProjMatrix();
	
	
	
	//init suugu
	canvas.Init();
	
	{//init debug
		//TEST_deshi_utils();
		//TEST_deshi_core();
	}
	array<u32*> random;

	Texture* yep = Storage::CreateTextureFromFile("UV_Grid_Sm.jpg").second;
	
	
	forI(400) {
		some[i] = rand() % 255;
	}

	srand(time(0));

	//start main loop
	TIMER_START(t_f);
	TIMER_START(fun);
	while(!DeshWindow->ShouldClose()){
		//if (TIMER_END(fun) > 1.f / 15.f * 1000) {
			TIMER_RESET(fun);
			DeshWindow->Update();
			DeshTime->Update();
			DeshInput->Update();
			canvas.Update();
			{//update debug
				using namespace UI;
				UI::ShowMetricsWindow();

				//ImGui::ShowDemoWindow();

				Storage::StorageBrowserUI();

				Begin("unitest");
				Text("ok");

				SetNextItemSize(vec2::ONE * 80);
				Image(yep);
				SetNextItemSize(vec2::ONE * 80);

				Image(yep, 1, UIImageFlags_FlipX);
				SetNextItemSize(vec2::ONE * 80);

				Image(yep, 1, UIImageFlags_FlipY);


				if (BeginTabBar("some guy")) {

					if (BeginTab("a tab")) {
						Text("ok hi");
						EndTab();
					}
					if (BeginTab("tab 2")) {
						Text("ok hi, heres some text ok!");
						EndTab();
					}

					EndTabBar();
				}

				if (BeginTabBar("some guy again")) {

					if (BeginTab("Meshes")) {
						Text("aight");
						Separator(7);
						Text("ok");
						EndTab();
					}
					if (BeginTab("Textures")) {
						Text("ok hi, heres some text again ok!");
						EndTab();
					}

					EndTabBar();
				}
				
				End();

			}
			DeshConsole->Update();
			UI::Update();
			Render::Update();


			memory_clear_temp();
			DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
		//}
	}
	
	//cleanup deshi
	Render::Cleanup();
	DeshWindow->Cleanup();
	DeshConsole->Cleanup();
	Logger::Cleanup();
}