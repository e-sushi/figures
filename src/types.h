#pragma once
#ifndef SUUGU_TYPES_H
#define SUUGU_TYPES_H

#include "kigu/common.h"
#include "core/memory.h"

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Token
//TODO add token types as their parsing is implemented
enum TokenType : u32{
	Token_Null = 0,
	Token_ERROR = 0,                // when something doesnt make sense during lexing
	Token_EOF,                      // end of file
	
	Token_Identifier,               // function/variable names
	Token_Literal,
	
	//// control ////
	Token_Semicolon,                // ;
	Token_OpenBrace,                // {
	Token_CloseBrace,               // }
	Token_OpenParen,                // (
	Token_CloseParen,               // )
	Token_OpenSquare,               // [
	Token_CloseSquare,              // ]
	Token_Comma,                    // ,
	Token_QuestionMark,             // ?
	Token_Colon,                    // :
	Token_Dot,                      // .
	Token_At,                       // @
	Token_Pound,                    // #
	Token_Backtick,                 // `
	
	//// operators ////
	Token_Plus,                     // +
	Token_Negation,                 // -
	Token_Multiplication,           // *
	Token_Division,                 // /
	Token_BitNOT,                   // ~
	Token_BitAND,                   // &
	Token_AND,                      // &&
	Token_BitOR,                    // |
	Token_OR,                       // ||
	Token_BitXOR,                   // ^
	Token_BitShiftLeft,             // <<
	Token_BitShiftRight,            // >>
	Token_Modulo,                   // %
	Token_Assignment,               // =
	Token_Equal,                    // ==
	Token_NotEqual,                 // !=
	Token_LogicalNOT,               // !
	Token_LessThan,                 // <
	Token_LessThanOrEqual,          // <=
	Token_GreaterThan,              // >
	Token_GreaterThanOrEqual,       // >=
	
	Token_COUNT,
	Token_CONTROLS_START   = Token_Semicolon,
	Token_CONTROLS_END     = Token_Plus-1,
	Token_OPERATORS_START  = Token_Plus,
	Token_OPERATORS_END    = Token_COUNT-1,
};

struct Token{
	TokenType type;
	char raw[256] = {}; //TODO this is temporary
	union{
		f64 value;   //when literal
		//cstring raw; //when not literal
	};
};

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Expression
enum ExpressionType : u32 {
	Expression_NONE,
	
	Expression_IdentifierLHS,
	Expression_IdentifierRHS,
	
	//Special ternary conditional expression type
	//Expression_TernaryConditional,
	
	//Types
	Expression_Literal,
	
	//Unary Operators
	Expression_UnaryOpBitComp,
	Expression_UnaryOpLogiNOT,
	Expression_UnaryOpNegate,
	
	//Binary Operators
	Expression_BinaryOpPlus,
	Expression_BinaryOpMinus,
	Expression_BinaryOpMultiply,
	Expression_BinaryOpDivision,
	Expression_BinaryOpAND,
	Expression_BinaryOpBitAND,
	Expression_BinaryOpOR,
	Expression_BinaryOpBitOR,
	Expression_BinaryOpLessThan,
	Expression_BinaryOpGreaterThan,
	Expression_BinaryOpLessThanOrEqual,
	Expression_BinaryOpGreaterThanOrEqual,
	Expression_BinaryOpEqual,
	Expression_BinaryOpNotEqual,
	Expression_BinaryOpModulo,
	Expression_BinaryOpBitXOR,
	Expression_BinaryOpBitShiftLeft,
	Expression_BinaryOpBitShiftRight,
	Expression_BinaryOpAssignment,
};

static const char* ExTypeStrings[] = {
	"NONE",
	
	"idLHS: ",
	"idRHS: ",
	
	//"tern: ",
	
	"literal: ",
	
	"~",
	"!",
	"-",
	
	"+",
	"-",
	"*",
	"/",
	"&&",
	"&",
	"||",
	"|",
	"<",
	">",
	"<=",
	">=",
	"==",
	"!=",
	"%",
	"^",
	"<<",
	">>",
	"=",
};

struct Expression {
	ExpressionType type;
	cstring expstr;
	
	//TODO support different types
	f64 val;
	
	TNode node;
	
	Expression() {}
	
	Expression(char* str, ExpressionType type) {
		this->type = type;
		expstr={str, strlen(str)};
	}
};

namespace Parser {
	Expression parse(array<Token> tokens);
	void pretty_print(Expression& e);
}

union vec2f64{
	f64 arr[2];
	struct{ f64 x; f64 y; };
	
	static const vec2f64 ZERO;
	static const vec2f64 ONE;
	static const vec2f64 UP;
	static const vec2f64 DOWN;
	static const vec2f64 LEFT;
	static const vec2f64 RIGHT;
	static const vec2f64 UNITX;
	static const vec2f64 UNITY;
	
	inline void    operator= (vec2f64 rhs){x=rhs.x;y=rhs.y;}
	inline vec2f64 operator- (vec2f64 rhs){return {x-rhs.x,y-rhs.y};}
	inline void    operator-=(vec2f64 rhs){x-=rhs.x;y-=rhs.y;}
	inline vec2f64 operator+ (vec2f64 rhs){return {x+rhs.x,y+rhs.y};}
	inline void    operator+=(vec2f64 rhs){x+=rhs.x;y+=rhs.y;}
	inline vec2f64 operator/ (vec2f64 rhs){return {x/rhs.x,y/rhs.y};}
	inline vec2f64 operator* (f64 rhs){return {x*rhs,y*rhs};}
	inline void    operator*=(f64 rhs){x*=rhs;y*=rhs;}
	inline vec2f64 operator/ (f64 rhs){return {x/rhs,y/rhs};}
	inline void    operator/=(f64 rhs){x/=rhs;y/=rhs;}
	friend vec2f64 operator* (f64 lhs, vec2f64 rhs){return rhs * lhs;}
};

inline const vec2f64 vec2f64::ZERO  = vec2f64{ 0,  0};
inline const vec2f64 vec2f64::ONE   = vec2f64{ 1,  1};
inline const vec2f64 vec2f64::RIGHT = vec2f64{ 1,  0};
inline const vec2f64 vec2f64::LEFT  = vec2f64{-1,  0};
inline const vec2f64 vec2f64::UP    = vec2f64{ 0,  1};
inline const vec2f64 vec2f64::DOWN  = vec2f64{ 0, -1};
inline const vec2f64 vec2f64::UNITX = vec2f64{ 1,  0};
inline const vec2f64 vec2f64::UNITY = vec2f64{ 0,  1};

struct Constant{
	str8 name;
	str8 unit;
	f64  value;
};

struct Element{
	vec2 pos; //NOTE world space //TODO maybe cache a screen position for elements 
	vec2 size; //world size
	s32 cursor = 0; //for tracking where in the token array we are editing
	array<Token> tokens; //list of tokens the user has input and their strings to show 
	Expression statement;
	
	void AddToken(TokenType t);
	//draws input boxes and tokens
	//TODO(sushi) add parameter for if element is active
	void Update();
	
	Element() {};
};

//struct Graph{
//	vec2f64 position{0,0};
//	vec2f64 dimensions{2,-1.25};
//	vec2f64 cameraPosition{0,0}; //in graph space
//	f64     cameraZoom = 5.0;
//	
//	f64 gridZoomFit               = 5.0;
//	f64 gridZoomFitIncrements[3]  = {2.0, 2.5, 2.0};
//	u32 gridZoomFitIncrementIndex = 0;
//	u32 gridMajorLinesCount       = 12;
//	f64 gridMajorLinesIncrement   = 1.0;
//	u32 gridMinorLinesCount       = 4;
//	f64 gridMinorLinesIncrement   = 0.2;
//	b32 gridShowMajorLines        = true;
//	b32 gridShowMinorLines        = true;
//	b32 gridShowAxisCoords        = true;
//};

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

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Element
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

//element: anything with position, size, coordinate space, and display info
struct Element2{
	f64 x, y, z;
	f64 width, height, depth;
	//CoordinateSpace space;
	ElementType type;
};

////workspace: region of the canvas in which expressions are able to interact together  
//struct Expression2;
//struct Workspace{
//Element2 element;
//str8 name;
//array<Expression2*> expressions = array<Expression2*>(deshi_allocator);
//};
//#define ElementToWorkspace(elem_ptr) ((Workspace*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(Workspace, element))))

////graph: graphing grid with a local camera in which equations can be drawn
struct GraphElement{ //NOTE this is in expectance of Graph being extracted to a deshi module
	Element2 element;
	Graph* graph;
};
#define ElementToGraphElement(elem_ptr) ((GraphElement*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(GraphElement, element))))

//struct TextElement{
//Element2 element;
//str8 text;
//Font* font;
//f32 font_height;
//vec2 scale;
//f32 rotation;
//};
//#define ElementToTextElement(elem_ptr) ((TextElement*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(TextElement, element))))

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Term
enum TermType_{
	TermType_Expression,
	TermType_Operator,
	TermType_Literal,
	//TermType_Variable,
	//TermType_FunctionCall,
}; typedef Type TermType;

enum TermFlags_{
	TermFlag_NONE = 0,
	
	//// operator argument flags //// //NOTE these flags are mainly used to determine empty slots on operators
	TermFlag_OpArgLeft   = (1 << 0),
	TermFlag_OpArgRight  = (1 << 1),
	TermFlag_OpArgTop    = (1 << 2),
	TermFlag_OpArgBottom = (1 << 3),
}; typedef Flags TermFlags;
#define OPARG_MASK (TermFlag_OpArgLeft | TermFlag_OpArgRight | TermFlag_OpArgTop | TermFlag_OpArgBottom)
#define RemoveOpArgs(var) RemoveFlag(var, OPARG_MASK)
#define ReplaceOpArgs(var, new_flags) ((var) = (((var) & ~OPARG_MASK) | new_flags))

//term: generic base thing (literal, operator, variable, function call, etc)
//TODO maybe union operator and literal structs into this?
struct Term{
	TermType  type;
	TermFlags flags;
	u32 linear; //left to right position in expression
	
	Term* left;
	Term* right;
	Term* next;
	Term* prev;
	Term* parent;
	Term* first_child;
	Term* last_child;
	u32   child_count;
};

#define for_right(term_ptr) for(Term* it = term_ptr; it != 0; it = it->right)

global_ inline void insert_left(Term* target, Term* term){
	if(target->left) target->left->right = term;
	term->right = target;
	term->left  = target->left;
	target->left = term;
	term->linear = target->linear;
	for_right(term->right) it->linear++;
}

global_ inline void insert_right(Term* target, Term* term){
	if(target->right) target->right->left = term;
	term->left   = target;
	term->right  = target->right;
	target->right = term;
	term->linear = target->linear+1;
	for_right(term->right) it->linear++;
}

global_ inline void remove_leftright(Term* term){
	if(term->right) term->right->left = term->left;
	if(term->left)  term->left->right = term->right;
	for_right(term->right) it->linear--;
	term->right = term->left = 0;
}

global_ inline void insert_after(Term* target, Term* term){
	if(target->next) target->next->prev = term;
	term->next = target->next;
	term->prev = target;
	target->next = term;
}

global_ inline void insert_before(Term* target, Term* term){
	if(target->prev) target->prev->next = term;
	term->prev = target->prev;
	term->next = target;
	target->prev = term;
}

global_ inline void remove_horizontally(Term* term){
	if(term->next) term->next->prev = term->prev;
	if(term->prev) term->prev->next = term->next;
	term->next = term->prev = 0;
}

global_ void insert_last(Term* parent, Term* child){
	child->parent = parent;
	if(parent->first_child){
		insert_after(parent->last_child, child);
		parent->last_child = child;
	}else{
		parent->first_child = child;
		parent->last_child  = child;
	}
	parent->child_count++;
}

global_ void insert_first(Term* parent, Term* child){
	child->parent = parent;
	if(parent->first_child){
		insert_before(parent->first_child, child);
		parent->first_child = child;
	}else{
		parent->first_child = child;
		parent->last_child  = child;
	}
	parent->child_count++;
}

global_ void remove_from_parent(Term* term){
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

global_ void change_parent_insert_last(Term* new_parent, Term* term){
	if(new_parent == term->parent) return;
	remove_from_parent(term);
	remove_horizontally(term);
	insert_last(new_parent, term);
}

global_ void change_parent_insert_first(Term* new_parent, Term* term){
	if(new_parent == term->parent) return;
	remove_from_parent(term);
	remove_horizontally(term);
	insert_first(new_parent, term);
}

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Expression
//expression: collection of terms in the form of a syntax tree
struct Expression2{
	Element2 element;
	//Workspace* workspace;
	Term  term;
	Term* cursor;
	//Term* cursor_end;
	Term* equals;
	b32 valid;
	f64 solution;
	u32 term_count;
};
#define ElementToExpression(elem_ptr) ((Expression2*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(Expression2, element))))
#define ExpressionFromTerm(term_ptr) ((Expression2*)((u8*)(term_ptr) - (upt)(OffsetOfMember(Expression2, term))))

//operator: symbol that represents an operation on one or many terms
//NOTE in order of precedence, so the higher value it is (lower sequentially), the lower the precedence
//NOTE these are logical operators, not symbol-based operators
//TODO try to find a way to store the number of arguments
enum OpType_{
	OpType_NULL = 0,
	
	OpPrecedence_1  = (1 << 8),
	//OpType_Parentheses,
	//OpType_SquareBrackets,
	//OpType_CurlyBrackets,
	//OpType_AbsoluteValue,
	//OpType_Root,
	//OpType_Derivative,
	//OpType_Integral,
	//OpType_Limit,
	//OpType_Sum,
	//OpType_PartialDerivative
	
	OpPrecedence_2  = (1 << 9),
	//OpType_Exponential,
	
	OpPrecedence_3  = (1 << 10),
	//OpType_Negation,
	//OpType_BitwiseNOT,
	//OpType_LogicalNOT,
	
	OpPrecedence_4  = (1 << 11),
	//OpType_ImplicitMultiplication, //5x
	OpType_ExplicitMultiplication, //5*x
	OpType_Division,
	//OpType_Modulo,
	
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
	
	OpType_COUNT,
}; typedef Type OpType;
#define OPPRECEDENCE_MASK 0xFFFFFF00

struct Operator{
	Term term;
	OpType type;
	
	//NOTE these are inverted b/c the precedence flags get larger as they decrease in precedence
	inline b32 operator> (Operator& rhs){ return (type & OPPRECEDENCE_MASK) <  (rhs.type & OPPRECEDENCE_MASK); }
	inline b32 operator>=(Operator& rhs){ return (type & OPPRECEDENCE_MASK) <= (rhs.type & OPPRECEDENCE_MASK); }
	inline b32 operator< (Operator& rhs){ return (type & OPPRECEDENCE_MASK) >  (rhs.type & OPPRECEDENCE_MASK); }
	inline b32 operator<=(Operator& rhs){ return (type & OPPRECEDENCE_MASK) >= (rhs.type & OPPRECEDENCE_MASK); }
};
#define OperatorFromTerm(term_ptr) ((Operator*)((u8*)(term_ptr) - (upt)(OffsetOfMember(Operator, term))))

//TODO rework this to be string based (will just fix a bunch of different issues at the cost of storage)
//     but move the input logic somehere since it acts as a string -> float scanner
struct Literal{
	Term term;
	f64 value;
	u32 decimal; //digits since decimal
	u32 zeros;   //zeros at the end of input
};
#define LiteralFromTerm(term_ptr) ((Literal*)((u8*)(term_ptr) - (upt)(OffsetOfMember(Literal, term))))

#endif //SUUGU_TYPES_H
