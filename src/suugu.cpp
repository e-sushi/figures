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
most math should be f64 instead of f32

Parser TODOs
------------
- implement a system for adding to an already existing AST tree


*/


//// deshi includes ////
#include "defines.h"
#include "deshi.h"
#include "utils/string.h"
#include "utils/array.h"
#include "core/memory.h"
#include "utils/map.h"
#include "math/Math.h"
#include "core/logging.h"

//// STL includes ////


//// suugu includes ////
#define SUUGU_IMPLEMENTATION
#include "types.h"
#include "canvas.h"
#include "lexer.cpp"
#include "parser.cpp"
#include "solver.cpp"
#include "canvas.cpp"

int main() {
	//suugu vars
	Canvas canvas;
	
	//init deshi
	deshi::init();
	Render::UseDefaultViewProjMatrix();
	
	//Memory::Init(Gigabytes(4), Gigabytes(1));
	
	//init suugu
	canvas.Init();
	
	//start main loop
	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {
		DeshiImGui::NewFrame();                    //place imgui calls after this
		Memory::Update();
		DeshTime->Update();
		DeshWindow->Update();
		DeshInput->Update();
		DeshConsole->Update(); Console2::Update();
		//canvas.Update();

		static Font* font = Storage::CreateFontFromFileBDF("gohufont-11.bdf").second;
		
		

		{//debug area
			UI::PushFont(font);
			UI::Begin("testguy", vec2::ONE * 300, vec2::ONE * 300);
			
			const char* opt[] = {
				"opt 1",
				"opt 2",
				"opt 3"
			};
			static u32 select = 0;
			
			//UI::DropDown("dropy", opt, 3, select);
			
			UI::PushLayer(7);
			UI::Text("oh yeah a yest");
			UI::Button("FUCK");
			UI::PopLayer();
			UI::Text("oh yeah a yest");
			UI::Text("oh yeah a yest");
			UI::Text("oh yeah a yest");
			
			//UI::PushLayer(4);
			//UI::Button("layer 4");
			//UI::PushLayer(5);
			//UI::Text("layer 5");
			//UI::PushLayer(6);
			//UI::Text("layer 6");
			//UI::PushLayer(7);
			//UI::Text("layer 7");
			//UI::PushLayer(8);
			//UI::Text("layer 8");
			//UI::PopLayer(5);

			UI::End();
			UI::PopFont();

			Render::DrawTextUI(font, cstr_lit("testin this dude"), vec2::ONE * 300, Color_White, vec2::ONE, 7, vec2::ZERO, DeshWindow->dimensions);
			Render::DrawTextUI(font, cstr_lit("testin this dude again"), vec2(300, 300 + 60), Color_White, vec2::ONE, 6, vec2::ZERO, DeshWindow->dimensions);
			Render::DrawTextUI(font, cstr_lit("testin this dude agh"), vec2(300, 300 + 120), Color_White, vec2::ONE, 6, vec2::ZERO, DeshWindow->dimensions);
			Render::DrawTextUI(font, cstr_lit("testin this dude FUCK"), vec2(300, 300 + 180), Color_White, vec2::ONE, 6, vec2::ZERO, DeshWindow->dimensions);

			
		}
		
		UI::Update();
		Render::Update();                          //place imgui calls before this
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}
	
	
	
	//cleanup deshi
	deshi::cleanup();
}