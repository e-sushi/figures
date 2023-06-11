enum{
    SymbolType_Child,
    SymbolType_Glyph,
    SymbolType_MathObject,
};

#define ErrorMessage(...)\
LogE("MathObjectCompiler", compiler.file->name, ":", compiler.curt->line, ":", compiler.curt->column, ": ", __VA_ARGS__);\
DebugBreakpoint

struct Symbol {
    str8 name;
    u64 hash;

    u64 line, column;  // source location of definition

    Type type;
    union{
        u32 child_idx;
        str8 glyph;
        MathObject* mathobj;
    };
};

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
    Token_String,
    Token_Word,

    Token_Group_Keywords,

    Token_Keyword_form = Token_Group_Keywords,
    Token_Keyword_manipulations,
    Token_Keyword_visual,
    Token_Keyword_text,
    Token_Keyword_instructions,
    Token_Keyword_align,
    Token_Keyword_to,
    Token_Keyword_is,
    Token_Keyword_MathObject,
    
    Token_Group_MathObj_Types,
    
    Token_Keyword_Function = Token_Group_MathObj_Types,

    Token_COUNT,
};

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
        str8case("to"):            return Token_Keyword_to;
        str8case("is"):            return Token_Keyword_is;
        str8case("MathObject"):    return Token_Keyword_MathObject;
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
    str8l("String"),
    str8l("Word"),

    str8l("keyword \"form\""),
    str8l("keyword \"manipulations\""),
    str8l("keyword \"visual\""),
    str8l("keyword \"text\""),
    str8l("keyword \"instructions\""),
    str8l("keyword \"align\""),
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
    Symbol* symbol_table;

    File* file;
    u64 line;
    u64 column;

    str8 buffer;
    str8 stream;

    // the MathObject we are currently defining
    MathObject* current_mathobj;

    Token* tokens; // kigu array
    Token* curt; // pointer
    u32* MathObject_tokens; // kigu array of MathObject tokens so that we can make top level names global 
    u32* returns; // kigu array of tokens to return to when parsing global names is done
    MathObject** return_mathobj; // kigu array parallel to last that gives the mathobj associated with the data we are returning to
}compiler;

pair<spt,b32> symbol_table_find(u64 key){
    spt index = -1, middle = -1;
    if(!array_count(compiler.symbol_table)) {
        return {-1,0};
    }
    spt left = 0;
    spt right = array_count(compiler.symbol_table)-1;
    while(left <= right){
        middle = left+((right-left)/2);
        if(compiler.symbol_table[middle].hash == key){
            index = middle;
            break;
        }
        if(compiler.symbol_table[middle].hash < key){
            left = middle+1;
            middle = left+((right-left)/2);
        }else{
            right = middle-1;
        }
    }
    return {middle, index == -1};
}

pair<Symbol*, b32> symbol_table_add(str8 name, Type type) {
    u64 hash = str8_hash64(name);
    
    auto [idx,found] = symbol_table_find(hash);
    if(found) return {&compiler.symbol_table[idx], 1};
    
    Symbol* s = array_insert(compiler.symbol_table, idx);
    s->type = type;
    
    return {s, 0};
}

str8 eat_word() {
    str8 out = compiler.stream;
    
    out.count = compiler.stream.str - out.str;
    return out;
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

void advance_buffer() {
    if(*compiler.stream.str == '\n'){
        compiler.column = 1;
        compiler.line++;
    }else{
        compiler.column++;
    }
    str8_advance(&compiler.stream);
}


b32 compile_parse() {
    bool failed = 0;
    for_array(compiler.MathObject_tokens){ 
        compiler.curt = compiler.tokens + *it + 1;
        if(compiler.curt->type != Token_Word) {
            ErrorMessage("expected a name for top level definition.");
            failed = 1;
            continue;
        }
        
        auto [symbol, found] = symbol_table_add(compiler.curt->raw, SymbolType_MathObject);
        if(found){
            ErrorMessage("symbol '", compiler.curt->raw, "' is already defined on line ", symbol->line, " in column ", symbol->column);
            failed = 1;
            continue;
        }
        symbol->mathobj = make_math_object();
        symbol->line = compiler.curt->line;
        symbol->column = compiler.curt->column;
        compiler.current_mathobj = symbol->mathobj;
        compiler.curt++;
        while(compiler.curt->type == Token_Comma) {
            compiler.curt++;
            auto [symbol, found] = symbol_table_add(compiler.curt->raw, SymbolType_MathObject);
            if(found){
                ErrorMessage("symbol '", compiler.curt->raw, "' is already defined on line ", symbol->line, " in column ", symbol->column);
                failed = 1;
                goto subcontinue;    
            }
            symbol->mathobj = compiler.current_mathobj;
            symbol->line = compiler.curt->line;
            symbol->column = compiler.curt->column;
        }

        compiler.curt++;
        if(compiler.curt->type != Token_Keyword_is) {
            ErrorMessage("expected 'is' after name(s) of top level definition");
            failed = 1;
            continue;
        }
        
        compiler.curt++;
        if(compiler.curt->type < Token_Group_MathObj_Types) {
            ErrorMessage("expected a MathObject type after 'is'. Possible types are: ");
            forI(Token_COUNT - Token_Group_MathObj_Types) {
                Log("", tokenStrings[i+Token_Group_MathObj_Types]);
            }
            failed = 1;
            continue;
        }

        switch(compiler.curt->type){
            case Token_Keyword_Function: compiler.current_mathobj->type = MathObject_Function; break;
            default: DebugBreakpoint;
        };
        
        compiler.curt++;
        if(compiler.curt->type != Token_OpenBrace) {
            ErrorMessage("expected a '{' after MathObject type in definition of ", symbol->name);
            failed = 1;
            continue;
        }

        *array_push(compiler.returns) = compiler.curt - compiler.tokens;
        *array_push(compiler.return_mathobj) = compiler.current_mathobj;
subcontinue:;
    }


    return !failed;
}

b32 compile_lex() {
#define charcase(c, type_)                  \
case c: {                                   \
    Token* t = array_push(compiler.tokens); \
    t->line = compiler.line;                \
    t->column = compiler.column;            \
    t->raw = str8{compiler.stream.str, 1};  \
    t->type = type_;                        \
    advance_buffer();                       \
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

            case '"' :{
                u32 ls = compiler.line, cs = compiler.column;
                advance_buffer();
                str8 start = compiler.stream;
                while(compiler.stream){
                    advance_buffer();
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
                advance_buffer();
                eat_whitespace();
            }break;

            case '/': {
                
                // possibly a comment
                if(*(compiler.stream.str+1) == '/') {
                    while(*compiler.stream.str != '\n') advance_buffer();
                    advance_buffer();
                } else if (*(compiler.stream.str+1) == '*') {
                    while(!(*compiler.stream.str == '*' && *(compiler.stream.str+1) == '/')) advance_buffer();
                    advance_buffer();
                    advance_buffer();
                }
                eat_whitespace();
            }break;

            default: {
                str8 start = compiler.stream;
                while(compiler.stream) {
                    if(is_whitespace(*compiler.stream.str) || 
                        match_any(*compiler.stream.str, 
                        ':', '{', '}', '(', ')', ';', '[', ']', '"', ',', '.')) {
                        break;
                    }
                    advance_buffer();
                }
                Token* t = array_push(compiler.tokens);
                t->raw = {start.str, compiler.stream.str - start.str};
                t->line = compiler.line;
                t->column = compiler.column;
                t->type = word_or_keyword(t->raw);
                if(t->type == Token_Keyword_MathObject){
                    *array_push(compiler.MathObject_tokens) = array_count(compiler.tokens) - 1;
                }
                eat_whitespace();

            }break;
        }
    }
    return 1;
}

b32 compile_math_objects(str8 path) {
    compiler.line = compiler.column = 1;
    compiler.file = file_init(path, FileAccess_Write);
    compiler.buffer = file_read_alloc(compiler.file, compiler.file->bytes, deshi_allocator);
    defer{memzfree(compiler.buffer.str);};

    compiler.stream = compiler.buffer;

    array_init(compiler.symbol_table, 4, deshi_allocator);
    array_init(compiler.tokens, 4, deshi_allocator);
    array_init(compiler.MathObject_tokens, 4, deshi_allocator);
    array_init(compiler.returns, 4, deshi_allocator); // TODO(sushi) since we can count how many mathobj there will be before we need these we can preallocate them with exact amounts.
    array_init(compiler.return_mathobj, 4, deshi_allocator); 
    defer{
        array_deinit(compiler.symbol_table);
        array_deinit(compiler.tokens);
        array_deinit(compiler.MathObject_tokens);
        array_deinit(compiler.returns);
        array_deinit(compiler.return_mathobj);
    };

    if(!compile_lex()) return 0;
    forX_array(token, compiler.tokens) {
        Log("", tokenStrings[token->type], " ", token->raw);
    }

    compiler.curt = compiler.tokens;
    if(!compile_parse()) return 0;

    return 1;
}

#undef ErrorMessage