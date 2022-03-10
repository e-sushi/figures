#pragma once
#ifndef SUUGU_TYPES_H
#define SUUGU_TYPES_H

#include "kigu/common.h"

//TODO(sushi) update parsing description and move it to parsing.cpp
/*
here im going to write out some examples of what our trees look like
see suugu.cpp for our brackus naur grammar, that defines how these trees are made up

this was written 08/08/2021, so it could be outdated

an AST for something like 2 + 2 would be 
					
					statement (the user's input)
						|
				    expression
						|
					  bitOR
					    |
					 bit XOR
					    |
					 bit AND
					    |
					 equality
					    |
					relational
					    |
					bit shift
					    |
					 additive
					/   |   \
				term  binop  term
				 /		|      \			
			 factor     +     factor
			  /                   \
		  literal               literal

which is very long, but its necessary we have these many steps
to ensure we evaluate expressions according to precedence and associativity

the way we search this tree to evaluate it is through a depth-first search (DFS)

im going to write out other examples so i can reference what the tree should look like

------  (3 - 2) * 2  ------
   this ex shows how incredibly deep these trees can get with just one set of parenthesis
			
					statement (the user's input)
						|
					   exp
						|
					  bitOR
						|
					 bit XOR
						|
					 bit AND
						|
					 equality
						|
					relational
						|
					bit shift
						|
					 additive
						|   
			-----------term-----------
			|           |            |
			|         binop          |
		  factor        |         factor
		 /  |  \        *            |   
	   (   exp   )                literal
			|                        |
		  bitOR                      2
			|
		 bit XOR
			|
		 bit AND
			|
		 equality
			|
		relational
			|
		bit shift
			|
		 additive
			|
		  binop
		 /  |  \
   literal  -  literal
	  |           |
	  3           2

the branches from factor containing the ( and ) are only to indicate that the factor decided to have that 
next node because it found parenthesis, in the actual tree those nodes wouldn't be there

------  2 * 2 * 2 * 2  ------


							  statement (the user's input)
								  |
								 exp
								  |
								bitOR
								  |
							   bit XOR
								  |
							   bit AND
								  |
							   equality
								  |
							  relational
								  |
							  bit shift
								  |
							   additive
								  |
		 ------------------------term-----------------------
		 |       |        |       |       |        |       |
	  factor   binop   factor   binop   factor   binop   factor
		 |       |        |       |       |        |       |
	  literal    *     literal    *    literal     *    literal
		 |                |               |                |
		 2                2               2                2


------  1 - 2 * !3 & 4  ------

		
							  statement (the user's input)
								  |
								 exp
								  |
								bitOR
								  |
							   bit XOR
								  |
				  -------------bit AND-------------
				  |               |               |              
			  equality            &           equality   
				  |                               |
			  relational                      relational
				  |                               |
			  bit shift                       bit shift
				  |                               |
			  additive----                     additive
			 /    |        \                      |
		  term    -    ----term-----             term
		   |           |     |     |              |
		factor     factor  binop  factor        factor
		   |         |       |   /   |            |
		literal   literal    *  !    |         literal
		   |         |            factor          |
		   1         2               |            4
								  literal
									 |
									 3

its probably worth explaining how we'll step through a complex tree like this
remember that we're doing DFS

imagine we have 2 registers whose purpose is to store a value and 
a stack whose purpose is to store several valuies for later use

[1, 2, ...] represents the stack 
{a, b}      represents registers a and b's values

reg a is our MAIN register and b is a helper who helps with binary op operations

->  indicates going down the tree into the specified node
-^  indicates going up the tree to the specified node
... indicates skipping through nodes we dont need to worry about

	statement -> ... -> bit AND -> LEFT equality -> ... -> additive -> LEFT term -> factor -> literal
	
----we create the value 1 and put it in reg a
	{1, 0} [] 
	
	literal -^ ... -^ additive -> -

----we see that we have to do a subtraction so we store reg a (left nodes 'result') on the stack
	{1, 0} [1]

	- -^ additive -> RIGHT term -> LEFT factor -> literal

----we create the value 2 and put it in reg a
	{2, 0} [1]

	literal -^ factor -^ term -> *

----we see that we have to do a subtraction with the last result so we store it
	{2, 0} [2, 1]

	* -^ term -> factor -> unaryop!

----we see that we are going to do a unaryop NOT, but unary ops dont need to store anything, they just evaluate
	their child node and do their operation when its done
	{2, 0} [2, 1]

	unaryop! -^ factor -> factor -> literal

----we create the value 3 and put it in reg a
	{3, 0} [2, 1]

	 literal -^ factor -^ factor 

----this factor knows to evalute the unaryop NOT and does that to reg a
	{0, 0} [2, 1]

	 factor -^ term 

----this term knows to evalute * between the reg and the last stored value so we pop 2 into reg b and multiply a and b
	then store the result in a
	{0, 2} [1]
	 
	 term -^ additive

----this additive node knows to evaluate its left term minus its right term in ASM this is a little more complex
	but here we'll say we pop 1 into reg b, subtract a from b and store the result in reg a
	{0, 1} []

	additive -^ ... -^ bit AND -> &

----from here on its pretty simple, we go down the right side, and when we return we AND the left result
	result with the right result
*/


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
//// @Operator
//TODO add operators as their parsing is implemented
enum Operator : u32{ //number of arguments == number of possible input slots
	Operator_NULL = 0,
	
	//// one argument ////
	//Operator_Negation,
	//Operator_AbsoluteValue,
	//Operator_BitwiseNOT,
	
	//// two arguments ////
	Operator_Addition,
	Operator_Subtraction,
	Operator_Multiplication,
	Operator_Division,
	//Operator_Exponential, //base, power
	//Operator_Root,        //index, radicand
	//Operator_Derivative,  //variable, expression
	//Operator_Integral,    //variable, expression
	//Operator_Limit,       //approach, expression
	//Operator_Modulo,
	//Operator_BitwiseAND,
	//Operator_BitwiseOR,
	//Operator_BitwiseXOR,
	//Operator_ArithmaticShiftLeft,
	//Operator_ArithmaticShiftRight,
	//Operator_LogicalShiftLeft,
	//Operator_LogicalShiftRight,
	//Operator_CircularShiftLeft,
	//Operator_CircularShiftRight,
	
	//// three arguments ////
	//Operator_Sum,         //start, stop, step
	
	//// N arguments ////
	//Operator_PartialDerivative, //variables..., expression
	
	Operator_COUNT,
	//Operator_1ARG_START = Operator_Negation,
	//Operator_1ARG_END   = Operator_Addition-1,
	//Operator_2ARG_START = Operator_Addition,
	//Operator_2ARG_END   = Operator_Sum-1,
	//Operator_3ARG_START = Operator_Sum,
	//Operator_3ARG_END   = Operator_PartialDerivative-1,
	//Operator_NARG_START = Operator_PartialDerivative,
	//Operator_NARG_END   = Operator_COUNT-1,
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

struct Graph{
	vec2f64 position{0,0};
	vec2f64 dimensions{2,-1.25};
	vec2f64 cameraPosition{0,0}; //in graph space
	f64     cameraZoom = 5.0;
	
	f64 gridZoomFit               = 5.0;
	f64 gridZoomFitIncrements[3]  = {2.0, 2.5, 2.0};
	u32 gridZoomFitIncrementIndex = 0;
	u32 gridMajorLinesCount       = 12;
	f64 gridMajorLinesIncrement   = 1.0;
	u32 gridMinorLinesCount       = 4;
	f64 gridMinorLinesIncrement   = 0.2;
	b32 gridShowMajorLines        = true;
	b32 gridShowMinorLines        = true;
	b32 gridShowAxisCoords        = true;
};

//maybe make it so the canvas can store its own windows as well
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

#endif //SUUGU_TYPES_H
