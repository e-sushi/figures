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
#define DESHI_DISABLE_IMGUI
#include "defines.h"
#include "core/assets.h"
#include "core/commands.h"
#include "core/console.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
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

#include "utils/deshi_utils_tests.cpp"

local Canvas canvas;

int main(){
	//init deshi
	Assets::enforceDirectories();
	memory_init(Gigabytes(1), Gigabytes(1));
	Logger::Init(5, true);
	DeshConsole->Init();
	DeshTime->Init();
	DeshWindow->Init("deshi", 1280, 720);
	Render::Init();
	Storage::Init();
	UI::Init();
	Cmd::Init();
	DeshWindow->ShowWindow();
	Render::UseDefaultViewProjMatrix();
	
	

	//init suugu
	canvas.Init();
	
	{//init debug
		TEST_deshi_utils_string();
	}
	
	//start main loop
	TIMER_START(t_f);
	while(!DeshWindow->ShouldClose()){
		DeshWindow->Update();
		DeshTime->Update();
		DeshInput->Update();
		canvas.Update();
		{//update debug
			using namespace UI;

			UI::DemoWindow();
			UI::ShowMetricsWindow();
		}
		DeshConsole->Update();
		UI::Update();
		Render::Update();
		memory_clear_temp();
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}
	
	//cleanup deshi
	Render::Cleanup();
	DeshWindow->Cleanup();
	DeshConsole->Cleanup();
	Logger::Cleanup();
}