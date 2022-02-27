#pragma once
#ifndef SUUGU_TYPES_H
#define SUUGU_TYPES_H

#include "kigu/common.h"

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

enum TreeNodeType {
	//TODO set this up if we end up going beyond expressions
};

//abstract node tree struct
struct TreeNode {
	TreeNode* next = 0;
	TreeNode* prev = 0;
	TreeNode* parent = 0;
	TreeNode* first_child = 0;
	TreeNode* last_child = 0;
	u32   child_count = 0;
	
	//debug vars
	string comment;
};

#define for_node(node) for(TreeNode* it = node; it != 0; it = it->next)
#define for_node_reverse(node) for(TreeNode* it = node; it != 0; it = it->prev)

inline void insert_after(TreeNode* target, TreeNode* node) {
	if (target->next) target->next->prev = node;
	node->next = target->next;
	node->prev = target;
	target->next = node;
}

inline void insert_before(TreeNode* target, TreeNode* node) {
	if (target->prev) target->prev->next = node;
	node->prev = target->prev;
	node->next = target;
	target->prev = node;
}

inline void remove_horizontally(TreeNode* node) {
	if (node->next) node->next->prev = node->prev;
	if (node->prev) node->prev->next = node->next;
	node->next = node->prev = 0;
}

void insert_last(TreeNode* parent, TreeNode* child) {
	if (parent == 0) { child->parent = 0; return; }
	
	child->parent = parent;
	if (parent->first_child) {
		insert_after(parent->last_child, child);
		parent->last_child = child;
	}
	else {
		parent->first_child = child;
		parent->last_child = child;
	}
	parent->child_count++;
}

void insert_first(TreeNode* parent, TreeNode* child) {
	if (parent == 0) { child->parent = 0; return; }
	
	child->parent = parent;
	if (parent->first_child) {
		insert_before(parent->first_child, child);
		parent->first_child = child;
	}
	else {
		parent->first_child = child;
		parent->last_child = child;
	}
	parent->child_count++;
}

void remove(TreeNode* node) {
	//remove self from parent
	if (node->parent) {
		if (node->parent->child_count > 1) {
			if (node == node->parent->first_child) node->parent->first_child = node->next;
			if (node == node->parent->last_child)  node->parent->last_child = node->prev;
		}
		else {
			Assert(node == node->parent->first_child && node == node->parent->last_child, "if node is the only child node, it should be both the first and last child nodes");
			node->parent->first_child = 0;
			node->parent->last_child = 0;
		}
		node->parent->child_count--;
	}
	
	//add children to parent (and remove self from children)
	if (node->child_count > 1) {
		for (TreeNode* child = node->first_child; child != 0; child = child->next) {
			insert_last(node->parent, child);
		}
	}
	
	//remove self horizontally
	remove_horizontally(node);
	
	//reset self  //TODO not necessary if we are deleting this node, so exclude this logic in another function TreeNodeDelete?
	node->parent = node->first_child = node->last_child = 0;
	node->child_count = 0;
}

void change_parent(TreeNode* new_parent, TreeNode* node) {
	//if old parent, remove self from it 
	if (node->parent) {
		if (node->parent->child_count > 1) {
			if (node == node->parent->first_child) node->parent->first_child = node->next;
			if (node == node->parent->last_child)  node->parent->last_child = node->prev;
		}
		else {
			Assert(node == node->parent->first_child && node == node->parent->last_child, "if node is the only child node, it should be both the first and last child nodes");
			node->parent->first_child = 0;
			node->parent->last_child = 0;
		}
		node->parent->child_count--;
	}
	
	//remove self horizontally
	remove_horizontally(node);
	
	//add self to new parent
	insert_last(new_parent, node);
}



struct ParseArena {
	u8* data = 0;
	u8* cursor = 0;
	upt size = 0;
	
	void init(upt bytes) {
		data = (u8*)memalloc(bytes);
		cursor = data;
		size = bytes;
	}
	
	template<typename T>
		void* add(const T& in) {
		if (cursor - data < sizeof(T) + size) {
			data = (u8*)memalloc(size);
			cursor = data;
		}
		memcpy(cursor, &in, sizeof(T));
		cursor += sizeof(T);
		return cursor - sizeof(T);
	}
};

enum Token_Type {
	Token_ERROR,                    // when something doesnt make sense during lexing
	Token_EOF,                      // end of file
	Token_Identifier,               // function/variable names
	Token_Return,                   // return
	Token_Literal,                  // 1, 2, 3.221, "string", 'c'
	Token_Signed32,                 // s32 
	Token_Signed64,                 // s64 
	Token_Unsigned32,               // u32 
	Token_Unsigned64,               // u64 
	Token_Float32,                  // f32 
	Token_Float64,                  // f64 
	Token_Semicolon,                // ;
	Token_OpenBrace,                // {
	Token_CloseBrace,               // }
	Token_OpenParen,                // (
	Token_CloseParen,               // )
	Token_Comma,                    // ,
	Token_Plus,                     // +
	Token_PlusAssignment,           // +=
	Token_Negation,                 // -
	Token_NegationAssignment,       // -=
	Token_Multiplication,           // *
	Token_MultiplicationAssignment, // *=
	Token_Division,                 // /
	Token_DivisionAssignment,       // /=
	Token_BitNOT,                   // ~
	Token_BitNOTAssignment,         // ~=
	Token_BitAND,                   // &
	Token_BitANDAssignment,         // &=
	Token_AND,                      // &&
	Token_BitOR,                    // |
	Token_BitORAssignment,          // |=
	Token_OR,                       // ||
	Token_BitXOR,                   // ^
	Token_BitXORAssignment,         // ^=
	Token_BitShiftLeft,             // <<
	Token_BitShiftRight,            // >>
	Token_Modulo,                   // %
	Token_ModuloAssignment,         // %=
	Token_Assignment,               // =
	Token_Equal,                    // ==
	Token_LogicalNOT,               // !
	Token_NotEqual,                 // !=
	Token_LessThan,                 // <
	Token_LessThanOrEqual,          // <=
	Token_GreaterThan,              // >
	Token_GreaterThanOrEqual,       // >=
	Token_QuestionMark,             // ?
	Token_Colon,                    // :
	Token_If,                       // if
	Token_Else,                     // else
	Token_Comment,                  // no syntax yet
};

global_ map<Token_Type, const char*> tokToStr{
	{Token_ERROR,              ""},
	{Token_EOF,                ""},			       
	{Token_Identifier,         ""},
	{Token_OpenParen,          "("},
	{Token_CloseParen,         ")"},
	{Token_OpenBrace,          "{"},
	{Token_CloseBrace,         "}"},
	{Token_Comma,              ","},
	{Token_Semicolon,          ";"},
	{Token_Literal,            ""},
	{Token_Assignment,         "="},
	{Token_Plus,               "+"},
	{Token_Negation,           "-"},
	{Token_Multiplication,     "*"},
	{Token_Division,           "/"},
	{Token_LogicalNOT,         "!"},
	{Token_BitNOT,             "~"},
	{Token_LessThan,           "<"},
	{Token_GreaterThan,        ">"},
	{Token_LessThanOrEqual,    "<="},
	{Token_GreaterThanOrEqual, ">="},
	{Token_AND,                "&"},
	{Token_BitAND,             "&&"},
	{Token_OR,                 "|"},
	{Token_BitOR,              "||"},
	{Token_Equal,              "=="},
	{Token_NotEqual,           "!="},
	{Token_BitXOR,             "^"},
	{Token_BitShiftLeft,       "<<"},
	{Token_BitShiftRight,      ">>"},
	{Token_Modulo,             "%"},
	{Token_QuestionMark,       "?"},
	{Token_Colon,              ":"},
	{Token_If,                 ""},
	{Token_Else,               ""},
	{Token_Comment,            ""}
};
global_ map<const char*, Token_Type> strToTok{
	{"",     Token_ERROR},
	{"",     Token_EOF},
	{"",     Token_Identifier},
	{"(",    Token_OpenParen},
	{")",    Token_CloseParen},
	{"{",    Token_OpenBrace},
	{"}",    Token_CloseBrace},
	{",",    Token_Comma},
	{";",    Token_Semicolon},
	{"",     Token_Literal},
	{"=",    Token_Assignment},
	{"+",    Token_Plus},
	{"-",    Token_Negation},
	{"*",    Token_Multiplication},
	{"/",    Token_Division},
	{"!",    Token_LogicalNOT},
	{"~",    Token_BitNOT},
	{"<",    Token_LessThan},
	{">",    Token_GreaterThan},
	{"<=",   Token_LessThanOrEqual},
	{">=",   Token_GreaterThanOrEqual},
	{"&",    Token_AND},
	{"&&",   Token_BitAND},
	{"|",    Token_OR},
	{"||",   Token_BitOR},
	{"==",   Token_Equal},
	{"!=",   Token_NotEqual},
	{"^",    Token_BitXOR},
	{"<<",   Token_BitShiftLeft},
	{">>",   Token_BitShiftRight},
	{"%",    Token_Modulo},
	{"?",    Token_QuestionMark},
	{":",    Token_Colon},
	{"",     Token_If},
	{"",     Token_Else},
	{"",     Token_Comment}
};

struct token {
	Token_Type type;
	//TODO get rid of this static carray in favor of cstring
	char str[255] = "";
	cstring raw;
	vec2 strSize; //the strings size on screen in px
	
	token() {}
	token(Token_Type _type) : type(_type) { strcpy(str, tokToStr[_type]); }
};

enum ExpressionType : u32 {
	Expression_NONE,

	Expression_IdentifierLHS,
	Expression_IdentifierRHS,
	
	//Special ternary conditional expression type
	//maybe used when/if we allow scripting type stuff
	//or yknow, just finish su and make it embedded :)
	//Expression_TernaryConditional,
	
	//Types
	Expression_Literal,
	
	//Unary Operators
	Expression_UnaryOpBitComp,
	Expression_UnaryOpLogiNOT,
	Expression_UnaryOpNegate,
	//Expression_IncrementPrefix,
	//Expression_IncrementPostfix,
	//Expression_DecrementPrefix,
	//Expression_DecrementPostfix,
	
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
	Expression_BinaryOpMemberAccess,
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
	//"++ pre",
	//"++ post",
	//"-- pre",
	//"-- post",
	
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
	"accessor",
};

struct Expression {
	cstring expstr;
	ExpressionType type;
	
	//TODO support different types
	f64 val;

	TreeNode node;
	
	Expression() {}
	
	Expression(char* str, ExpressionType type) {
		this->type = type;
		expstr={str, strlen(str)};
	}
};

namespace Parser {
	Expression parse(array<token> tokens);
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

#endif //SUUGU_TYPES_H
