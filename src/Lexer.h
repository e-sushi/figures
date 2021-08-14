#pragma once
#ifndef SUUGU_LEXER_H
#define SUUGU_LEXER_H

#include "utils/array.h"
#include "utils/string.h"
#include "utils/map.h"

enum token_type {
	tok_ERROR,			    // when something doesnt make sense during lexing
	tok_EOF,			    // end of file
	tok_Keyword,		    // int, float, etc.
	tok_Identifier,		    // function/variable names
	tok_OpenParen,		    // (
	tok_CloseParen,		    // )
	tok_OpenBrace,		    // {
	tok_CloseBrace,		    // }
	tok_Comma,			    // ,
	tok_Semicolon,		    // ;
	tok_Literal,	        // any fixed numerical value
	tok_Assignment,         // =
	tok_Plus,               // +
	tok_Negation,		    // -
	tok_Multiplication,     // *
	tok_Division,           // /
	tok_LogicalNOT,         // !
	tok_BitwiseComplement,  // ~
	tok_LessThan,		    // <
	tok_GreaterThan,        // >
	tok_LessThanOrEqual,	// <=
	tok_GreaterThanOrEqual, // >=
	tok_AND,                // &&
	tok_BitAND,             // &
	tok_OR,                 // ||
	tok_BitOR,              // |
	tok_Equal,              // ==
	tok_NotEqual,           // !=
	tok_BitXOR,				// ^
	tok_BitShiftLeft,		// <<
	tok_BitShiftRight,		// >>
	tok_Modulo,             // %
	tok_QuestionMark,       // ?
	tok_Colon,				// :
	tok_If,					// if
	tok_Else,				// else
	tok_Comment             // a comment, use #

};

local map<token_type, string> tokToStr{
	{tok_ERROR,              ""},
	{tok_EOF,                ""},			       
	{tok_Keyword,            ""},
	{tok_Identifier,         ""},
	{tok_OpenParen,          "("},
	{tok_CloseParen,         ")"},
	{tok_OpenBrace,          "{"},
	{tok_CloseBrace,         "}"},
	{tok_Comma,              ","},
	{tok_Semicolon,          ";"},
	{tok_Literal,            ""},
	{tok_Assignment,         "="},
	{tok_Plus,               "+"},
	{tok_Negation,           "-"},
	{tok_Multiplication,     "*"},
	{tok_Division,           "/"},
	{tok_LogicalNOT,         "!"},
	{tok_BitwiseComplement,  "~"},
	{tok_LessThan,           "<"},
	{tok_GreaterThan,        ">"},
	{tok_LessThanOrEqual,    "<="},
	{tok_GreaterThanOrEqual, ">="},
	{tok_AND,                "&"},
	{tok_BitAND,             "&&"},
	{tok_OR,                 "|"},
	{tok_BitOR,              "||"},
	{tok_Equal,              "=="},
	{tok_NotEqual,           "!="},
	{tok_BitXOR,             "^"},
	{tok_BitShiftLeft,       "<<"},
	{tok_BitShiftRight,      ">>"},
	{tok_Modulo,             "%"},
	{tok_QuestionMark,       "?"},
	{tok_Colon,              ":"},
	{tok_If,                 ""},
	{tok_Else,               ""},
	{tok_Comment,            ""}

};

local map<string, token_type> strToTok{
	{"", tok_ERROR},
	{"", tok_EOF},
	{"", tok_Keyword},
	{"", tok_Identifier},
	{"(", tok_OpenParen},
	{")", tok_CloseParen},
	{"{", tok_OpenBrace},
	{"}", tok_CloseBrace},
	{",", tok_Comma},
	{";", tok_Semicolon},
	{"", tok_Literal},
	{"=", tok_Assignment},
	{"+", tok_Plus},
	{"-", tok_Negation},
	{"*", tok_Multiplication},
	{"/", tok_Division},
	{"!", tok_LogicalNOT},
	{"~", tok_BitwiseComplement},
	{"<", tok_LessThan},
	{">", tok_GreaterThan},
	{"<=", tok_LessThanOrEqual},
	{">=", tok_GreaterThanOrEqual},
	{"&", tok_AND},
	{"&&", tok_BitAND},
	{"|", tok_OR},
	{"||", tok_BitOR},
	{"==", tok_Equal},
	{"!=", tok_NotEqual},
	{"^", tok_BitXOR},
	{"<<", tok_BitShiftLeft},
	{">>", tok_BitShiftRight},
	{"%", tok_Modulo},
	{"?", tok_QuestionMark},
	{":", tok_Colon},
	{"", tok_If},
	{"", tok_Else},
	{"", tok_Comment}

};

struct token {
	string str;
	token_type type;

	union {

	};

	token() {};
	token(token_type t) { type = t; str = *tokToStr.at(t); }
};



namespace Lexer {
	array<token> lex(string input);
}

#endif