struct VariableMap{
    char* key;
    Variable value;
};

struct UnitMap{
    char* key;
    Unit value;
};

struct{
    struct{ // map of defined constants and units 
        VariableMap* constants;
        UnitMap* units;
    }maps;
	
    struct{ // lists of tags that have been used on constants or units
        str8* constants;
        str8* units;
    }tags;
	
}library;

// loads a suugu library (.slib) into the global library
//TODO(sushi) this is unfinished and what is here needs to be compressed badly.
//            right now it only handles defining a constant with the fields id, value, and symbol
void library_load(str8 filename){
    str8 file = file_read_simple(ToString8(deshi_temp_allocator, "data/", filename), deshi_temp_allocator);
	
#define liberr(line_num, line_col, ...) LogE("", "library error: ", filename, "(", line_num, ",", line_col, "): ", __VA_ARGS__)
	
    str8 stream = file;
	
    u32 line_num = 1;
    u32 line_col = 1;
	
    auto stream_next = [&](){
        if(stream.str[0] == '\r'){      
            line_col = 0;               
            line_num++;                 
            str8_advance(&stream);      
        }else if(stream.str[0] == '\n'){
            line_col = 0;               
            line_num++;                 
        } else line_col++;              
        str8_advance(&stream);         
    };
	
    auto skip_whitespace = [&](){
        while(is_whitespace(decoded_codepoint_from_utf8(stream.str,4).codepoint)){
            stream_next();
        }
    };
	
    auto skip_until_whitespace = [&](){
        while(!is_whitespace(decoded_codepoint_from_utf8(stream.str,4).codepoint)){
            stream_next();
        }
    };
	
    auto eat_until = [&](u32 codepoint){
        u8* save = stream.str;
        while(stream && codepoint != decoded_codepoint_from_utf8(stream.str,4).codepoint){
            stream_next();
        }
        return str8{save, stream.str-save};
    };
	
    while(stream.count){
        if(str8_begins_with(stream, STR8("###"))){
            u32 line_start = line_num;
            u32 col_start = line_col;
            stream_next();
            while(stream.count > 1 && !(stream.str[0] == '#' && stream.str[1] == '#' && stream.str[2] == '#'))
                stream_next();
            if(stream.count <= 1){
                liberr(line_start, col_start, "began a multiline comment (###), but did not end it.");
                return;
            }
            forI(3) stream_next();
        }else if(stream.str[0] == '#'){
            while(stream && stream.str[0] != '\r' && stream.str[0] != '\n'){
                stream_next();
            }
            stream_next();
        }else if(str8_begins_with(stream, STR8("constant"))){
            Variable constant = {0};
			
			//make an expression holding one literal
			constant.expr = make_expression();
			constant.expr->terms.add(Term{});
			Term* literal_term = &constant.expr->terms[constant.expr->terms.count-1];
			literal_term->type = TermType_Literal;
			ast_insert_last(&constant.expr->term, literal_term);
			linear_insert_right(&constant.expr->term, literal_term);
            
            forI(8) stream_next();
            skip_whitespace();
            if(stream.str[0] != '{'){
                liberr(line_num, line_col, "expected a '{' after keyword 'constant");
                return;
            }
            stream_next();
			
            struct{
                b32 id:1;
                b32 symbol:1;
                b32 value:1;
            }checklist = {0};
			
            while(1){ //TODO(sushi) the way this is makes it so that nothing but fields can exist within a definition, so you can't have comments
                skip_whitespace();
                if(str8_begins_with(stream, STR8("id"))){
                    checklist.id = 1;
                    skip_until_whitespace();
                    skip_whitespace();
                    if(stream.str[0] != '='){
                        liberr(line_num, line_col, "expected a '=' after field identifier 'id'");
                        return;
                    }
					
                    stream_next();
                    skip_whitespace();
					
                    if(stream.str[0] == '{'){
                        liberr(line_num, line_col, "lists are not allowed for identifiers");
                    }else if(stream.str[0] ==';'){
                        liberr(line_num, line_col, "missing identifier for id field.");
                    }
					
                    str8 id = stream;
                    while(stream.str[0] != ';' && !is_whitespace(decoded_codepoint_from_utf8(stream.str,4).codepoint)){
                        stream_next();
                    }
                    id.count = stream.str - id.str;
					literal_term->raw = id;
                    u32 line_start = line_num;
                    u32 col_start = line_col;
                    skip_whitespace();
                    if(!stream.count || stream.str[0] != ';'){
                        liberr(line_start, col_start, "expected ';'");
                        return;
                    }
                    stream_next();
                }else if(str8_begins_with(stream, STR8("symbol"))){
                    checklist.symbol = 1;
                    skip_until_whitespace();
                    skip_whitespace();
                    if(stream.str[0] != '='){
                        liberr(line_num, line_col, "expected a '=' after field identifier 'id'");
                        return;
                    }
					
                    stream_next();
                    skip_whitespace();
					
                    if(stream.str[0] ==';'){
                        liberr(line_num, line_col, "missing value for symbol field. A symbol field can be a string or a list of strings.");
                    }
					
                    if(stream.str[0] == '{'){
                        stream_next();
                        while(1){
                            skip_whitespace();
                            if(stream.str[0] != '"'){
                                liberr(line_num, line_col, "expected a string (\"...\") in list for symbol field.");
                                return;
                            }
                            u32 line_start = line_num, col_start = line_col;
                            stream_next();
                            str8 str = eat_until('"');
                            if(!stream){
                                liberr(line_start, col_start, "unterminated string.");
                                return;
                            }
                            arrput(constant.symbols, str);
                            stream_next();
                            skip_whitespace();
                            if(stream.str[0] == '}'){
                                stream_next();
                                break;
                            }else if(stream.str[0] == ','){
                                stream_next();
                            }
                        }
                    }else if(stream.str[0] == '"'){
                        u32 line_start = line_num, col_start = line_col;
                        stream_next();
                        str8 str = eat_until('"');
                        if(!stream){
                            liberr(line_start, col_start, "unterminated string.");
                            return;
                        }
                        arrput(constant.symbols, str);
                        stream_next();
                    }
					
                    skip_whitespace();
                    if(stream.str[0] != ';'){
                        liberr(line_num, line_col, "expected ';'");
                        return;
                    }
                    stream_next();
                }else if(str8_begins_with(stream, STR8("value"))){
                    checklist.value = 1;
                    skip_until_whitespace();
                    skip_whitespace();
                    if(stream.str[0] != '='){
                        liberr(line_num,line_col,"expected '=' after field identifier 'value'");
                        return;
                    }
                    stream_next();
                    skip_whitespace();
                    str8 val = stream;
                    u32 line_start = line_num, col_start = line_col;
                    while(stream && stream.str[0] != ';' && !is_whitespace(utf8codepoint(stream.str))){
                        stream_next();
                    }
                    if(!stream){
                        liberr(line_start, col_start, "unexpected end of file while parsing constant value field.");
                        return;
                    }
                    val.count = stream.str - val.str;
					literal_term->lit_value = stod(val);
                    if(literal_term->lit_value == 0.0){ //NOTE(sushi) for whatever reason, strtod returns 0.0 if no valid conversion could be made, so it's possible someone tries to make a constant with a value 0 and we error here.
                        liberr(line_num, line_col, "invalid number given for value field.");
                        return;
                    }
                    skip_whitespace();
                    if(stream.str[0] != ';'){
                        liberr(line_num,line_col,"expected ';'");
                        return;
                    }
                    stream_next();
                }
            }
        }else{
            stream_next();
        }
    }
	
}