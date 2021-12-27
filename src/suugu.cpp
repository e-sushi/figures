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

#define DESHI_DISABLE_IMGUI
//// deshi includes ////
#include "defines.h"
#include "deshi.h"
#include "utils/string.h"
#include "utils/array.h"
#include "core/memory.h"
#include "utils/map.h"
#include "math/Math.h"
#include "core/logger.h"


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
	
	Assets::enforceDirectories();
	Memory::Init(Gigabytes(1), Gigabytes(1));
	Logger::Init(5, true);
	DeshConsole->Init();
	DeshTime->Init();
	DeshWindow->Init("deshi", 1280, 720);
	Render::Init();
	Storage::Init();
	UI::Init();
	Cmd::Init();
	
	DeshWindow->ShowWindow();
	
	DeshConsole->AddLog("{{a,c=yellow}this is to test\na formatted newline{}}");
	DeshConsole->AddLog("{{a,c=yellow}this is to test\na formatted newline that is not terminated properly");
	DeshConsole->AddLog("{{a,c=red}some red text{}} {{c=green}some green text");
	DeshConsole->AddLog("some normal text {{c=red}some red text{}}");
	DeshConsole->AddLog("some more text");
	Log("tag1", "a collection of the same tags");
	Log("tag1", "a collection of the same tags");
	Log("tag1", "a collection of the same tags");
	Log("tag1", "a collection of the same tags");
	Log("tag1", "a collection of the same tags");
	Log("tag2", "a collection of the same tags");
	Log("tag3", "a collection of the same tags");
	Log("tag4", "a collection of the same tags");
	Log("tag5", "a collection of the same tags");
	Log("tag5", "a collection of the same tags");
	Log("tag5", "a collection of the same tags");
	
	
	
	Render::UseDefaultViewProjMatrix();
	
	
	//init suugu
	canvas.Init();
	
	//Texture* tex = Storage::CreateTextureFromFile("lcdpix.png").second;
	
	//start main loop
	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {
		Memory::Update();
		DeshTime->Update();
		DeshWindow->Update();
		DeshInput->Update();
		canvas.Update();
		
		{//debug area
			//UI::DemoWindow();
			//UI::ShowMetricsWindow();
		}
		
		DeshConsole->Update();
		UI::Update();
		Render::Update();                          //place imgui calls before this
		Memory::Update();
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}
	
	//cleanup deshi
	deshi::cleanup();
}