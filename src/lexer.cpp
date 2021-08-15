array<char> stopping_chars{
	';', ' ', '{',  '}', '(', ')',
	',', '+', '*', '/', '-', '<', '>',
	'=', '!', '~', '\n', '&', '|', '^',
	'%', ':', '?', '\0'
};

template<class T>
static int is_in(T& c, array<T>& array) {
	for (T t : array) { if (t == c) return 1; }
	return 0;
}

//TODO(sushi) make a map of tokens to characters instead, so implementing new tokens is easier
//NOTE this has been reimpl from su and could have errors or unecessary things throughout
array<token> Lexer::lex(string input) {
	array<token> tokens;
	char currChar = 0;
	string buff = "";
	u32 lines = 1;
    
	//special comment case
	if (input[0] == '#') {
		token t;
		t.str = input.substr(1, input.size);
		t.type = tok_Comment;
		tokens.add(t);
	}
	//special case for one character
	else if (input.size == 1) {
		token t;
		t.str = buff;
        
		//TODO make this cleaner
		if (isalpha(input[0])) {
			//if it begins with a letter it must be an identifier
			//for now
			t.type = tok_Identifier;
		}
		else if (isdigit(input[0])) {
			//check if its a digit, then verify that the rest are digits
			bool error = false;
			for (int i = 0; i < input.size; i++) {
				if (!isdigit(input[i]) && input[i] != '.') error = true;
			}
			if (error) t.type = tok_ERROR;
			else t.type = tok_Literal;
		}
		//if its neither we should have some sepcial character
		else if (stopping_chars.has(buff[0])) {
			//check what our stopping character is 
			if (currChar != ' ' && currChar != '\n') {
				token t;
				t.str = currChar;
				switch (currChar) {
					case ';':  t.type = tok_Semicolon;         break;
					case '{':  t.type = tok_OpenBrace;         break;
					case '}':  t.type = tok_CloseBrace;        break;
					case '(':  t.type = tok_OpenParen;         break;
					case ')':  t.type = tok_CloseParen;        break;
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
					case '&':  t.type = tok_BitAND;            break;
					case '|':  t.type = tok_BitOR;             break;
					case '!':  t.type = tok_LogicalNOT;        break;
					case '=':  t.type = tok_Assignment;        break;
					case '>':  t.type = tok_GreaterThan;       break;
					case '<':  t.type = tok_LessThan;          break;
				}
				tokens.add(t);
			}
		}
        
		tokens.add(t);
	}
	//any other case
	else {
		for (int i = 0; i < input.size; i++) {
			currChar = input[i];
			buff += currChar;
			//check that our current character isn't any 'stopping' characters
			if (is_in(currChar, stopping_chars)) {
				//store both the stopping character and previous buffer as tokens
				//also decide what type of token it is
				if (buff[0]) {
					token t;
					t.str = buff.substr(0, buff.size);
                    
					//TODO make this cleaner
					if (isalpha(buff[0])) {
						//if it begins with a letter it must be an identifier
						t.type = tok_Identifier;
					}
					else if (isdigit(buff[0])) {
						//check if its a digit, then verify that the rest are digits
						bool error = false;
						for (int i = 0; i < buff.size - 1; i++) {
							if (!isdigit(buff[i]) && buff[i] != '.') error = true;
						}
						if (error) t.type = tok_ERROR;
						else t.type = tok_Literal;
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
						case '(':  t.type = tok_OpenParen;         break;
						case ')':  t.type = tok_CloseParen;        break;
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
							if (i != input.size - 1 && input[i + 1] == '&') {
								t.type = tok_AND;
								t.str += '&';
							}
							else {
								t.type = tok_BitAND;
							}
						}break;
                        
						case '|': {
							if (i != input.size - 1 && input[i + 1] == '|') {
								t.type = tok_OR;
								t.str += '|';
							}
							else {
								t.type = tok_BitOR;
							}
						}break;
                        
						case '!': {
							if (i != input.size - 1 && input[i + 1] == '=') {
								t.type = tok_NotEqual;
								t.str += '=';
							}
							else {
								t.type = tok_LogicalNOT;
							}
						}break;
                        
						case '=': {
							if (i != input.size - 1 && input[i + 1] == '=') {
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
			else if (i == input.size - 1) {
				if (buff[0]) {
					token t;
					t.str = buff.substr(0, buff.size);
                    
					//TODO make this cleaner
					if (isalpha(buff[0])) {
						//if it begins with a letter it must be an identifier
						t.type = tok_Identifier;
					}
					else if (isdigit(buff[0])) {
						//check if its a digit, then verify that the rest are digits
						bool error = false;
						for (int i = 0; i < buff.size - 1; i++) {
							if (!isdigit(buff[i]) && buff[i] != '.') error = true;
						}
						if (error) t.type = tok_ERROR;
						else t.type = tok_Literal;
					}
					tokens.add(t);
				}
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
				//buff += currChar;
			}
			if (currChar == '\n') lines++;
		}
	}
	return tokens;
}


