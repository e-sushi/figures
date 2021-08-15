#pragma once
#ifndef SUUGU_PARSER_H
#define SUUGU_PARSER_H

#include "syntax.h"
#include "lexer.h"

namespace Parser {
	Statement parse(array<token> tokens);
}


#endif //SUUGU_PARSER_H