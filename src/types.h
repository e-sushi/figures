/* Index:
@vec2f64
@element
@term
@expression
*/

#pragma once
#ifndef SUUGU_TYPES_H
#define SUUGU_TYPES_H

#include "kigu/common.h"
#include "core/memory.h"
#include "external/stb/stb_ds.h"

struct Expression;


////workspace: region of the canvas in which expressions are able to interact together  
//struct Expression;
//struct Workspace{
//Element element;
//str8 name;
//arrayT<Expression*> expressions = arrayT<Expression*>(deshi_allocator);
//};
//#define ElementToWorkspace(elem_ptr) ((Workspace*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(Workspace, element))))

////graph: graphing grid with a local camera in which equations can be drawn
// struct GraphElement{
// 	Element element;
// 	uiGraphCartesian* cartesian_graph;
// };
//TODO(sushi) remove this and usage of it since we can just use normal C casting
#define ElementToGraphElement(elem_ptr) ((GraphElement*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(GraphElement, element))))

//struct TextElement{
//Element element;
//str8 text;
//Font* font;
//f32 font_height;
//vec2 scale;
//f32 rotation;
//};
//#define ElementToTextElement(elem_ptr) ((TextElement*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(TextElement, element))))

/////////////////
//// @pencil ////
/////////////////
struct PencilStroke{
	f32   size;
	color color;
	vec2* pencil_points; // kigu array
};

////////////////
//// @tools ////
////////////////
enum CanvasTool : u32{
	CanvasTool_Navigation,
	CanvasTool_Context,
	CanvasTool_Expression,
	CanvasTool_Pencil,
};
local const char* canvas_tool_strings[] = {
	"Navigation", "Context", "Expression", "Pencil",
};

////////////////
//// @binds ////
////////////////
enum CanvasBind_{ //TODO ideally support multiple keybinds per action
	//[GLOBAL] SetTool
	CanvasBind_SetTool_Navigation = Key_ESCAPE  | InputMod_Any,
	CanvasBind_SetTool_Context    = Mouse_RIGHT | InputMod_AnyCtrl,
	CanvasBind_SetTool_Expression = Key_E       | InputMod_AnyCtrl, //NOTE temp making this CTRL+E for simplicity
	CanvasBind_SetTool_Pencil     = Key_P       | InputMod_AnyCtrl,
	CanvasBind_SetTool_Graph      = Key_G       | InputMod_AnyCtrl,
	CanvasBind_SetTool_Previous   = Mouse_4     | InputMod_None,
	
	//[GLOBAL] Camera 
	CanvasBind_Camera_Pan = Mouse_MIDDLE | InputMod_None,
	
	//[LOCAL]  Navigation 
	CanvasBind_Navigation_Pan       = Mouse_LEFT  | InputMod_Any,
	CanvasBind_Navigation_ResetPos  = Key_NP0     | InputMod_None,
	CanvasBind_Navigation_ResetZoom = Key_NP0     | InputMod_None,
	
	//[LOCAL]  Expression
	CanvasBind_Expression_Select                = Mouse_LEFT    | InputMod_None,
	CanvasBind_Expression_Create                = Mouse_RIGHT   | InputMod_None,
	CanvasBind_Expression_CursorLeft            = Key_LEFT      | InputMod_None,
	CanvasBind_Expression_CursorRight           = Key_RIGHT     | InputMod_None,
	CanvasBind_Expression_CursorWordLeft        = Key_LEFT      | InputMod_AnyCtrl,
	CanvasBind_Expression_CursorWordRight       = Key_RIGHT     | InputMod_AnyCtrl,
	CanvasBind_Expression_CursorUp              = Key_UP        | InputMod_None,
	CanvasBind_Expression_CursorDown            = Key_DOWN      | InputMod_None,
	CanvasBind_Expression_CursorHome            = Key_HOME      | InputMod_None,
	CanvasBind_Expression_CursorEnd             = Key_END       | InputMod_None,
	CanvasBind_Expression_CursorDeleteLeft      = Key_BACKSPACE | InputMod_None,
	CanvasBind_Expression_CursorDeleteRight     = Key_DELETE    | InputMod_None,
	CanvasBind_Expression_CursorDeleteWordLeft  = Key_BACKSPACE | InputMod_AnyCtrl,
	CanvasBind_Expression_CursorDeleteWordRight = Key_DELETE    | InputMod_AnyCtrl,
	
	//[LOCAL]  Pencil
	CanvasBind_Pencil_Stroke             = Mouse_LEFT       | InputMod_Any,
	CanvasBind_Pencil_DeletePrevious     = Key_Z            | InputMod_AnyCtrl,
	CanvasBind_Pencil_DetailIncrementBy1 = Key_EQUALS       | InputMod_None,
	CanvasBind_Pencil_DetailIncrementBy5 = Key_EQUALS       | InputMod_AnyShift,
	CanvasBind_Pencil_DetailDecrementBy1 = Key_MINUS        | InputMod_None,
	CanvasBind_Pencil_DetailDecrementBy5 = Key_MINUS        | InputMod_AnyShift,
}; typedef KeyCode CanvasBind;




//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @term
enum TermType : u32 { //TermType_{
	TermType_Expression,
	TermType_Operator,
	TermType_Literal,
	TermType_Variable,
	TermType_FunctionCall,
	TermType_Logarithm,
	TermType_FunctionArg,	
}; //typedef Type TermType;

enum TermFlags_{
	TermFlag_NONE = 0,
	
	//// operator argument flags //// //NOTE these flags are mainly used to determine empty slots on operators
	//TODO(delle) explain these better, i forgot what they do and why, seems to be mostly for rendering?
	TermFlag_OpArgLeft   = (1 << 0),
	TermFlag_OpArgRight  = (1 << 1),
	TermFlag_OpArgTop    = (1 << 2),
	TermFlag_OpArgBottom = (1 << 3),
	
	TermFlag_LeftParenHasMatchingRightParen = (1 << 4),
	TermFlag_DanglingClosingParenToRight    = (1 << 5),
	//TermFlag_DrawParenthesesAsBlock         = (1 << 6), //TODO symbolab style input boxes (test with parentheses and exponents)
}; typedef Flags TermFlags;
#define OPARG_MASK (TermFlag_OpArgLeft | TermFlag_OpArgRight | TermFlag_OpArgTop | TermFlag_OpArgBottom)
#define RemoveOpArgs(var) RemoveFlag(var, OPARG_MASK)
#define ReplaceOpArgs(var, new_flags) ((var) = (((var) & ~OPARG_MASK) | new_flags))

//operator: symbol that represents an operation on one or many terms
//NOTE in order of precedence, so the higher value it is (lower sequentially), the lower the precedence
//NOTE these are operation-based operators, not token-based operators
//TODO try to find a way to store the number of arguments
enum OpType : u32{//temp c++ enum thing for debugging
	OpType_NULL = 0,
	
	OpPrecedence_1  = (1 << 8),
	OpType_Parentheses, //TODO maybe just make this a term type since it doesnt behave like an operator?
	//OpType_AbsoluteValue,
	//OpType_Root,
	//OpType_Derivative,
	//OpType_Integral,
	//OpType_Limit,
	//OpType_Sum,
	//OpType_PartialDerivative
	
	OpPrecedence_2  = (1 << 9),
	OpType_Exponential,
	
	OpPrecedence_3  = (1 << 10),
	OpType_Negation,
	//OpType_BitwiseNOT,
	//OpType_LogicalNOT,
	
	OpPrecedence_4  = (1 << 11),
	OpType_ImplicitMultiplication, //5x
	OpType_ExplicitMultiplication, //5*x
	OpType_Division,
	OpType_Modulo,
	
	OpPrecedence_5  = (1 << 12),
	OpType_Addition,
	OpType_Subtraction,
	
	OpPrecedence_6  = (1 << 13),
	//OpType_ArithmaticShiftLeft,
	//OpType_ArithmaticShiftRight,
	//OpType_LogicalShiftLeft,
	//OpType_LogicalShiftRight,
	//OpType_CircularShiftLeft,
	//OpType_CircularShiftRight,
	
	OpPrecedence_7  = (1 << 14),
	//OpType_LessThan,
	//OpType_LessThanEqual,
	//OpType_GreaterThan,
	//OpType_GreaterThanEqual,
	
	OpPrecedence_8  = (1 << 15),
	//OpType_BooleanEquals,
	//OpType_BooleanNotEquals,
	
	OpPrecedence_9  = (1 << 16),
	//OpType_BitwiseAND,
	
	OpPrecedence_10 = (1 << 17),
	//OpType_BitwiseXOR,
	
	OpPrecedence_11 = (1 << 18),
	//OpType_BitwiseOR,
	
	OpPrecedence_12 = (1 << 19),
	//OpType_LogicalAND,
	
	OpPrecedence_13 = (1 << 20),
	//OpType_LogicalOR,
	
	OpPrecedence_14 = (1 << 21),
	OpType_ExpressionEquals, //NOTE this should be one of the lowest precedence operators
}; //typedef Type OpType;
#define OPPRECEDENCE_MASK 0xFFFFFF00
//NOTE these are inverted b/c the precedence flags get larger as they decrease in precedence
#define OpIsGreater(op1,op2)      (((op1)->op_type & OPPRECEDENCE_MASK) <  ((op2)->op_type & OPPRECEDENCE_MASK))
#define OpIsGreaterEqual(op1,op2) (((op1)->op_type & OPPRECEDENCE_MASK) <= ((op2)->op_type & OPPRECEDENCE_MASK))
#define OpIsLesser(op1,op2)       (((op1)->op_type & OPPRECEDENCE_MASK) >  ((op2)->op_type & OPPRECEDENCE_MASK))
#define OpIsLesserEqual(op1,op2)  (((op1)->op_type & OPPRECEDENCE_MASK) >= ((op2)->op_type & OPPRECEDENCE_MASK))

static str8 OpTypeStrs(u32 type){
	switch(type){
		case OpType_NULL:                   return str8l("OpType_NULL");
		case OpPrecedence_1:                return str8l("OpPrecedence_1");
		case OpType_Parentheses:            return str8l("OpType_Parentheses");
		case OpPrecedence_2:                return str8l("OpPrecedence_2");
		case OpType_Exponential:            return str8l("OpType_Exponential");
		case OpPrecedence_3:                return str8l("OpPrecedence_3");
		case OpType_Negation:               return str8l("OpType_Negation");
		case OpPrecedence_4:                return str8l("OpPrecedence_4");
		case OpType_ImplicitMultiplication: return str8l("OpType_ImplicitMultiplication");
		case OpType_ExplicitMultiplication: return str8l("OpType_ExplicitMultiplication");
		case OpType_Division:               return str8l("OpType_Division");
		case OpType_Modulo:                 return str8l("OpType_Modulo");
		case OpPrecedence_5:                return str8l("OpPrecedence_5");
		case OpType_Addition:               return str8l("OpType_Addition");
		case OpType_Subtraction:            return str8l("OpType_Subtraction");
		case OpPrecedence_6:                return str8l("OpPrecedence_6");
		case OpPrecedence_7:                return str8l("OpPrecedence_7");
		case OpPrecedence_8:                return str8l("OpPrecedence_8");
		case OpPrecedence_9:                return str8l("OpPrecedence_9");
		case OpPrecedence_10:               return str8l("OpPrecedence_10");
		case OpPrecedence_11:               return str8l("OpPrecedence_11");
		case OpPrecedence_12:               return str8l("OpPrecedence_12");
		case OpPrecedence_13:               return str8l("OpPrecedence_13");
		case OpPrecedence_14:               return str8l("OpPrecedence_14");
		default:                            return str8l("invalid term type");
	}
}

struct MathObject;
struct Part;

//term: generic base thing (literal, operator, variable, function call, etc)
struct Term{
	TermType  type;
	TermFlags flags;
	Text raw;
	MathObject* mathobj; // the MathObject containing information about the type of this Term
	Part* part; // the part of the MathObject this Term represents

	struct{
		Term* left, *right, *up, *down;
	}movement;

	// wasteful, we should probably just use the information on the MathObject for this
	struct{
		Term* left, *right, *up, *down;
	}movement_in;

	//syntax tree
	Term* prev;
	Term* next;
	Term* parent;
	Term* first_child;
	Term* last_child;
	u32   child_count;
	
	//linear
	Term* left;
	Term* right;
	Term* outside;
	Term* first_inside;
	Term* last_inside;
};

global void ast_insert_after(Term* target, Term* term){
	if(target->next) target->next->prev = term;
	term->next = target->next;
	term->prev = target;
	target->next = term;
}

global void ast_insert_before(Term* target, Term* term){
	if(target->prev) target->prev->next = term;
	term->prev = target->prev;
	term->next = target;
	target->prev = term;
}

global void ast_remove_horizontally(Term* term){
	if(term->next) term->next->prev = term->prev;
	if(term->prev) term->prev->next = term->next;
	term->next = term->prev = 0;
}

global void ast_insert_last(Term* parent, Term* child){
	child->parent = parent;
	if(parent->first_child){
		ast_insert_after(parent->last_child, child);
		parent->last_child = child;
	}else{
		parent->first_child = child;
		parent->last_child  = child;
	}
	parent->child_count++;
}

global void ast_insert_first(Term* parent, Term* child){
	child->parent = parent;
	if(parent->first_child){
		ast_insert_before(parent->first_child, child);
		parent->first_child = child;
	}else{
		parent->first_child = child;
		parent->last_child  = child;
	}
	parent->child_count++;
}

global void ast_remove_from_parent(Term* term){
	if(term->parent == 0) return;
	if(term->parent->child_count > 1){
		if(term == term->parent->first_child) term->parent->first_child = term->next;
		if(term == term->parent->last_child)  term->parent->last_child  = term->prev;
	}else{
		Assert(term == term->parent->first_child && term == term->parent->last_child, "if term is the only child term, it should be both the first and last child terms");
		term->parent->first_child = 0;
		term->parent->last_child  = 0;
	}
	term->parent->child_count--;
}

global void ast_change_parent_insert_last(Term* new_parent, Term* term){
	if(new_parent == term->parent) return;
	ast_remove_from_parent(term);
	ast_remove_horizontally(term);
	ast_insert_last(new_parent, term);
}

global void ast_change_parent_insert_first(Term* new_parent, Term* term){
	if(new_parent == term->parent) return;
	ast_remove_from_parent(term);
	ast_remove_horizontally(term);
	ast_insert_first(new_parent, term);
}

global void linear_insert_left(Term* target, Term* term){
	if(target->left) target->left->right = term;
	term->right = target;
	term->left  = target->left;
	target->left = term;
	if(target->outside){
		term->outside = target->outside;
		if(target == target->outside->first_inside){
			target->outside->first_inside = term;
		}
	}
}

global void linear_insert_right(Term* target, Term* term){
	if(target->right) target->right->left = term;
	term->left  = target;
	term->right = target->right;
	target->right = term;
	if(target->outside){
		term->outside = target->outside;
		if(target == target->outside->last_inside){
			target->outside->last_inside = term;
		}
	}
}

global void linear_remove(Term* term){
	if(term->right) term->right->left = term->left;
	if(term->left)  term->left->right = term->right;
	if(term->outside){
		if(term == term->outside->first_inside) term->outside->first_inside = term->right;
		if(term == term->outside->last_inside) term->outside->last_inside = term->left;
	}
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @expression
//expression: collection of terms in the form of a syntax tree
struct Expression{
	Node node;
	//Workspace* workspace;
	
	dstr8 raw;
	
	b32 changed;
	Term root;
	Term* equals;
	Term* rightmost;
	
	u32 raw_cursor_start;
	Term* term_cursor_start;
	b32 right_paren_cursor;
	
	b32 valid;
	u32 unknown_vars;
	f64 solution;
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @element
enum CoordinateSpace : u32{
	//CoordinateSpace_World,
	//CoordinateSpace_Screen,
	//CoordinateSpace_Workspace, //TODO maybe add local to workspace
};

enum ElementType : u32{
	ElementType_NULL,
	ElementType_Expression,
	//ElementType_Workspace,
	ElementType_Graph,
	//ElementType_Text,
};

// represents data pertaining to a discrete visual element on screen
struct Visual {

};

struct TermPos {
	Term* term;
	f32 pos;
};

enum {
	RenderPart_Individual,
	RenderPart_Group,
};

struct RenderPart{
	Type       type;
	u32 group_children; // number of children parts a group has
	vec2   position; // the position of this part
	vec2        bbx; // the bounding box formed by child nodes
	f32     midline; // 
	s32      vstart; // index into vertex arena 
	s32      istart; // index into index arena
	s32 vcount, icount;
	Term* term; // the term that was rendered
	b32 cursor_ignore; // the cursor will not consider this RenderPart when moving
};

// element: anything with position, size, coordinate space, and display info
struct Element{
	union{struct{f32 x,y,z;};
		vec3 pos;
	};
	union{struct{f32 width,height,depth;};
		vec3 size;
	};
	//CoordinateSpace space;
	ElementType type;
	uiItem* item; // handle to the uiItem representing this Element

	union{
		struct {
			Expression handle;
			RenderPart* rendered_parts;
			// sorted lists of Terms over each axes so that 
			// we may move the cursor between them
			struct{
				TermPos* x;
				TermPos* y;
			}position_map; // TODO(sushi) better name, this isn't really a map
		}expression;
		
		uiGraphCartesian cartesian_graph;
	};
};

// returns the index in the x and y arrays where the term belongs based on the given position
pair<spt,spt> position_map_find(Element* element, vec2 pos) {
	spt x_idx; 
	{
		spt index = -1, middle = -1;
		spt left = 0;
		spt right = array_count(element->expression.position_map.x)-1;
		while(left <= right){
			middle = left+((right-left)/2);
			if(element->expression.position_map.x[middle].pos < pos.x){
				left = middle+1;
				middle = left+((right-left)/2);
			}else{
				right = middle-1;
			}
		}
		x_idx = middle;
	}
	spt y_idx;
	{
		spt index = -1, middle = -1;
		spt left = 0;
		spt right = array_count(element->expression.position_map.y)-1;
		while(left <= right){
			middle = left+((right-left)/2);
			if(element->expression.position_map.y[middle].pos < pos.y){
				left = middle+1;
				middle = left+((right-left)/2);
			}else{
				right = middle-1;
			}
		}
		y_idx = middle;
	}

	return {x_idx, y_idx};
}

void position_map_insert(Element* element, Term* term, vec2 pos) {
	Assert(element->type == ElementType_Expression);
	auto [x,y] = position_map_find(element, pos);
	if(x == -1) x = 0;
	if(y == -1) y = 0;
	TermPos xpos = {term, pos.x};
	TermPos ypos = {term, pos.y};
	*array_insert(element->expression.position_map.x, x) = xpos;
	element->expression.position_map.y = kigu__array_insert_wrapper((element->expression.position_map.y), sizeof(*(element->expression.position_map.y)), (y));
	*((element->expression.position_map.y)+(y)) = ypos;
}

// NOTE(sushi) this compiles on clang, need to know if it compiles on MSVC as well
/*
const uiStyle element_default_style = {
	.positioning = pos_relative,
	.sizing = size_auto,
	.background_color = Color_Black,
	.border_style = border_solid,
	.border_width = 1,
	.text_wrap = text_wrap_none,
};
*/
const uiStyle element_default_style = {
	/*positioning*/ pos_relative,
	/*anchor*/ anchor_top_left,
	/*sizing*/ 0,
	/*pos*/ {0,0},
	/*size*/ {0,0},
	/*min_size*/ {0,0},
	/*max_size*/ {0,0},
	/*margin*/ {0,0,0,0},
	/*padding*/ {0,0,0,0},
	/*scale*/ {0,0},
	/*scroll*/ {0,0},
	/*background_color*/ Color_Black,
	/*background_image*/ 0,
	/*border_style*/ border_solid,
	/*border_color*/ Color_White,
	/*border_width*/ 1,
	/*font*/ 0,
	/*font_height*/ 0,
	/*text_wrap*/ text_wrap_none,
	/*text_color*/ Color_White,
	/*tab_spaces*/ 0,
	/*focus*/ 0,
	/*display*/ display_vertical,
	/*overflow*/ overflow_scroll,
	/*content_align*/ {0,0},
	/*hover_passthrough*/ false,
};


//~////////////////////////////////////////////////////////////////////////////////////////////////
// @Display


enum {
	Token_EOF,
	Token_align,
	Token_render,
	Token_text,
	Token_left,
	Token_right,
	Token_top,
	Token_bottom,
	Token_center,
	Token_center_x,
	Token_center_y,
	Token_origin,
	Token_origin_x,
	Token_origin_y,
	Token_child,
	Token_term_raw,
	Token_avg,
	Token_max,
	Token_min,
	Token_shape,
	Token_line,

	Token_Integer,
	Token_Backtick,
	Token_String,
	Token_OpenParen,
	Token_CloseParen,
	Token_Comma,
};

struct Token {
	Type type;
	str8 raw;
	u64 hash;
	u64 line, column;
};


Token* tokenize_instructions(str8 instructions) {
#define str8case(str) case str8_static_hash64(str8_static_t(str))
#define charcase(c, type_)       \
case c: {                        \
    Token* t = array_push(out);  \
    t->line = line;              \
    t->column = column;          \
    t->raw = str8{stream.str, 1};\
    t->type = type_;             \
    advance_stream();            \
    eat_whitespace();            \
}break;

	Token* out;
	array_init(out, 1, deshi_allocator);

	u64 line = 1, column = 1;
	str8 stream = instructions;

	auto advance_stream = [&]() {
		if(*stream.str == '\n') {
			column = 1;
			line++;
		}else{
			column++;
		}
		str8_advance(&stream);
	};

	auto eat_whitespace = [&]() {
		while(stream) {
			if(!is_whitespace(utf8codepoint(stream.str))) {
				break;
			}
			if(*stream.str == '\n') {
				column = 1;
				line++;
			}else{
				column++;
			}
			str8_advance(&stream);
		}
	};

	auto eat_word = [&]() {
		str8 out = stream;
		while(stream) {
			if(is_whitespace(utf8codepoint(stream.str))) {
				break;
			}
			column++;
			str8_advance(&stream);
		}
		out.count = stream.str - out.str;
		return out;
	};

	eat_whitespace();
	while(stream) {
		switch(*stream.str) {
			charcase('`', Token_Backtick);
			charcase(',', Token_Comma);
			charcase('(', Token_OpenParen);
			charcase(')', Token_CloseParen);
			case '\'': {
				advance_stream();
				str8 start = stream;
				u64 save_line = line, save_column = column;
				while(stream && *stream.str != '\'') {
					str8_advance(&stream);
				}
				Token* t = array_push(out);
				t->raw = {start.str, stream.str-start.str};
				t->type = Token_String;
				t->line = save_line;
				t->column = save_column;
				advance_stream();
			}break;
			default: {
				str8 start = stream;
				b32 is_word = true;
				if(isdigit(*start.str)) {
					is_word = false;
					while(isdigit(*stream.str)) advance_stream();
				} else while(stream) {
					if(is_whitespace(*stream.str) || match_any(*stream.str, '`', '\'', ',', '(', ')')) {
						break;
					}
					advance_stream();
				}
				Token* t = array_push(out);
				t->raw = {start.str, stream.str-start.str};
				t->line = line;
				t->column = column;
				if(!is_word) {
					t->type = Token_Integer;
				}else{
					u64 hash = str8_hash64(t->raw);
					switch(hash) {
						str8case("align"): t->type = Token_align; break;
						str8case("render"): t->type = Token_render; break;
						str8case("text"): t->type = Token_text; break;
						str8case("left"): t->type = Token_left; break;
						str8case("right"): t->type = Token_right; break;
						str8case("top"): t->type = Token_top; break;
						str8case("bottom"): t->type = Token_bottom; break;
						str8case("center_x"): t->type = Token_center_x; break;
						str8case("center_y"): t->type = Token_center_y; break;
						str8case("origin_x"): t->type = Token_origin_x; break;
						str8case("origin_y"): t->type = Token_origin_y; break;
						str8case("child"): t->type = Token_child; break;
						str8case("term_raw"): t->type = Token_term_raw; break;
						str8case("avg"): t->type = Token_avg; break;
						str8case("max"): t->type = Token_max; break;
						str8case("min"): t->type = Token_min; break;
						str8case("shape"): t->type = Token_shape; break;
						str8case("line"): t->type = Token_line; break;
						default: {
							LogE("instrcomp", "unknown word '", t->raw, "' on line ", t->line, " column ", t->column);
							return 0;
						}break;
					}
				}
			}break;
		}
		eat_whitespace();
	}
	Token* eof = array_push(out);
	eof->type = Token_EOF;
	return out;
#undef str8case
}

// type containing data needed for displaying math in some way
struct Display {
	TNode node;
	Term* obj; // the Term this display represents
	MathObject* mathobj; // the MathObject that this Display belongs to

	str8 text; // data used when displaying as text
	str8 s_expression; //  data used when displaying as an s-expression
	Token* instruction_tokens; // kigu array of instructions used when rendering

};

void compile_display(Term* term) {
	MathObject* curmo = term->mathobj;

}

//~////////////////////////////////////////////////////////////////////////////////////////////////
// @MathObject

// represents a unit, which may or may not be composed of other units
struct Unit{
	Unit* unit; //array of units that this unit may be made of, 0 if base unit
	str8 id; 	   // the unique identifier of this unit 
	str8 quantity; // the physical quantity this unit represents such as length, mass, etc.
	str8 symbols;  // symbols this unit may use
	str8 description;
};

struct Variable{
	Expression* expr; //TODO maybe let variable terms have expression children rather than a tree disconnect
	str8 name;
	Unit* unit;
	str8* symbols; // a list of symbols that this variable may take on. if a symbol conflicts with another, we will try to use a different one to avoid conflicts
	b32 right_of_equals;
};

struct Function{
	s32 arity; // number of arguments 
};

struct Number {
	// TODO(sushi) replace this with mint and a built in fixed point type
	f64 value; 
};

enum{
	MathObject_Placeholder, // builtin
	MathObject_Number, // builtin
	//MathObject_Array, // builtin
	MathObject_Function,
	MathObject_Constant,
	MathObject_Unit,
};

const str8 MathObjectTypeStrings[] = {
	str8l("Placeholder"),
	str8l("Number"),
	str8l("Function"),
	str8l("Constant"),
	str8l("Unit"),
};

struct Movement {
	s32 left, right, up, down;
};

typedef Movement* MovementArray;

// Base object of all mathematical things in suugu, such as operators,
// functions, constants, units, etc. This contains information such as its name and
// data regarding how to display it.
struct MathObject {
	str8 name;
	u64  hash;
	str8 description;
	Type type;
	Display display; // how to display this MathObject in various ways.
	// a movement for each part that determines how the cursor should move between them
	MovementArray movements;

	// TODO(sushi) this is pretty limited, for instance, in a case like 
	//                 (1+2)/(3+4+5) 
	//             when the cursor moves up from '5', it should to the end of the numerator
	//             and up from '3' should go to the beginning.
	//             we should probably add some kind of rules that determine how the cursor should seek where it should
	//             go visually
	// which part should the cursor go to when the MathObject is moved into from some direction
	Movement movement_in;

	union{
		Function func;
		Number number;
	};
};

struct{ // these are made in the compiler for now
	MathObject placeholder;
	MathObject number;

	// arithmetic
	MathObject addition;
	MathObject subtraction;
	MathObject multiplication;
	MathObject division;
}math_objects;

// TODO(sushi) set this up when we are able to do event driven input
// struct MathObjectKey{
//     KeyCode key;
//     MathObject* mathobj;
// };

// typedef MathObjectKey* KeyTable; 
// global KeyTable key_table;

// pair<spt, b32> key_table_find()


// MathObjects can be referred to by multiple names
// struct MathObjectTableEntry{
// 	str8 name;
// 	u64  hash;
// 	MathObject* mathobj;
// };

// typedef MathObjectTableEntry* MathObjectTable;
// global MathObjectTable math_objects;

// pair<spt,b32> mathobj_table_find(MathObjectTable* table, u64 key){
//     spt index = -1, middle = -1;
//     spt left = 0;
//     spt right = array_count(*table)-1;
//     while(left <= right){
//         middle = left+((right-left)/2);
//         if((*table)[middle].hash == key){
//             index = middle;
//             break;
//         }
//         if((*table)[middle].hash < key){
//             left = middle+1;
//             middle = left+((right-left)/2);
//         }else{
//             right = middle-1;
//         }
//     }
//     return {middle, index != -1};
// }

// pair<MathObjectTableEntry*, b32> mathobj_table_add(MathObjectTable* table, str8 name) {
//     u64 hash = str8_hash64(name);
    
//     auto [idx,found] = mathobj_table_find(table, hash);
//     if(found) return {&(*table)[idx], 1};
//     if(idx == -1) idx = 0; // -1 returned on empty array, so need to say we're inserting at 0
    
//     MathObjectTableEntry* s = array_insert(*table, idx);
// 	s->name = name;
// 	s->hash = hash;
//     return {s, 0};
// }

// b32 mathobj_table_remove(MathObjectTable* table, str8 name) {
//     u64 hash = str8_hash64(name);

//     auto [idx,found] = mathobj_table_find(table, hash);
//     if(!found) return 0;

//     array_remove_ordered(*table, idx);
//     return 1;
// }




//~////////////////////////////////////////////////////////////////////////////////////////////////
// @memory
// TODO(sushi) chunked arenas (pools?) so that we can grow
struct{
	Arena* elements;
	Node inactive_elements;
	Arena* terms;
	Node inactive_terms;
	Arena* math_objects;
	Node inactive_math_objects;
}arenas;

void suugu_memory_init() {
	arenas.elements = memory_create_arena(500*sizeof(Element));
	arenas.terms = memory_create_arena(5000*sizeof(Term));
	arenas.math_objects = memory_create_arena(5000*sizeof(MathObject));
	arenas.inactive_elements.next = &arenas.inactive_elements;
	arenas.inactive_elements.prev = &arenas.inactive_elements;
	arenas.inactive_terms.next = &arenas.inactive_terms;
	arenas.inactive_terms.prev = &arenas.inactive_terms;
	arenas.inactive_math_objects.next = &arenas.inactive_math_objects;
	arenas.inactive_math_objects.prev = &arenas.inactive_math_objects; 
}

global Element* 
create_element(){
	Element* result;
	if(arenas.inactive_elements.next != &arenas.inactive_elements){
		result = (Element*)arenas.inactive_elements.next;
		NodeRemove(arenas.inactive_elements.next);
		ZeroMemory(result, sizeof(Element));
	}else{
		if(arenas.elements->used + sizeof(Element) > arenas.elements->size) {
			LogE("suugu_memory", "ran out of memory when allocating an Element, allocated size is ", arenas.elements->size/bytesDivisor(arenas.elements->size), " ", bytesUnit(arenas.elements->size));
			return 0;
		}
		result = (Element*)arenas.elements->cursor;
		arenas.elements->cursor += sizeof(Element);
		arenas.elements->used += sizeof(Element);
	}
	return result;
}

global void
delete_element(Element* element) {
	NotImplemented; // TODO(sushi) when we need it 
}

global Term* 
create_term(){
	Term* result;
	if(arenas.inactive_terms.next != &arenas.inactive_terms){
		result = (Term*)(arenas.inactive_terms.next+sizeof(Node));
		NodeRemove(arenas.inactive_terms.next);
		ZeroMemory(result, sizeof(Term));
	}else{
		if(arenas.terms->used + sizeof(Term)+sizeof(Node) > arenas.terms->size) {
			LogE("suugu_memory", "ran out of memory when allocating a Term, allocated size is ", arenas.terms->size/bytesDivisor(arenas.terms->size), " ", bytesUnit(arenas.terms->size));
			return 0;
		}
		result = (Term*)arenas.terms->cursor;
		arenas.terms->cursor += sizeof(Term)+sizeof(Node);
		arenas.terms->used += sizeof(Term)+sizeof(Node);
	}
	result->raw = text_init(str8l(""), deshi_allocator);
	return result;
}

global void
delete_term(Term* t) {
	text_deinit(&t->raw);
	Node* n = (Node*)(t-sizeof(Node));
	NodeInsertPrev(&arenas.inactive_terms, n);
}

global MathObject* 
create_math_object() {
	MathObject* result;
	if(arenas.inactive_math_objects.next != &arenas.inactive_math_objects){ // this probably shouldn't happen
		result = (MathObject*)(arenas.inactive_math_objects.next+sizeof(Node));
		NodeRemove(arenas.inactive_math_objects.next);
		ZeroMemory(result, sizeof(MathObject));
	}else{
		if(arenas.math_objects->used + sizeof(MathObject)+sizeof(Node) > arenas.math_objects->size) {
			LogE("suugu_memory", "ran out of memory when allocating a MathObject, allocated size is ", arenas.math_objects->size/bytesDivisor(arenas.math_objects->size), " ", bytesUnit(arenas.math_objects->size));
			return 0;
		}
		result = (MathObject*)arenas.math_objects->cursor;
		arenas.math_objects->cursor += sizeof(MathObject)+sizeof(Node);
		arenas.math_objects->used += sizeof(MathObject)+sizeof(Node);
	}
	return result;
}

global void
delete_math_object(MathObject* mathobj) {
	NotImplemented; // TODO(sushi) when we need it 
}


#endif //SUUGU_TYPES_H
