

#define ErrorMessage(...)\
LogE("MathObjectCompiler", compiler.file->name, ":", compiler.curt->line, ":", compiler.curt->column, ": ", __VA_ARGS__)

enum {
    Token_Semicolon,
    Token_Comma,
    Token_Colon,
    Token_Period,
    Token_OpenBrace,
    Token_CloseBrace,
    Token_OpenParen,
    Token_CloseParen,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_Dollar,
    Token_String,
    Token_Word,
    Token_Integer,

    Token_Group_Keywords,

    Token_Keyword_form = Token_Group_Keywords,
    Token_Keyword_manipulations,
    Token_Keyword_visual,
    Token_Keyword_text,
    Token_Keyword_instructions,
    Token_Keyword_align,
    Token_Keyword_glyph,
    Token_Keyword_to,
    Token_Keyword_is,
    Token_Keyword_MathObject,

    Token_Keyword_right,
    Token_Keyword_left,
    Token_Keyword_top,
    Token_Keyword_topright,
    Token_Keyword_topleft,
    Token_Keyword_bottom,
    Token_Keyword_bottomright,
    Token_Keyword_bottomleft,
    Token_Keyword_origin,
    Token_Keyword_origin_x,
    Token_Keyword_origin_y,
    Token_Keyword_center,
    Token_Keyword_center_x,
    Token_Keyword_center_y,

    Token_Group_MathObj_Types,
    
    Token_Keyword_Function = Token_Group_MathObj_Types,

    Token_COUNT,
};

Type token_align_keyword_to_align_type(Type in) {
    switch(in) {
        case Token_Keyword_right:       return AlignType_Right;
        case Token_Keyword_left:        return AlignType_Left;
        case Token_Keyword_top:         return AlignType_Top;
        case Token_Keyword_topright:    return AlignType_TopRight;
        case Token_Keyword_topleft:     return AlignType_TopLeft;
        case Token_Keyword_bottom:      return AlignType_Bottom;
        case Token_Keyword_bottomright: return AlignType_BottomRight;
        case Token_Keyword_bottomleft:  return AlignType_BottomLeft;
        case Token_Keyword_origin:      return AlignType_Origin;
        case Token_Keyword_origin_x:    return AlignType_OriginX;
        case Token_Keyword_origin_y:    return AlignType_OriginY;
        case Token_Keyword_center:      return AlignType_Center;
        case Token_Keyword_center_x:    return AlignType_CenterX;
        case Token_Keyword_center_y:    return AlignType_CenterY;
    }
    LogE("MathObjectCompiler", "unknown token align type: ", in);
    return 0;
}

u32 word_or_keyword(str8 s) {
    u64 hash = str8_hash64(s);
#define str8case(str) case str8_static_hash64(str8_static_t(str))
    switch(hash){
        str8case("Function"):      return Token_Keyword_Function;
        str8case("form"):          return Token_Keyword_form;
        str8case("manipulations"): return Token_Keyword_manipulations;
        str8case("visual"):        return Token_Keyword_visual;
        str8case("text"):          return Token_Keyword_text;
        str8case("instructions"):  return Token_Keyword_instructions;
        str8case("align"):         return Token_Keyword_align;
        str8case("glyph"):         return Token_Keyword_glyph;
        str8case("to"):            return Token_Keyword_to;
        str8case("is"):            return Token_Keyword_is;
        str8case("MathObject"):    return Token_Keyword_MathObject;
        str8case("right"):         return Token_Keyword_right;
        str8case("left"):          return Token_Keyword_left;
        str8case("top"):           return Token_Keyword_top;
        str8case("topright"):      return Token_Keyword_topright;
        str8case("topleft"):       return Token_Keyword_topleft;
        str8case("bottom"):        return Token_Keyword_bottom;
        str8case("bottomright"):   return Token_Keyword_bottomright;
        str8case("bottomleft"):    return Token_Keyword_bottomleft;
        str8case("origin"):        return Token_Keyword_origin;
        str8case("origin_x"):      return Token_Keyword_origin_x;
        str8case("origin_y"):      return Token_Keyword_origin_y;
        str8case("center"):        return Token_Keyword_center;
        str8case("center_x"):      return Token_Keyword_center_x;
        str8case("center_y"):      return Token_Keyword_center_y;
    };
    return Token_Word;
#undef str8case
}

str8 tokenStrings[] = {
    str8l("Semicolon"),
    str8l("Comma"),
    str8l("Colon"),
    str8l("Period"),
    str8l("OpenBrace"),
    str8l("CloseBrace"),
    str8l("OpenParen"),
    str8l("CloseParen"),
    str8l("OpenBracket"),
    str8l("CloseBracket"),
    str8l("Dollar"),
    str8l("String"),
    str8l("Word"),
    str8l("Integer"),

    str8l("keyword \"form\""),
    str8l("keyword \"manipulations\""),
    str8l("keyword \"visual\""),
    str8l("keyword \"text\""),
    str8l("keyword \"instructions\""),
    str8l("keyword \"align\""),
    str8l("keyword \"glyph\""),
    str8l("keyword \"to\""),
    str8l("keyword \"is\""),
    str8l("keyword \"MathObject\""),
    
    str8l("Function"), 
};

struct Token {
    str8 raw;
    Type type;
    u64 line, column;
};

struct{ // compiler
    SymbolTable symbol_table;

    File* file;
    u64 line;
    u64 column;

    str8 buffer;
    str8 stream;

    // the MathObject we are currently defining
    MathObject* current_mathobj;

    Token* tokens; // kigu array
    Token* curt; // pointer to the current token being parsed 
    u32* MathObject_tokens; // kigu array of MathObject tokens so that we can make top level names global 
    u32* returns; // kigu array of tokens to return to when parsing global names is done
    MathObject** return_mathobj; // kigu array parallel to returns that gives the mathobj associated with the data we are returning to

    b32 parse_fail;
}compiler;

void print_symbols(Symbol** table) {
    forI(array_count(*table)) {
        Symbol s = (*table)[i];
        switch(s.type){
            case SymbolType_Child:{
                Log("", "symbol ", s.name, " is a child representing child '", s.child_idx, "'.");
            }break;
            case SymbolType_Glyph:{
                Log("", "symbol ", s.name, " is a glyph representing '", s.glyph, "'.");
            }break;
            case SymbolType_MathObject:{
                Log("", "symbol ", s.name, " is a MathObject with addr ", (void*)s.mathobj);
            }break;
        }
    }
}

b32 is_digit(u32 codepoint){
	switch(codepoint){
		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9':
			return true;
		default:
			return false;
	}
}

void eat_whitespace() {
    while(compiler.stream) {
        if(!is_whitespace(*compiler.stream.str)){
            break;
        }
        if(*compiler.stream.str == '\n'){
            compiler.column = 1;
            compiler.line++;
        }else{
            compiler.column++;
        }
        str8_advance(&compiler.stream);
    }
}

void advance_stream() {
    if(*compiler.stream.str == '\n'){
        compiler.column = 1;
        compiler.line++;
    }else{
        compiler.column++;
    }
    str8_advance(&compiler.stream);
}

b32 compile_parse_instructions() {
    MathObject* curmo = compiler.current_mathobj;
    array_init(curmo->display.instructions, 4, deshi_allocator);
    array_init(curmo->display.symbols, 1, deshi_allocator);
    while(1){
        switch(compiler.curt->type) {
            case Token_Word:{
                // user must be defining a new symbol to reference later
                // we need to figure out what type of symbol it is
                str8 symname = compiler.curt->raw;

                compiler.curt++;
                if(compiler.curt->type != Token_Keyword_is) {
                    ErrorMessage("expected 'is' after symbol definition name.");
                    return 0;
                }

                compiler.curt++;
                Type symtype;
                switch(compiler.curt->type) {
                    case Token_Dollar: symtype = SymbolType_Child; break;
                    case Token_Keyword_glyph: symtype = SymbolType_Glyph; break;
                    default:{
                        ErrorMessage("expected a glyph or child value for definition of symbol '", symname, "'");
                        return 0;
                    }break;
                }

                auto [symbol, found] = symbol_table_add(&curmo->display.symbols, symname, symtype);
                if(found){
                    ErrorMessage("symbol '", symbol->name, "' already defined on line ", symbol->line, " in column ", symbol->column);
                    return 0;
                }

                symbol->name = symname;

                switch(symbol->type){
                    case SymbolType_Child:{
                        compiler.curt++;
                        if(compiler.curt->type != Token_Integer) {
                            ErrorMessage("expected an integer after '$' to indicate which child this symbol represents");
                            return 0;
                        }
                        symbol->child_idx = stolli(compiler.curt->raw);
                    }break;
                    case SymbolType_Glyph:{
                        compiler.curt++;
                        if(compiler.curt->type != Token_String) {
                            ErrorMessage("expected a string after 'glyph'");
                            return 0;
                        }
                        symbol->glyph = compiler.curt->raw;
                    }break;
                }

                compiler.curt++;
                if(compiler.curt->type != Token_Semicolon) {
                    ErrorMessage("expected semicolon after definition of symbol '", symname, "'");
                    return 0;
                }
                compiler.curt++;
            }break;
            case Token_Keyword_align:{
                print_symbols(&curmo->display.symbols);
                AlignInstruction instr;
                compiler.curt++;

                if(compiler.curt->type != Token_Word){
                    ErrorMessage("unexpected token '", compiler.curt->type, "'. Expected a symbol for operand of 'align'.");
                    return 0;
                }

                auto p = symbol_table_find(&curmo->display.symbols, str8_hash64(compiler.curt->raw));
                spt idx = p.first;
                b32 found = p.second;
                if(!found){
                    ErrorMessage("reference to unknown symbol '", compiler.curt->raw, "'");
                    return 0;
                }

                instr.lhs.symbol = curmo->display.symbols + idx;
                
                compiler.curt++;
                if(compiler.curt->type == Token_Keyword_to) {
                    ErrorMessage("cannot specify alignment of an entire symbol, you must specify some part of it (right, left, topleft, etc.) to align");
                    return 0;
                }

                instr.lhs.align_type = token_align_keyword_to_align_type(compiler.curt->type);
                if(!instr.lhs.align_type) {
                    ErrorMessage("unexpected token '", compiler.curt->raw, "' following symbol. Expected one of: top, left, right, bottom, topleft, topright, bottomleft, bottomright, center, center_x, center_y, origin, origin_x, origin_y");
                    return 0;
                }

                compiler.curt++;
                if(compiler.curt->type != Token_Keyword_to) {
                    ErrorMessage("expected 'to' between operands of 'align'");
                    return 0;
                }

                compiler.curt++;
                p = symbol_table_find(&curmo->display.symbols, str8_hash64(compiler.curt->raw));
                idx = p.first;
                found = p.second;
                if(!found){
                    ErrorMessage("reference to unknown symbol '", compiler.curt->raw, "'");
                    return 0;
                }

                instr.rhs.symbol = curmo->display.symbols + idx;

                // TODO(sushi) this is a poor check for this error
                compiler.curt++;
                if(compiler.curt->type == Token_Semicolon) {
                    ErrorMessage("cannot specify alignment of an entire symbol, you must specify some part of it (right, left, topleft, etc.) to align");
                    return 0;
                }

                instr.rhs.align_type = token_align_keyword_to_align_type(compiler.curt->type);
                if(!instr.rhs.align_type) {
                    ErrorMessage("unexpected token '", compiler.curt->raw, "' following symbol. Expected one of: top, left, right, bottom, topleft, topright, bottomleft, bottomright, center, center_x, center_y, origin, origin_x, origin_y");
                    return 0;
                }

                Instruction* out = array_push(compiler.current_mathobj->display.instructions);
                out->type = InstructionType_Align;
                out->align = instr;

                compiler.curt++;
                if(compiler.curt->type != Token_Semicolon) {
                    ErrorMessage("expected ';' after align instruction.");
                    return 0;
                }

                compiler.curt++;
            }break;
        }
        if(compiler.curt->type == Token_CloseBracket) break;
    }
    return 1;
}

b32 compile_parse() {
    bool failed = 0;
    for_array(compiler.MathObject_tokens){ 
        compiler.curt = compiler.tokens + *it + 1;
        if(compiler.curt->type != Token_Word) {
            ErrorMessage("expected a name for top level definition.");
            compiler.parse_fail = 1;
            continue;
        }
        
        auto [symbol, found] = symbol_table_add(&compiler.symbol_table, compiler.curt->raw, SymbolType_MathObject);
        if(found){
            ErrorMessage("symbol '", compiler.curt->raw, "' is already defined on line ", symbol->line, " in column ", symbol->column);
            compiler.parse_fail = 1;
            continue;
        }
        symbol->mathobj = make_math_object();
        symbol->name   = compiler.curt->raw;
        symbol->line   = compiler.curt->line;
        symbol->column = compiler.curt->column;
        compiler.current_mathobj = symbol->mathobj;
        while((compiler.curt+1)->type == Token_Comma) {
            compiler.curt += 2;
            auto [symbol, found] = symbol_table_add(&compiler.symbol_table, compiler.curt->raw, SymbolType_MathObject);
            if(found){
                ErrorMessage("symbol '", compiler.curt->raw, "' is already defined on line ", symbol->line, " in column ", symbol->column);
                compiler.parse_fail = 1;
                goto subcontinue0;    
            }
            symbol->mathobj = compiler.current_mathobj;
            symbol->line = compiler.curt->line;
            symbol->column = compiler.curt->column;
            symbol->name = compiler.curt->raw;
        }

        compiler.curt++;
        if(compiler.curt->type != Token_Keyword_is) {
            ErrorMessage("expected 'is' after name(s) of top level definition");
            compiler.parse_fail = 1;
            continue;
        }
        
        compiler.curt++;
        if(compiler.curt->type < Token_Group_MathObj_Types) {
            ErrorMessage("expected a MathObject type after 'is'. Possible types are: ");
            forI(Token_COUNT - Token_Group_MathObj_Types) {
                Log("", tokenStrings[i+Token_Group_MathObj_Types]);
            }
            compiler.parse_fail = 1;
            continue;
        }

        switch(compiler.curt->type){
            case Token_Keyword_Function: compiler.current_mathobj->type = MathObject_Function; break;
            default: DebugBreakpoint;
        };
        
        compiler.curt++;
        if(compiler.curt->type != Token_OpenBrace) {
            ErrorMessage("expected a '{' after MathObject type in definition of ", symbol->name);
            compiler.parse_fail = 1;
            continue;
        }

        *array_push(compiler.returns) = compiler.curt - compiler.tokens;
        *array_push(compiler.return_mathobj) = compiler.current_mathobj;


subcontinue0:;
    }

    forI(array_count(compiler.returns)) {
        compiler.current_mathobj = compiler.return_mathobj[i];
        compiler.curt = compiler.tokens + compiler.returns[i];
        while(compiler.curt->type != Token_CloseBrace){
            compiler.curt++;
            switch(compiler.curt->type) {

                case Token_Keyword_form: {
                    Log("MathObjectCompiler", "TODO(sushi) implement tree parser");
                    while(compiler.curt->type != Token_Semicolon) compiler.curt++;
                    compiler.curt++;
                }break;

                case Token_Keyword_visual: {
                    compiler.curt++;
                    if(compiler.curt->type != Token_Colon) {
                        ErrorMessage("expected ':' after key 'visual'");
                        compiler.parse_fail = 1;
                        continue;
                    }

                    compiler.curt++;
                    if(compiler.curt->type != Token_OpenBrace) {
                        ErrorMessage("expected '{' starting a map for key 'visual'");
                        compiler.parse_fail = 1;
                        continue;
                    }

                    while(1) {
                        compiler.curt++;
                        switch(compiler.curt->type){

                            case Token_Keyword_text:{
                                compiler.curt++;
                                if(compiler.curt->type != Token_Colon) {
                                    ErrorMessage("expected ':' after key 'text'");
                                    compiler.parse_fail = 1;
                                    goto subcontinue1;
                                }

                                compiler.curt++;
                                if(compiler.curt->type != Token_String) {
                                    ErrorMessage("expected string for key 'text'");
                                    compiler.parse_fail = 1;
                                    goto subcontinue1;
                                }

                                compiler.current_mathobj->display.text = compiler.curt->raw;

                                compiler.curt++;
                                if(compiler.curt->type != Token_Semicolon){
                                    ErrorMessage("expected ';' after value of key 'text'");
                                    compiler.parse_fail = 1;
                                    goto subcontinue1;
                                }
                            }break;

                            case Token_Keyword_instructions: {
                                compiler.curt++;
                                if(compiler.curt->type != Token_Colon) {
                                    ErrorMessage("expected ':' after key 'instructions'");
                                    compiler.parse_fail = 1;
                                    goto subcontinue1;
                                }

                                compiler.curt++;
                                if(compiler.curt->type != Token_OpenBracket){
                                    ErrorMessage("expected a '[' to start an array of instructions for key 'instructions'");
                                    compiler.parse_fail = 1;
                                    goto subcontinue1;
                                }

                                compiler.curt++;
                                compile_parse_instructions();

                            }break;
                        }

                        if(compiler.curt->type == Token_CloseBrace) break;
                    }

                }break; 
            }
        }
subcontinue1:;
    }


    return !compiler.parse_fail;
}

b32 compile_lex() {
#define charcase(c, type_)                  \
case c: {                                   \
    Token* t = array_push(compiler.tokens); \
    t->line = compiler.line;                \
    t->column = compiler.column;            \
    t->raw = str8{compiler.stream.str, 1};  \
    t->type = type_;                        \
    advance_stream();                       \
    eat_whitespace();                       \
}break;
    while(compiler.stream) {
        switch(*compiler.stream.str) {
            charcase(';', Token_Semicolon);
            charcase(',', Token_Comma);
            charcase(':', Token_Colon);
            charcase('.', Token_Period);
            charcase('{', Token_OpenBrace);
            charcase('}', Token_CloseBrace);
            charcase('(', Token_OpenParen);
            charcase(')', Token_CloseParen);
            charcase('[', Token_OpenBracket);
            charcase(']', Token_CloseBracket);
            charcase('$', Token_Dollar);


            case '"' :{
                u32 ls = compiler.line, cs = compiler.column;
                advance_stream();
                str8 start = compiler.stream;
                while(compiler.stream){
                    advance_stream();
                    if(*compiler.stream.str == '\n'){
                        ErrorMessage("newlines in strings are not supported.");
                        return 0;
                    }
                    if(*compiler.stream.str == '"'){
                        break;
                    }
                } 
                if(!compiler.stream) {
                    ErrorMessage("unexpected end of file while parsing string starting on line ", ls, " at column ", cs, ".");
                    return 0;
                }
                Token* t = array_push(compiler.tokens);
                t->type = Token_String;
                t->raw = {start.str, compiler.stream.str - start.str};
                t->line = ls;
                t->column = cs;
                advance_stream();
                eat_whitespace();
            }break;

            case '/': {
                
                // possibly a comment
                if(*(compiler.stream.str+1) == '/') {
                    while(compiler.stream && *compiler.stream.str != '\n') advance_stream();
                    advance_stream();
                } else if (*(compiler.stream.str+1) == '*') {
                    while(!(*compiler.stream.str == '*' && *(compiler.stream.str+1) == '/')) advance_stream();
                    advance_stream();
                    advance_stream();
                }
                eat_whitespace();
            }break;

            default: {
                str8 start = compiler.stream;
                b32 is_word = true;
                if(is_digit(*start.str)) {
                    is_word = false;
                    while(is_digit(*compiler.stream.str)) advance_stream();
                } else while(compiler.stream) {
                    if(is_whitespace(*compiler.stream.str) || 
                        match_any(*compiler.stream.str, 
                        ':', '{', '}', '(', ')', ';', '[', ']', '"', ',', '.')) {
                        break;
                    }
                    advance_stream();
                }
                // TODO(sushi) we can hash the raw and store it on the token to prevent hashing repeatedly later
                Token* t = array_push(compiler.tokens);
                t->raw = {start.str, compiler.stream.str - start.str};
                t->line = compiler.line;
                t->column = compiler.column;
                t->type = (is_word? word_or_keyword(t->raw) : Token_Integer);
                if(t->type == Token_Keyword_MathObject){
                    *array_push(compiler.MathObject_tokens) = array_count(compiler.tokens) - 1;
                }
                eat_whitespace();

            }break;
        }
    }
    return 1;
}

// returns an array of symbols pertaining to MathObjects
// the array is really a map from identifiers to MathObjects,
// so symbol_table_ functions should be used to interact with it.
SymbolTable compile_math_objects(str8 path) {
    compiler.line = compiler.column = 1;
    compiler.file = file_init(path, FileAccess_Write);
    compiler.buffer = file_read_alloc(compiler.file, compiler.file->bytes, deshi_allocator);
    defer{memzfree(compiler.buffer.str);};

    compiler.stream = compiler.buffer;

    array_init(compiler.symbol_table, 1, deshi_allocator);
    array_init(compiler.tokens, 1, deshi_allocator);
    array_init(compiler.MathObject_tokens, 1, deshi_allocator);
    array_init(compiler.returns, 1, deshi_allocator); // TODO(sushi) since we can count how many mathobj there will be before we need these we can preallocate them with exact amounts.
    array_init(compiler.return_mathobj, 1, deshi_allocator); 
    defer{
        array_deinit(compiler.symbol_table);
        array_deinit(compiler.tokens);
        array_deinit(compiler.MathObject_tokens);
        array_deinit(compiler.returns);
        array_deinit(compiler.return_mathobj);
    };

    if(!compile_lex()) return 0;

    // debug tokens
    // forX_array(token, compiler.tokens) {
    //     Log("", tokenStrings[token->type], " ", token->raw);
    // }

    compiler.curt = compiler.tokens;
    if(!compile_parse()) return 0;

    forI(array_count(compiler.symbol_table)){
        Symbol s = compiler.symbol_table[i];
        if(s.type == SymbolType_MathObject){ 
            Log("", "Symbol ", s.name, ":");
            MathObject* mo = s.mathobj;
            Instruction* instr = mo->display.instructions;
            if(!instr) continue;
            forI(array_count(instr)) {
                Log("", instr[i]);
            }
        }
    }

    return compiler.symbol_table;
}

#undef ErrorMessage