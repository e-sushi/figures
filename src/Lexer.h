#pragma once
#ifndef SUUGU_LEXER_H
#define SUUGU_LEXER_H

#include "utils/string.h"
#include "utils/array.h"

#include "defines.h"

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

} typedef token_type u32;

struct token {
	string str;
	token_type type = 0;
};

array<char> stopping_chars{
	';', ' ', '{',  '}', '\(', '\)',
	',', '+', '*', '/', '-', '<', '>',
	'=', '!', '~', '\n', '&', '|', '^',
	'%', ':', '?'
};

array<string> keywords{
	"int", "return", "if", "else"
};

template<class T>
static int is_in(T& c, array<T>& array) {
	for (T t : array) { if (t == c) return 1; }
	return 0;
}


namespace Lexer {
	//NOTE this has been reimpl from su and could have errors or unecessary things throughout
	array<token> lex(string input) {
		array<token> tokens;
		char currChar = 0;
		string buff = "";
		u32 lines = 1;

		//TODO get rid of maxbuff here
		for(int i = 0; i < input.size; i++) {
			currChar = input[i];
			//check that our current character isn't any 'stopping' characters
			if (is_in(currChar, stopping_chars)) {
				//store both the stopping character and previous buffer as tokens
				//also decide what type of token it is
				if (buff[0]) {
					token t;
					t.str = buff;
					//check if token is a keyword
					if (is_in(buff, keywords)) {
						if      (buff == "int")    t.type = tok_Keyword;
						else if (buff == "if")     t.type = tok_If;
						else if (buff == "else")   t.type = tok_Else;
					}
					//if its not then it could be a number of other things
					else {
						//TODO make this cleaner
						if (isalpha(buff[0])) {
							//if it begins with a letter it must be an identifier
							//for now
							t.type = tok_Identifier;
						}
						else if (isdigit(buff[0])) {
							//check if its a digit, then verify that the rest are digits
							bool error = false;
							for (int i = 0; i < buff.size; i++) {
								if (!isdigit(buff[i]) && buff[i] != '.') error = true;
							}
							if (error) t.type = tok_ERROR;
							else t.type = tok_Literal;
						}
					}
					tokens.add(t);
				}

				//check what our stopping character is 
				if (currChar != ' ' && currChar != '\n') {
					token t;
					t.str = currChar;
					switch (currChar) {
						case ';':  t.type = tok_Semicolon;         break;
						case '{':  t.type = tok_OpenBrace;         break;
						case '}':  t.type = tok_CloseBrace;        break;
						case '\(': t.type = tok_OpenParen;         break;
						case '\)': t.type = tok_CloseParen;        break;
						case ',':  t.type = tok_Comma;             break;
						case '+':  t.type = tok_Plus;              break;
						case '-':  t.type = tok_Negation;          break;
						case '*':  t.type = tok_Multiplication;    break;
						case '/':  t.type = tok_Division;          break;
						case '~':  t.type = tok_BitwiseComplement; break;
						case '%':  t.type = tok_Modulo;            break;
						case '^':  t.type = tok_BitXOR;            break;
						case '?':  t.type = tok_QuestionMark;      break;
						case ':':  t.type = tok_Colon;             break;

						case '&': {
							if (input[i + 1] == '&') {
								t.type = tok_AND;
								t.str += '&';
							}
							else {
								t.type = tok_BitAND;
							}
						}break;

						case '|': {
							if (input[i + 1] == '|') {
								t.type = tok_OR;
								t.str += '|';
							}
							else {
								t.type = tok_BitOR;
							}
						}break;

						case '!': {
							if (input[i + 1] == '=') {
								t.type = tok_NotEqual;
								t.str += '=';
							}
							else {
								t.type = tok_LogicalNOT;
							}
						}break;

						case '=': {
							if (input[i + 1] == '=') {
								t.type = tok_Equal;
								t.str += '=';
							}
							else {
								t.type = tok_Assignment;
							}
						}break;

						case '>': {
							char c = input[i + 1];
							if (c == '=') {
								t.type = tok_GreaterThanOrEqual;
								t.str += '=';
							}
							else if (c == '>') {
								t.type = tok_BitShiftRight;
								t.str += '>';
							}
							else {
								t.type = tok_GreaterThan;
							}
						}break;

						case '<': {
							char c = input[i + 1];
							if (c == '=') {
								t.type = tok_LessThanOrEqual;
								t.str += '=';
							}
							else if (c == '<') {
								t.type = tok_BitShiftLeft;
								t.str += '<';
							}
							else {
								t.type = tok_LessThan;
							}
						}break;
					}
					tokens.add(t);
				}
				buff.clear();
			}
			else if (currChar == EOF) {
				token t;
				t.str = "End of File";
				t.type = tok_EOF;
				tokens.add(t);
				break;
			}
			else if (currChar != '\t' && currChar != '\n') {
				//if not then keep adding to buffer if character is not a newline or tab
				buff += currChar;
			}
			if (currChar == '\n') lines++;
		}

		return tokens;
	}
}

#endif