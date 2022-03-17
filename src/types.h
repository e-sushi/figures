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
	CoordinateSpace_World,
	CoordinateSpace_Screen,
	//CoordinateSpace_Workspace, //TODO maybe add local to workspace
};

enum ElementType : u32{
	ElementType_NULL,
	ElementType_Expression,
	//ElementType_Workspace,
	//ElementType_Graph,
	//ElementType_Text,
};

//element: anything with position, size, coordinate space, and display info
struct Element2{
	f64 x, y, z;
	f64 width, height, depth;
	CoordinateSpace space;
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
//struct GraphElement{ //NOTE this is in expectance of Graph being extracted to a deshi module
//Element2 element;
//Graph* graph;
//};
//#define ElementToGraphElement(elem_ptr) ((GraphElement*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(GraphElement, element))))

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
//// @Expression
//expression: collection of terms in the form of a syntax tree
struct Expression2{
	Element2 element;
	//Workspace* workspace;
	TNode  node;
	TNode* cursor;
	//TNode* cursor_end;
	TNode* equals;
	b32 valid;
	f64 solution;
};
#define ElementToExpression(elem_ptr) ((Expression2*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(Expression2, element))))
#define TermNodeToExpression(node_ptr) ((Expression2*)((u8*)(node_ptr) - (upt)(OffsetOfMember(Expression2, node))))

//term: generic base thing (literal, operator, variable, function call, etc)
enum TermType : u32{
	TermType_Expression,
	TermType_Operator,
	TermType_Literal,
	//TermType_Variable,
	//TermType_FunctionCall,
};

enum TermFlags : u32{
	TermFlag_NONE = 0,
	//TermFlag_ParenthesisLeft  = (1 << 0),
	//TermFlag_ParenthesisRight = (1 << 1),
};

//operator: symbol that represents an operation on one or many terms
//NOTE in order of precedence, so the higher value it is (lower sequentially), the lower the precedence
//NOTE these are logical operators, not symbol-based operators
//TODO try to find a way to store the number of arguments
enum OpType : u32{
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
};
#define OPERATOR_PRECEDENCE_MASK 0xFFFFFF00

struct Operator{
	TNode node;
	OpType type;
	
	//NOTE these are inverted b/c the precedence flags get larger as they decrease in precedence
	inline b32 operator> (Operator* rhs){ return (type & OPERATOR_PRECEDENCE_MASK) <  (rhs->type & OPERATOR_PRECEDENCE_MASK); }
	inline b32 operator>=(Operator* rhs){ return (type & OPERATOR_PRECEDENCE_MASK) <= (rhs->type & OPERATOR_PRECEDENCE_MASK); }
	inline b32 operator< (Operator* rhs){ return (type & OPERATOR_PRECEDENCE_MASK) >  (rhs->type & OPERATOR_PRECEDENCE_MASK); }
	inline b32 operator<=(Operator* rhs){ return (type & OPERATOR_PRECEDENCE_MASK) >= (rhs->type & OPERATOR_PRECEDENCE_MASK); }
};
#define TermNodeToOperator(node_ptr) ((Operator*)((u8*)(node_ptr) - (upt)(OffsetOfMember(Operator, node))))

struct Literal{
	TNode node;
	f64 value;
	u32 decimal;
};
#define TermNodeToLiteral(node_ptr) ((Literal*)((u8*)(node_ptr) - (upt)(OffsetOfMember(Literal, node))))

#endif //SUUGU_TYPES_H
