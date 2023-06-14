

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
    Token_Keyword_name,
    Token_Keyword_desc,
    Token_Keyword_manipulations,
    Token_Keyword_visual,
    Token_Keyword_text,
    Token_Keyword_instructions,
    Token_Keyword_align,
    Token_Keyword_glyph,
    Token_Keyword_to,
    Token_Keyword_is,
    Token_Keyword_MathObject,
    Token_Keyword_key,
    Token_Keyword_arity,
    Token_Keyword_out,
    Token_Keyword_movement,

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
    Token_Keyword_up,
    Token_Keyword_down,

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
        str8case("name"):          return Token_Keyword_name;
        str8case("desc"):          return Token_Keyword_desc;
        str8case("manipulations"): return Token_Keyword_manipulations;
        str8case("visual"):        return Token_Keyword_visual;
        str8case("text"):          return Token_Keyword_text;
        str8case("instructions"):  return Token_Keyword_instructions;
        str8case("align"):         return Token_Keyword_align;
        str8case("glyph"):         return Token_Keyword_glyph;
        str8case("to"):            return Token_Keyword_to;
        str8case("is"):            return Token_Keyword_is;
        str8case("MathObject"):    return Token_Keyword_MathObject;
        str8case("key"):           return Token_Keyword_key;
        str8case("arity"):         return Token_Keyword_arity;
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
        str8case("out"):           return Token_Keyword_out;
        str8case("movement"):      return Token_Keyword_movement;
        str8case("up"):            return Token_Keyword_up;
        str8case("down"):          return Token_Keyword_down;
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
    str8l("keyword \"name\""),
    str8l("keyword \"desc\""),
    str8l("keyword \"manipulations\""),
    str8l("keyword \"visual\""),
    str8l("keyword \"text\""),
    str8l("keyword \"instructions\""),
    str8l("keyword \"align\""),
    str8l("keyword \"glyph\""),
    str8l("keyword \"to\""),
    str8l("keyword \"is\""),
    str8l("keyword \"MathObject\""),
    str8l("keyword \"key\""),
    str8l("keyword \"arity\""),
    str8l("keyword \"out\""),
    str8l("keyword \"movement\""),

    str8l("keyword \"right\""),
    str8l("keyword \"left\""),
    str8l("keyword \"top\""),
    str8l("keyword \"topright\""),
    str8l("keyword \"topleft\""),
    str8l("keyword \"bottom\""),
    str8l("keyword \"bottomright\""),
    str8l("keyword \"bottomleft\""),
    str8l("keyword \"origin\""),
    str8l("keyword \"origin_x\""),
    str8l("keyword \"origin_y\""),
    str8l("keyword \"center\""),
    str8l("keyword \"center_x\""),
    str8l("keyword \"center_y\""),
    str8l("keyword \"up\""),
    str8l("keyword \"down\""),
    
    str8l("Function"), 
};

// TODO(sushi) set this up when we are able to do event driven input
// KeyCode key_str_to_keycode(str8 key) {
// #define str8case(str) case str8_static_hash64(str8_static_t(str))
//     switch(str8_hash64(key)) {
//        str8case("plus"): return Key_EQUALS | InputMod_AnyShift;
//         // TODO(sushi) more input keys         
//     }
// #undef str8case
// }

struct Token {
    str8 raw;
    u64 hash;
    Type type;
    u64 line, column;
};

struct{ // compiler
    SymbolTable symbol_table;
    // TODO(sushi) make it so that only complete symbols get returned
    SymbolTable finished_symbols;

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
}compiler;

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

void compile_parse_tree() {
    NotImplemented;
    MathObject* curmo = compiler.current_mathobj;

    u32 nesting = 0;
    b32 prevparen = 0;
    while(1) {
        switch(compiler.curt->type) {
            case Token_OpenParen: {
                nesting++;
                compiler.curt++;


            }break;
            case Token_Word: {
                
            }break;
        }
        if(nesting==1 && compiler.curt->type == Token_CloseParen) break;
    }
}

b32 compile_parse_instructions() {
    MathObject* curmo = compiler.current_mathobj;
    array_init(curmo->display.instructions, 4, deshi_allocator);
    while(1){
        switch(compiler.curt->type) {
           
            case Token_Keyword_align:{
                AlignInstruction instr;
                compiler.curt++;

                if(compiler.curt->type != Token_Word){
                    ErrorMessage("unexpected token '", compiler.curt->type, "'. Expected a symbol for operand of 'align'.");
                    return false;
                }

                auto p = symbol_table_find(&curmo->display.symbols, compiler.curt->hash);
                spt idx = p.first;
                b32 found = p.second;
                if(!found){
                    ErrorMessage("reference to unknown symbol '", compiler.curt->raw, "'");
                    return false;
                }

                instr.lhs.symbol = curmo->display.symbols + idx;
                
                compiler.curt++;
                if(compiler.curt->type == Token_Keyword_to) {
                    ErrorMessage("cannot specify alignment of an entire symbol, you must specify some part of it (right, left, topleft, etc.) to align");
                    return false;
                }

                instr.lhs.align_type = token_align_keyword_to_align_type(compiler.curt->type);
                if(!instr.lhs.align_type) {
                    ErrorMessage("unexpected token '", compiler.curt->raw, "' following symbol. Expected one of: top, left, right, bottom, topleft, topright, bottomleft, bottomright, center, center_x, center_y, origin, origin_x, origin_y");
                    return false;
                }

                compiler.curt++;
                if(compiler.curt->type != Token_Keyword_to) {
                    ErrorMessage("expected 'to' between operands of 'align'");
                    return false;
                }

                compiler.curt++;
                p = symbol_table_find(&curmo->display.symbols, compiler.curt->hash);
                idx = p.first;
                found = p.second;
                if(!found){
                    ErrorMessage("reference to unknown symbol '", compiler.curt->raw, "'");
                    return false;
                }

                instr.rhs.symbol = curmo->display.symbols + idx;

                // TODO(sushi) this is a poor check for this error
                compiler.curt++;
                if(compiler.curt->type == Token_Semicolon) {
                    ErrorMessage("cannot specify alignment of an entire symbol, you must specify some part of it (right, left, topleft, etc.) to align");
                    return false;
                }

                instr.rhs.align_type = token_align_keyword_to_align_type(compiler.curt->type);
                if(!instr.rhs.align_type) {
                    ErrorMessage("unexpected token '", compiler.curt->raw, "' following symbol. Expected one of: top, left, right, bottom, topleft, topright, bottomleft, bottomright, center, center_x, center_y, origin, origin_x, origin_y");
                    return false;
                }

                Instruction* out = array_push(compiler.current_mathobj->display.instructions);
                out->type = InstructionType_Align;
                out->align = instr;

                compiler.curt++;
                if(compiler.curt->type != Token_Semicolon) {
                    ErrorMessage("expected ';' after align instruction.");
                    return false;
                }

                compiler.curt++;
            }break;
        }
        if(compiler.curt->type == Token_CloseBracket) break;
    }
    return true;
}

b32 compile_parse_movement() {
    MathObject* curmo = compiler.current_mathobj;

    u32 movements_defined = 0;
    while(1) {
        if(compiler.curt->type != Token_Word) {
            ErrorMessage("expected a symbol to assign movements to");
            Log("MathObjectCompiler", "available symbols in this context are: ");
            forI(array_count(curmo->display.symbols)) {
                Log("MathObjectCompiler", curmo->display.symbols[i].name);
            }
            return false;
        }

        auto [idx, found] = symbol_table_find(&curmo->display.symbols, compiler.curt->hash);
        if(!found) {
            ErrorMessage("unknown symbol '", compiler.curt->hash, "'");
            return false;
        }

        Symbol* cursym = curmo->display.symbols + idx;
        Log("aa", cursym->name, " ", (void*)cursym);
        cursym->movement.down  =
        cursym->movement.up    =
        cursym->movement.left  =
        cursym->movement.right = MOVEMENT_NONE;

        compiler.curt++;
        if(compiler.curt->type != Token_Colon) {
            ErrorMessage("expected ':' after symbol key");
            return false;
        }

        compiler.curt++;
        if(compiler.curt->type != Token_OpenBrace) {
            ErrorMessage("expected '{' to being map for movements of symbol.");
            return false;
        }

        compiler.curt++;
        if(compiler.curt->type == Token_CloseBrace) {
            ErrorMessage("empty map, expected at least one of ( left, right, up, down )");
            return false;
        }

        while(1) {
            switch(compiler.curt->type) {
#define movementcase(dir)                                                                          \
case GLUE(Token_Keyword_,dir): {                                                                   \
    compiler.curt++;                                                                               \
    if(compiler.curt->type != Token_Colon) {                                                       \
        ErrorMessage("expected a ':' after key '" STRINGIZE(dir) "'.");                            \
        return false;                                                                              \
    }                                                                                              \
                                                                                                   \
    if(cursym->movement. dir != -1) {                                                              \
        ErrorMessage("defined key 'left' twice");                                                  \
        return false;                                                                              \
    }                                                                                              \
                                                                                                   \
    compiler.curt++;                                                                               \
    switch(compiler.curt->type) {                                                                  \
        case Token_Word:{                                                                          \
            auto [subidx, found] = symbol_table_find(&curmo->display.symbols, compiler.curt->hash);\
            if(idx == subidx) {                                                                    \
                ErrorMessage("cannot assign a movement from a symbol to itself.");                 \
                return false;                                                                      \
            }                                                                                      \
             cursym->movement. dir = subidx;                                                       \
        }break;                                                                                    \
        case Token_Keyword_out: {                                                                  \
            cursym->movement. dir = MOVEMENT_OUT;                                                  \
        }break;                                                                                    \
        default: {                                                                                 \
            ErrorMessage("expected a symbol name or 'out' for movement key '" STRINGIZE(dir) "'"); \
            return false;                                                                          \
        }break;                                                                                    \
    }                                                                                              \
                                                                                                   \
    compiler.curt++;                                                                               \
    if(compiler.curt->type != Token_Semicolon) {                                                   \
        ErrorMessage("expected a ';'");                                                            \
        return false;                                                                              \
    }                                                                                              \
                                                                                                   \
    compiler.curt++;                                                                               \
}break;

                movementcase(left);
                movementcase(right);
                movementcase(up);
                movementcase(down);
            }

            if(compiler.curt->type == Token_CloseBrace) {
                break;
            }
        }

        compiler.curt++;
        movements_defined++;

        if(compiler.curt->type == Token_CloseBrace) {
            if(movements_defined < array_count(curmo->display.symbols)){
                ErrorMessage("you must define a movement for all defined symbols.");
                // TODO(sushi) it would be very easy to show what symbols aren't defined here
                return false;
            }
            break;
        }
    }

    return true;
}

void compile_parse() {
    // gather all MathObjects and store for later
    for_array(compiler.MathObject_tokens){ 
        compiler.curt = compiler.tokens + *it + 1;
        if(compiler.curt->type != Token_Word) {
            ErrorMessage("expected a name for top level definition.");
            continue;
        }
        
        auto [symbol, found] = symbol_table_add(&compiler.symbol_table, compiler.curt->raw, SymbolType_MathObject);
        if(found){
            ErrorMessage("symbol '", compiler.curt->raw, "' is already defined on line ", symbol->line, " in column ", symbol->column);
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
            continue;
        }
        
        compiler.curt++;
        if(compiler.curt->type < Token_Group_MathObj_Types) {
            ErrorMessage("expected a MathObject type after 'is'. Possible types are: ");
            forI(Token_COUNT - Token_Group_MathObj_Types) {
                Log("", tokenStrings[i+Token_Group_MathObj_Types]);
            }
            continue;
        }

        switch(compiler.curt->type){
            case Token_Keyword_Function: compiler.current_mathobj->type = MathObject_Function; break;
            default: DebugBreakpoint;
        };
        
        compiler.curt++;
        if(compiler.curt->type != Token_OpenBrace) {
            ErrorMessage("expected a '{' after MathObject type in definition of ", symbol->name);
            continue;
        }

        *array_push(compiler.returns) = compiler.curt - compiler.tokens;
        *array_push(compiler.return_mathobj) = compiler.current_mathobj;


subcontinue0:;
    }

    // parse each MathObject completely
    forI(array_count(compiler.returns)) {
        MathObject* curmo = compiler.current_mathobj = compiler.return_mathobj[i];
        compiler.curt = compiler.tokens + compiler.returns[i] + 1;
        array_init(curmo->display.symbols, 1, deshi_allocator);
        while(compiler.curt->type != Token_CloseBrace){
            switch(compiler.curt->type) {

                case Token_Keyword_name:{
                    compiler.curt++;
                    if(compiler.curt->type != Token_Colon) {
                        ErrorMessage("expected ':' after key 'name'");
                        goto subcontinue1;
                    }

                    compiler.curt++;
                    if(compiler.curt->type != Token_String) {
                        ErrorMessage("expected string for key 'name'");
                        goto subcontinue1;
                    }
                    
                    curmo->name = compiler.curt->raw;

                    compiler.curt++;
                    if(compiler.curt->type != Token_Semicolon) {
                        ErrorMessage("expected ';'");
                        goto subcontinue1;
                    }

                    compiler.curt++;
                }break;

                case Token_Keyword_desc: {
                    compiler.curt++;
                    if(compiler.curt->type != Token_Colon) {
                        ErrorMessage("expected ':' after key 'desc'");
                        goto subcontinue1;
                    }

                    compiler.curt++;
                    if(compiler.curt->type != Token_String) {
                        ErrorMessage("expected string for key 'desc'");
                        goto subcontinue1;
                    }
                    
                    curmo->description = compiler.curt->raw;

                    compiler.curt++;
                    if(compiler.curt->type != Token_Semicolon) {
                        ErrorMessage("expected ';'");
                        goto subcontinue1;
                    }

                    compiler.curt++;
                }break;

                case Token_Keyword_form: {
                    Log("MathObjectCompiler", "TODO(sushi) implement form parsing");
                    while(compiler.curt->type != Token_Semicolon) compiler.curt++;
                    compiler.curt++;
                }break;

                case Token_Keyword_manipulations: {
                    Log("MathObjectCompiler", "TODO(sushi) implement manipulations parsing");
                    while(compiler.curt->type != Token_CloseBracket) compiler.curt++;
                    compiler.curt++;
                }break;

                case Token_Keyword_arity: {
                    if(compiler.current_mathobj->type != MathObject_Function) {
                        ErrorMessage("key 'arity' can only be used on MathObjects that are Functions");
                        continue;
                    }

                    compiler.curt++;
                    if(compiler.curt->type != Token_Colon) {
                        ErrorMessage("expected ':' after key 'arity'");
                        continue;
                    }

                    compiler.curt++;
                    if(compiler.curt->type != Token_Integer) {
                        ErrorMessage("expected an integer for key 'arity'");
                        continue;
                    }
                    
                    compiler.current_mathobj->func.arity = stoi(compiler.curt->raw);

                    compiler.curt++;
                    if(compiler.curt->type != Token_Semicolon) {
                        ErrorMessage("expected a ';' after value of key 'arity'");
                        continue;
                    }

                    compiler.curt++;
                }break;

                case Token_Keyword_visual: {
                    compiler.curt++;
                    if(compiler.curt->type != Token_Colon) {
                        ErrorMessage("expected ':' after key 'visual'");
                        continue;
                    }

                    compiler.curt++;
                    if(compiler.curt->type != Token_OpenBrace) {
                        ErrorMessage("expected '{' starting a map for key 'visual'");
                        continue;
                    }

                    compiler.curt++;
                    if(compiler.curt->type == Token_CloseBrace) {
                        ErrorMessage("empty visual map.");
                        continue;
                    }

                    while(1) {
                        switch(compiler.curt->type){
                             case Token_Word:{
                                // user must be defining a new symbol to reference later
                                // we need to figure out what type of symbol it is
                                str8 symname = compiler.curt->raw;

                                compiler.curt++;
                                if(compiler.curt->type != Token_Keyword_is) {
                                    ErrorMessage("expected 'is' after symbol definition name.");
                                    return;
                                }

                                compiler.curt++;
                                Type symtype;
                                switch(compiler.curt->type) {
                                    case Token_Dollar: symtype = SymbolType_Child; break;
                                    case Token_Keyword_glyph: symtype = SymbolType_Glyph; break;
                                    default:{
                                        ErrorMessage("expected a glyph or child value for definition of symbol '", symname, "'");
                                        return;
                                    }break;
                                }

                                auto [symbol, found] = symbol_table_add(&curmo->display.symbols, symname, symtype);
                                if(found){
                                    ErrorMessage("symbol '", symbol->name, "' already defined on line ", symbol->line, " in column ", symbol->column);
                                    return;
                                }

                                symbol->name = symname;

                                switch(symbol->type){
                                    case SymbolType_Child:{
                                        compiler.curt++;
                                        if(compiler.curt->type != Token_Integer) {
                                            ErrorMessage("expected an integer after '$' to indicate which child this symbol represents");
                                            return;
                                        }
                                        symbol->child_idx = stolli(compiler.curt->raw);
                                    }break;
                                    case SymbolType_Glyph:{
                                        compiler.curt++;
                                        if(compiler.curt->type != Token_String) {
                                            ErrorMessage("expected a string after 'glyph'");
                                            return;
                                        }
                                        symbol->glyph = compiler.curt->raw;
                                    }break;
                                }

                                compiler.curt++;
                                if(compiler.curt->type != Token_Semicolon) {
                                    ErrorMessage("expected semicolon after definition of symbol '", symname, "'");
                                    return;
                                }
                                compiler.curt++;
                            }break;

                            case Token_Keyword_text:{
                                compiler.curt++;
                                if(compiler.curt->type != Token_Colon) {
                                    ErrorMessage("expected ':' after key 'text'");
                                    goto subcontinue1;
                                }

                                compiler.curt++;
                                if(compiler.curt->type != Token_String) {
                                    ErrorMessage("expected string for key 'text'");
                                    goto subcontinue1;
                                }

                                compiler.current_mathobj->display.text = compiler.curt->raw;

                                compiler.curt++;
                                if(compiler.curt->type != Token_Semicolon){
                                    ErrorMessage("expected ';' after value of key 'text'");
                                    goto subcontinue1;
                                }

                                compiler.curt++;
                            }break;

                            case Token_Keyword_instructions: {
                                compiler.curt++;
                                if(compiler.curt->type != Token_Colon) {
                                    ErrorMessage("expected ':' after key 'instructions'");
                                    goto subcontinue1;
                                }

                                compiler.curt++;
                                if(compiler.curt->type != Token_OpenBracket){
                                    ErrorMessage("expected a '[' to start an array of instructions for key 'instructions'");
                                    goto subcontinue1;
                                }

                                compiler.curt++;
                                if(!compile_parse_instructions()) goto subcontinue1;
                                compiler.curt++;
                            }break;

                            case Token_Keyword_movement: {
                                compiler.curt++;
                                if(compiler.curt->type != Token_Colon) {
                                    ErrorMessage("expected ':' after key 'movement'");
                                    goto subcontinue1;
                                }

                                compiler.curt++;
                                if(compiler.curt->type != Token_OpenBrace) {
                                    ErrorMessage("expected a '{' to start a map for key 'movement'");
                                    goto subcontinue1;
                                }

                                compiler.curt++;
                                if(!compile_parse_movement()) goto subcontinue1;
                                compiler.curt++;
                            }break;
                            default: {
                                ErrorMessage("unexpected token: ", compiler.curt->raw);
                                goto subcontinue1;
                            }break;
                        }

                        if(compiler.curt->type == Token_CloseBrace) break;
                    }

                }break; 
                default: {
                    ErrorMessage("unknown key: ", compiler.curt->raw);
                    return;
                }break;
            }
        }
subcontinue1:;
    }
}

void compile_lex() {
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
                        return;
                    }
                    if(*compiler.stream.str == '"'){
                        break;
                    }
                } 
                if(!compiler.stream) {
                    ErrorMessage("unexpected end of file while parsing string starting on line ", ls, " at column ", cs, ".");
                    return;
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
                if(is_word) {
                    t->hash = str8_hash64(t->raw);
                }
                eat_whitespace();

            }break;
        }
    }
    return;
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
        array_deinit(compiler.tokens);
        array_deinit(compiler.MathObject_tokens);
        array_deinit(compiler.returns);
        array_deinit(compiler.return_mathobj);
    };

    compile_lex();

    // debug tokens
    // forX_array(token, compiler.tokens) {
    //     Log("", tokenStrings[token->type], " ", token->raw);
    // }

    builtin_mathobj.placeholder = make_math_object();
    builtin_mathobj.placeholder->name = str8l("Placeholder");
    builtin_mathobj.placeholder->description = str8l("suugu's placeholder object, anything may go here.");
    builtin_mathobj.placeholder->type = MathObject_Placeholder;
    builtin_mathobj.placeholder->display.text = str8l("□");

    builtin_mathobj.number = make_math_object();
    builtin_mathobj.number->name = str8l("Number");
    builtin_mathobj.number->description = str8l("A mathematical object used to count, measure, and label.");
    builtin_mathobj.number->type = MathObject_Number;

    compiler.curt = compiler.tokens;
    compile_parse();

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
            forI(array_count(mo->display.symbols)) {
                Symbol s = mo->display.symbols[i];

                Log("", s.name, ": movement: ",
                    "up: ", s.movement.up,
                    " down: ", s.movement.down,
                    " left: ", s.movement.left,
                    " right: ", s.movement.right
                );
            }   
            
        }
    }

    return compiler.symbol_table;
}

#undef ErrorMessage