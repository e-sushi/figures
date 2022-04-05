/* Index:
@parse
@parse_whitespace
@parse_literal
@parse_letter
@parse_operator
@old_ast
@old_ast_insert_literal
@old_ast_insert_operator
@old_ast_insert_letter
@old_ast_delete_operator
@old_ast_delete_literal
*/

#define PRINT_AST true
#if PRINT_AST
local s32 debug_print_indent = -1;
void debug_print_term(Term* term){
	debug_print_indent++;
	string indent(deshi_temp_allocator); forI(debug_print_indent) indent += "  ";
	char* arg = (HasFlag(term->flags, TermFlag_OpArgLeft)) ? "  L"
		: (HasFlag(term->flags, TermFlag_OpArgRight) ) ? "  R"
		: (HasFlag(term->flags, TermFlag_OpArgTop)   ) ? "  T"
		: (HasFlag(term->flags, TermFlag_OpArgBottom)) ? "  B"
		: "   ";
	
	switch(term->type){
		case TermType_Expression:{
			Expression* expr = ExpressionFromTerm(term);
			Log("ast",expr->raw);
			for_node(term->first_child) debug_print_term(it);
			if(expr->valid){
				if(expr->equals){
					Log("ast", indent, to_string(expr->solution, true, deshi_temp_allocator), arg);
				}else if(expr->solution != MAX_F32){
					Log("ast", indent, stringf(deshi_temp_allocator, "=%g", expr->solution), arg);
				}
			}
			Log("ast","---------------------------------");
		}break;
		
		case TermType_Operator:{
			switch(term->op_type){
				case OpType_Parentheses:           { Log("ast", indent, "()", arg); }break;
				case OpType_Exponential:           { Log("ast", indent, "^",  arg); }break;
				case OpType_Negation:              { Log("ast", indent, "-",  arg); }break;
				case OpType_ExplicitMultiplication:{ Log("ast", indent, "*",  arg); }break;
				case OpType_Division:              { Log("ast", indent, "/",  arg); }break;
				case OpType_Addition:              { Log("ast", indent, "+",  arg); }break;
				case OpType_Subtraction:           { Log("ast", indent, "-",  arg); }break;
				case OpType_ExpressionEquals:      { Log("ast", indent, "=",  arg); }break;
			}
			for_node(term->first_child) debug_print_term(it);
		}break;
		
		case TermType_Literal:{
			Log("ast", indent, term->raw, arg);
		}break;
		
		//case TermType_Variable:{}break;
		//case TermType_FunctionCall:{}break;
	}
	
	debug_print_indent--;
}
#else
#  define debug_print_term(term) (void)0
#endif


//~///////////////////////////////////////////////////////////////////////////////////////////////
//// @parse
b32 parse(Expression* expr){
	b32 valid = true;
	expr->term = Term{TermType_Expression};
	expr->terms.clear();
	
	cstring stream{expr->raw.str, expr->raw.count};
	Term* cursor = &expr->term;
	while(stream && *stream != '\0'){
		char* token_start = stream.str;
#define TOKEN_OFFSET (token_start - expr->raw.str)
#define TOKEN_LENGTH (stream.str - token_start)
		
		switch(*stream){
			//-/////////////////////////////////////////////////////////////////////////////////////////////
			//// @parse_whitespace
			case ' ': case '\n': case '\r': case '\t': case '\v':{
				while(isspace(*stream)){ stream++; } //skip to non-whitespace
			}continue; //skip token creation
			
			//-/////////////////////////////////////////////////////////////////////////////////////////////
			//// @parse_literal
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '.':{
				f64 value = MAX_F64;
				
				while(isdigit(*stream) || *stream == '_'){ stream++; } //skip to non-digit (excluding underscore)
				if(*stream == '.' || *stream == 'e' || *stream == 'E'){ //TODO handle . and e
					stream++;
					while(isdigit(*stream)){ stream++; } //skip to non-digit
					value = stod(cstring{token_start, upt(TOKEN_LENGTH)});
				}else if(*stream == 'x' || *stream == 'X'){
					stream++;
					while(isxdigit(*stream)){ stream++; } //skip to non-hexdigit
					value = f64(b16tou64(cstring{token_start, upt(TOKEN_LENGTH)}));
				}else{
					value = f64(b10tou64(cstring{token_start, upt(TOKEN_LENGTH)}));
				}
				
				expr->terms.add(Term{});
				Term* term = &expr->terms[expr->terms.count-1];
				term->type      = TermType_Literal;
				term->raw.str   = token_start;
				term->raw.count = TOKEN_LENGTH;
				term->lit_value = value;
				
				if      (cursor->type == TermType_Expression){
					insert_last(&expr->term, term);
				}else if(cursor->type == TermType_Operator){
					insert_last(cursor, term);
					term->flags = TermFlag_OpArgRight;
				}else{
					valid = false;
				}
				
				cursor = term;
			}break;
			
			//-/////////////////////////////////////////////////////////////////////////////////////////////
			//// @parse_letter
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
            case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
            case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
            case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
            case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
            case '_':{
				//TODO variables and functions
			}break;
			
			//-/////////////////////////////////////////////////////////////////////////////////////////////
			//// @parse_operator
			case '(':{
				stream++;
				
				expr->terms.add(Term{});
				Term* term = &expr->terms[expr->terms.count-1];
				term->type      = TermType_Operator;
				term->raw.str   = token_start;
				term->raw.count = TOKEN_LENGTH;
				term->op_type   = OpType_Parentheses;
				
				if      (cursor->type == TermType_Expression){
					insert_last(&expr->term, term);
				}else if(cursor->type == TermType_Operator){
					if(cursor->op_type == OpType_Parentheses){
						//TODO implicit multiplication
						valid = false;
						insert_last(cursor->parent, term);
					}else{
						insert_last(cursor, term);
						term->flags = TermFlag_OpArgRight;
					}
				}else if(cursor->type == TermType_Literal){
					//TODO implicit multiplication
					valid = false;
					insert_last(cursor->parent, term);
				}else{
					valid = false;
					insert_last(cursor->parent, term);
				}
				
				cursor = term;
			}break;
			case ')':{
				stream++;
				
				//find last parenthesis op and make it the cursor
				for(s32 i = expr->terms.count-1; i >= 0; --i){
					if(   expr->terms[i].type == TermType_Operator
					   && expr->terms[i].op_type == OpType_Parentheses
					   && !HasFlag(expr->terms[i].flags, TermFlag_LeftParenHasMatchingRightParen)){
						cursor = expr->terms.data + i;
						break;
					}
				}
				
				if(cursor->type == TermType_Operator && cursor->op_type == OpType_Parentheses){
					cursor->raw.count = stream.str - cursor->raw.str - 1;
					AddFlag(cursor->flags, TermFlag_LeftParenHasMatchingRightParen);
				}else{
					//parenthesis was not found
					valid = false;
					AddFlag(cursor->flags, TermFlag_DanglingClosingParenToRight);
				}
			}break;
			
			case '^':{
				stream++;
				
				expr->terms.add(Term{});
				Term* term = &expr->terms[expr->terms.count-1];
				term->type      = TermType_Operator;
				term->raw.str   = token_start;
				term->raw.count = TOKEN_LENGTH;
				term->op_type   = OpType_Exponential;
				
				if      (cursor->type == TermType_Expression){
					insert_last(&expr->term, term);
				}else if(cursor->type == TermType_Literal || (cursor->type == TermType_Operator && cursor->op_type == OpType_Parentheses)){
					//loop until we find a lower precedence operator, then insert op below it
					Term* lower = cursor;
					while(   lower->parent->type == TermType_Operator
						  && OpIsLesser(term, lower->parent) //NOTE right-to-left precedence
						  && lower->parent->op_type != OpType_Parentheses){
						lower = lower->parent;
					}
					term->flags = (lower->flags & OPARG_MASK);
					insert_last(lower->parent, term);
					change_parent_insert_first(term, lower);
					ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
				}else{
					valid = false;
					insert_last(&expr->term, term);
				}
				cursor = term;
			}break;
			
			case '*':{
				stream++;
				
				expr->terms.add(Term{});
				Term* term = &expr->terms[expr->terms.count-1];
				term->type      = TermType_Operator;
				term->raw.str   = token_start;
				term->raw.count = TOKEN_LENGTH;
				term->op_type   = OpType_ExplicitMultiplication;
				
				if(cursor->type == TermType_Literal || (cursor->type == TermType_Operator && cursor->op_type == OpType_Parentheses)){
					//loop until we find a lower precedence operator, then insert op below it
					Term* lower = cursor;
					while(   lower->parent->type == TermType_Operator
						  && OpIsLesserEqual(term, lower->parent)
						  && lower->parent->op_type != OpType_Parentheses){
						lower = lower->parent;
					}
					term->flags = (lower->flags & OPARG_MASK);
					insert_last(lower->parent, term);
					change_parent_insert_first(term, lower);
					ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
				}else{
					valid = false;
					insert_last(&expr->term, term);
				}
				cursor = term;
			}break;
			case '/':{
				stream++;
				
				expr->terms.add(Term{});
				Term* term = &expr->terms[expr->terms.count-1];
				term->type      = TermType_Operator;
				term->raw.str   = token_start;
				term->raw.count = TOKEN_LENGTH;
				term->op_type   = OpType_Division;
				
				if(cursor->type == TermType_Literal || (cursor->type == TermType_Operator && cursor->op_type == OpType_Parentheses)){
					//loop until we find a lower precedence operator, then insert op below it
					Term* lower = cursor;
					while(   lower->parent->type == TermType_Operator
						  && OpIsLesserEqual(term, lower->parent)
						  && lower->parent->op_type != OpType_Parentheses){
						lower = lower->parent;
					}
					term->flags = (lower->flags & OPARG_MASK);
					insert_last(lower->parent, term);
					change_parent_insert_first(term, lower);
					ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
				}else{
					valid = false;
					insert_last(&expr->term, term);
				}
				cursor = term;
			}break;
			
			case '+':{
				stream++;
				
				expr->terms.add(Term{});
				Term* term = &expr->terms[expr->terms.count-1];
				term->type      = TermType_Operator;
				term->raw.str   = token_start;
				term->raw.count = TOKEN_LENGTH;
				term->op_type   = OpType_Addition;
				
				if(cursor->type == TermType_Literal || (cursor->type == TermType_Operator && cursor->op_type == OpType_Parentheses)){
					//loop until we find a lower precedence operator, then insert op below it
					Term* lower = cursor;
					while(   lower->parent->type == TermType_Operator
						  && OpIsLesserEqual(term, lower->parent)
						  && lower->parent->op_type != OpType_Parentheses){
						lower = lower->parent;
					}
					term->flags = (lower->flags & OPARG_MASK);
					insert_last(lower->parent, term);
					change_parent_insert_first(term, lower);
					ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
				}else{
					valid = false;
					insert_last(&expr->term, term);
				}
				cursor = term;
			}break;
			case '-':{
				stream++;
				
				expr->terms.add(Term{});
				Term* term = &expr->terms[expr->terms.count-1];
				term->type      = TermType_Operator;
				term->raw.str   = token_start;
				term->raw.count = TOKEN_LENGTH;
				
				if      (cursor->type == TermType_Expression){
					term->op_type = OpType_Negation;
					
					insert_last(&expr->term, term);
				}else if(cursor->type == TermType_Literal || (cursor->type == TermType_Operator && cursor->op_type == OpType_Parentheses)){
					term->op_type = OpType_Subtraction;
					
					//loop until we find a lower precedence operator, then insert op below it
					Term* lower = cursor;
					while(   lower->parent->type == TermType_Operator
						  && OpIsLesserEqual(term, lower->parent)
						  && lower->parent->op_type != OpType_Parentheses){
						lower = lower->parent;
					}
					term->flags = (lower->flags & OPARG_MASK);
					insert_last(lower->parent, term);
					change_parent_insert_first(term, lower);
					ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
				}else if(cursor->type == TermType_Operator){
					term->op_type = OpType_Negation;
					
					insert_last(cursor, term);
					term->flags = TermFlag_OpArgRight;
				}else{
					valid = false;
					insert_last(&expr->term, term);
				}
				cursor = term;
			}break;
			
			case '=':{
				stream++;
				
				expr->terms.add(Term{});
				Term* term = &expr->terms[expr->terms.count-1];
				term->type      = TermType_Operator;
				term->raw.str   = token_start;
				term->raw.count = TOKEN_LENGTH;
				term->op_type   = OpType_ExpressionEquals;
				
				if(cursor->type == TermType_Literal || (cursor->type == TermType_Operator && cursor->op_type == OpType_Parentheses)){
					//loop until we find a lower precedence operator, then insert op below it
					Term* lower = cursor;
					while(   lower->parent->type == TermType_Operator
						  && OpIsLesserEqual(term, lower->parent)
						  && lower->parent->op_type != OpType_Parentheses){
						lower = lower->parent;
					}
					term->flags = (lower->flags & OPARG_MASK);
					insert_last(lower->parent, term);
					change_parent_insert_first(term, lower);
					ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
				}else{
					valid = false;
					insert_last(&expr->term, term);
				}
				cursor = term;
				expr->equals = term;
			}break;
			
			//-/////////////////////////////////////////////////////////////////////////////////////////////
			//// @parse_error
			default:{
				LogE("lexer", "Unhandled token '",*stream,"' at (",TOKEN_OFFSET,").");
				valid = false;
			}break;
		}
#undef TOKEN_OFFSET
#undef TOKEN_LENGTH
	}
	
	//check if the AST is valid
	if(!valid) return false;
	if(expr->term.child_count == 0 || expr->term.child_count > 1) return false;
	if(expr->term.first_child->type == TermType_Literal) return false;
	forE(expr->terms){
		switch(it->type){
			case TermType_Operator:{
				switch(it->op_type){
					case OpType_Parentheses:{
						if(it->child_count != 1) return false;
						if(it->parent->type == TermType_Expression && it->first_child->type == TermType_Literal) return false;
						if(!HasFlag(it->flags, TermFlag_LeftParenHasMatchingRightParen)) return false;
					}break;
					
					case OpType_Negation:{
						if(it->child_count != 1) return false;
						if(it->parent->type == TermType_Expression && it->first_child->type == TermType_Literal) return false;
					}break;
					
					case OpType_Exponential:
					case OpType_ExplicitMultiplication:
					case OpType_Division:
					case OpType_Addition:
					case OpType_Subtraction:{
						if(it->child_count != 2) return false;
					}break;
					
					case OpType_ExpressionEquals:{
						if(it->child_count == 0) return false;
					}break;
				}
			}break;
		}
	}
	
	return true;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @old_ast_input
/*struct Literal{
Term term;
f64 value;
u32 decimal; //digits since decimal
u32 zeros;   //zeros at the end of input
};*/

/*case TermType_Literal:{
Literal* lit = LiteralFromTerm(term);
string lit_str = stringf(deshi_temp_allocator, "%.*f", (lit->decimal) ? lit->decimal-1 : 0, lit->value);

//add a space between different literals
if(term->left && term->left->type == TermType_Literal){
UI::Text(" ", UITextFlags_NoWrap); UI::SameLine();
}

//draw the literal string
UI::Text(lit_str, UITextFlags_NoWrap); UI::SameLine();

//draw the decimal when a whole number with decimal inputs (since printf truncates)
if(lit->decimal == 1){ //TODO decimal config here
UI::Text(".", UITextFlags_NoWrap); UI::SameLine();
}
}break;*/

//#define SWITCH_START(x) switch(x){
//#define SWITCH_END() }

/*//return false if the AST of the expression is invalid, true otherwise
b32 expression_is_valid(Expression* expr){
	Term* term = &expr->term;
	while(term){
		switch(term->type){
			case TermType_Expression:{
				if(term->right == 0  || term->right->type == TermType_Operator || term->child_count > 1){
					//TODO unary operators
					//TODO nested expressions
					return false;
				}
				if(term->child_count == 1 && term->first_child->type == TermType_Literal) return false;
			}break;
			case TermType_Operator:{
				switch(term->op_type){
					//// two arg operators ////
					case OpType_ExplicitMultiplication:
					case OpType_Division:
					case OpType_Addition:
					case OpType_Subtraction:
					{
						if(term->child_count != 2) return false;
						if(term->left == 0  || term->left->type  != TermType_Literal) return false;
						if(term->right == 0 || term->right->type != TermType_Literal) return false;
					}break;
					
					//// special operators ////
					case OpType_ExpressionEquals:{
						//TODO right side of equals
						//NOTE case TermType_Expression above handles when = is first term
						if(term->left == 0) return false;
					}break;
				}
			}break;
		}
		term = term->right;
	}
	return true;
}*/

/*void attach_at_operators(Term* left_op, Term* right_op){
Term* old_parent = 0;
while(left_op && right_op){
if(left_op == right_op) break;
if(*OperatorFromTerm(left_op) >= *OperatorFromTerm(right_op)){
while(left_op->parent && left_op->parent->type == TermType_Operator && *OperatorFromTerm(left_op->parent) >= *OperatorFromTerm(right_op)) left_op = left_op->parent;
if(left_op == right_op) break;
old_parent = left_op->parent;
change_parent_insert_first(right_op, left_op);
ReplaceOpArgs(left_op->flags, TermFlag_OpArgLeft);
left_op = (old_parent && old_parent->type == TermType_Operator) ? old_parent : 0;
}else{
while(right_op->parent && right_op->parent->type == TermType_Operator && *OperatorFromTerm(right_op->parent) > *OperatorFromTerm(left_op)) right_op = right_op->parent;
if(left_op == right_op) break;
old_parent = right_op->parent;
change_parent_insert_last(left_op, right_op);
ReplaceOpArgs(right_op->flags, TermFlag_OpArgRight);
right_op = (old_parent && old_parent->type == TermType_Operator) ? old_parent : 0;
}
}
}*/

/*//NOTE cursor will be on the left side
void split_at_operator(Expression* expr, Term* cursor){
u32 split = cursor->linear;
Term* left_parent = 0;
while(cursor->type == TermType_Operator){
if(cursor->linear <= split){
Term* old_parent = cursor->parent;
change_parent_insert_first(&expr->term, cursor);
RemoveOpArgs(cursor->flags);

if(cursor->child_count == 2){
change_parent_insert_first(old_parent, cursor->last_child);
ReplaceOpArgs(old_parent->first_child->flags, TermFlag_OpArgLeft);
}

if(left_parent){
change_parent_insert_last(cursor, left_parent);
ReplaceOpArgs(left_parent->flags, TermFlag_OpArgRight);
}
left_parent = cursor;

cursor = old_parent;
}else{
cursor = cursor->parent;
}
}
}*/

/*Operator* make_operator(OpType type, Expression* expr){
Operator* op = (Operator*)memory_alloc(sizeof(Operator)); //TODO expression arena
op->type = type;
op->term.type = TermType_Operator;

if(expr->cursor->type == TermType_Expression){
insert_first(&expr->term, &op->term);

if(expr->cursor->right){
if(expr->cursor->right->type == TermType_Literal){
if(expr->cursor->right->parent->type == TermType_Operator){
if(*op >= *OperatorFromTerm(expr->cursor->right->parent)){
//if op is greater/equal to right op, op steals right literal, then attach the sides
Term* stolen_lit_old_parent = expr->cursor->right->parent;
change_parent_insert_last(&op->term, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgRight);

attach_at_operators(&op->term, stolen_lit_old_parent);
}else{
//if op is less than right op, simply attach the sides
attach_at_operators(&op->term, expr->cursor->right->parent);
}
}else{
//if right's parent is not an operator, right is dangling, so just make it the right child
change_parent_insert_last(&op->term, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgRight);
}
}else if(expr->cursor->right->type == TermType_Operator){
attach_at_operators(&op->term, expr->cursor->right);
}else{
NotImplemented;
}
}
}
else if(expr->cursor->type == TermType_Operator){
if(expr->cursor->right){
if(expr->cursor->right->type == TermType_Literal){
if(expr->cursor->right->right && expr->cursor->right->right->type == TermType_Operator){
//split the sides of expression based on cursor
split_at_operator(expr, expr->cursor);

//insert op below a lower precedence operator on left side
if(*op <= *OperatorFromTerm(expr->cursor)){
Term* lower = expr->cursor;
while((lower->parent->type == TermType_Operator) && (*op <= *OperatorFromTerm(lower->parent))) lower = lower->parent;
op->term.flags = (lower->flags & OPARG_MASK);
insert_last(lower->parent, &op->term);
change_parent_insert_first(&op->term, lower);
ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
}else{
change_parent_insert_last(expr->cursor, &op->term);
ReplaceOpArgs(op->term.flags, TermFlag_OpArgRight);
}

//give the literal to op or right op, whichever has greater precedence
if(*op >= *OperatorFromTerm(expr->cursor->right->right)){
change_parent_insert_last(&op->term, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgRight);
}else{
change_parent_insert_first(expr->cursor->right->right, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgLeft);
}

//reattach the sides
attach_at_operators(&op->term, expr->cursor->right->right);
}else{
//if right right is not an operator, right lit is child of left op, so just make it the right child of op
change_parent_insert_last(&op->term, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgRight);

//insert op below a lower precedence operator on left side
if(*op <= *OperatorFromTerm(expr->cursor)){
Term* lower = expr->cursor;
while((lower->parent->type == TermType_Operator) && (*op <= *OperatorFromTerm(lower->parent))) lower = lower->parent;
op->term.flags = (lower->flags & OPARG_MASK);
insert_last(lower->parent, &op->term);
change_parent_insert_first(&op->term, lower);
ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
}else{
change_parent_insert_last(expr->cursor, &op->term);
ReplaceOpArgs(op->term.flags, TermFlag_OpArgRight);
}
}
}else if(expr->cursor->right->type == TermType_Operator){
//split the sides of expression based on cursor
split_at_operator(expr, expr->cursor);

//insert op below a lower precedence operator on left side
if(*op <= *OperatorFromTerm(expr->cursor)){
Term* lower = expr->cursor;
while((lower->parent->type == TermType_Operator) && (*op <= *OperatorFromTerm(lower->parent))) lower = lower->parent;
op->term.flags = (lower->flags & OPARG_MASK);
insert_last(lower->parent, &op->term);
change_parent_insert_first(&op->term, lower);
ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
}else{
change_parent_insert_last(expr->cursor, &op->term);
ReplaceOpArgs(op->term.flags, TermFlag_OpArgRight);
}

//reattach the sides
attach_at_operators(&op->term, expr->cursor->right);
}else{
NotImplemented;
}
}else{
//insert op below a lower precedence operator on left side
if(*op <= *OperatorFromTerm(expr->cursor)){
Term* lower = expr->cursor;
while((lower->parent->type == TermType_Operator) && (*op <= *OperatorFromTerm(lower->parent))) lower = lower->parent;
op->term.flags = (lower->flags & OPARG_MASK);
insert_last(lower->parent, &op->term);
change_parent_insert_first(&op->term, lower);
ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
}else{
change_parent_insert_last(expr->cursor, &op->term);
ReplaceOpArgs(op->term.flags, TermFlag_OpArgRight);
}
}
}
else if(expr->cursor->type == TermType_Literal){
if(expr->cursor->right){
if(expr->cursor->right->type == TermType_Literal){
//if left and right are literals, the expression is split; so append op on left side, then attach the sides
//insert op below a lower precedence operator
Term* lower = expr->cursor;
while((lower->parent->type == TermType_Operator) && (*op <= *OperatorFromTerm(lower->parent))) lower = lower->parent;
op->term.flags = (lower->flags & OPARG_MASK);
insert_last(lower->parent, &op->term);
change_parent_insert_first(&op->term, lower);
ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);

if(expr->cursor->right->parent->type == TermType_Operator){
if(*op >= *OperatorFromTerm(expr->cursor->right->parent)){
//if op is greater/equal to right op, op steals right literal, then attach the sides
Term* stolen_lit_old_parent = expr->cursor->right->parent;
change_parent_insert_last(&op->term, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgRight);

attach_at_operators(&op->term, stolen_lit_old_parent);
}else{
//if op is less than right op, simply attach the sides
attach_at_operators(&op->term, expr->cursor->right->parent);
}
}else{
//if right's parent is not an operator, right is dangling, so just make it the right child
change_parent_insert_last(&op->term, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgRight);
}
}else if(expr->cursor->right->type == TermType_Operator){
if(expr->cursor->left->type == TermType_Operator){
//if left is a nested lit and right is an op, the expression is connected; steal left lit for left op or op, split the expression at cursor, append op to left side, then attach the sides
//split the sides of expression based on cursor
split_at_operator(expr, expr->cursor->left);

if(*OperatorFromTerm(expr->cursor->left) >= *op){
change_parent_insert_last(expr->cursor->left, expr->cursor);
ReplaceOpArgs(expr->cursor->flags, TermFlag_OpArgRight);
}else{
change_parent_insert_first(&op->term, expr->cursor);
ReplaceOpArgs(expr->cursor->flags, TermFlag_OpArgLeft);
}

//insert op below a lower precedence operator on left side
if(*op <= *OperatorFromTerm(expr->cursor->left)){
Term* lower = expr->cursor->left;
while((lower->parent->type == TermType_Operator) && (*op <= *OperatorFromTerm(lower->parent))) lower = lower->parent;
op->term.flags = (lower->flags & OPARG_MASK);
insert_last(lower->parent, &op->term);
change_parent_insert_first(&op->term, lower);
ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
}else{
change_parent_insert_last(expr->cursor->left, &op->term);
ReplaceOpArgs(op->term.flags, TermFlag_OpArgRight);
}

attach_at_operators(&op->term, expr->cursor->right);
}else{
//if left is a dangling lit and right is an op, change left lit parent to op, attach the expressions
change_parent_insert_first(&op->term, expr->cursor);
ReplaceOpArgs(expr->cursor->flags, TermFlag_OpArgLeft);

insert_first(&expr->term, &op->term);

attach_at_operators(&op->term, expr->cursor->right);
}
}else{
NotImplemented;
}
}else{
//if right edge, loop until we find a lower precedence operator, then insert op below it
Term* lower = expr->cursor;
while((lower->parent->type == TermType_Operator) && (*op <= *OperatorFromTerm(lower->parent))) lower = lower->parent;
op->term.flags = (lower->flags & OPARG_MASK);
insert_last(lower->parent, &op->term);
change_parent_insert_first(&op->term, lower);
ReplaceOpArgs(lower->flags, TermFlag_OpArgLeft);
}
}else{
NotImplemented;
}

insert_right(expr->cursor, &op->term);
return op;
}*/

/*//TODO support Unicode using iswdigit()/iswalpha() once we handle it in DeshInput->charIn
//TODO remove duplication
b32 term_insertion(Expression* expr, char input){
b32 ast_changed = false;
input = tolower(input);
SWITCH_START(input);

//-///////////////////////////////////////////////////////////////////////////////////////////////
// @old_ast_insert_literal
case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':{
if(expr->term.child_count == 0){
ast_changed = true;
Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
lit->term.type   = TermType_Literal;
insert_first(&expr->term, &lit->term);
insert_right(&expr->term, &lit->term);
expr->cursor = &lit->term;
}

if(expr->cursor->type == TermType_Literal){ //appending to a literal
ast_changed = true;
Literal* lit = LiteralFromTerm(expr->cursor);
if(lit->decimal){ //we are appending as decimal values
lit->zeros = (input == '0') ? lit->zeros + 1 : 0;
lit->value = lit->value + (input-48)/pow(10,lit->decimal+lit->zeros);
lit->decimal++;
}else{            //we are appending as integer values
lit->value = 10*lit->value + (input-48);
}
}else if(expr->cursor->type == TermType_Operator){ //right side of operator //TODO non-binary/non-linear operators
ast_changed = true;
Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
lit->value = f32(input-48);

lit->term.type = TermType_Literal;
if(expr->cursor->right){
if(expr->cursor->right->type == TermType_Operator){
//if in between operators, become child of higher precedence one, the AST shouldn't need to change
if(*OperatorFromTerm(expr->cursor) >= *OperatorFromTerm(expr->cursor->right)){
lit->term.flags = TermFlag_OpArgRight;
insert_last(expr->cursor, &lit->term);
}else{
lit->term.flags = TermFlag_OpArgLeft;
insert_first(expr->cursor->right, &lit->term);
}
}else if(expr->cursor->right->type == TermType_Literal){
//if there was already a right literal of the operator, give it to a right operator and split the expression,
//or make it dangling if there is no right operator
if(expr->cursor->right->right && expr->cursor->right->right->type == TermType_Operator){
change_parent_insert_first(expr->cursor->right->right, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgLeft);

split_at_operator(expr, expr->cursor);
}else{
change_parent_insert_last(&expr->term, expr->cursor->right);
RemoveOpArgs(expr->cursor->right->flags);
}
lit->term.flags = TermFlag_OpArgRight;
insert_last(expr->cursor, &lit->term);
}
}else{
//if right edge, insert as right child of the left operator
lit->term.flags = TermFlag_OpArgRight;
insert_last(expr->cursor, &lit->term);
}
insert_right(expr->cursor, &lit->term);
expr->cursor = &lit->term;
}else if(expr->cursor->right->type == TermType_Operator){ //left side of operator
//NOTE no need to check if between operators here since the cursor would have been on an operator if there was one on left
ast_changed = true;
Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
lit->value = f32(input-48);

lit->term.type   = TermType_Literal;
lit->term.flags  = TermFlag_OpArgLeft;
insert_first(expr->cursor->right, &lit->term);
insert_right(expr->cursor, &lit->term);
expr->cursor = &lit->term;
}
}break;

case '.':{ //TODO comma in EU input mode
if(expr->term.child_count == 0){
ast_changed = true;
Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
lit->term.type   = TermType_Literal;
insert_first(&expr->term, &lit->term);
insert_right(&expr->term, &lit->term);
expr->cursor = &lit->term;
}

if(expr->cursor->type == TermType_Literal){
ast_changed = true;
Literal* lit = LiteralFromTerm(expr->cursor);
if(lit->decimal == 0) lit->decimal = 1;
}else if(expr->cursor->type == TermType_Operator){ //right side of operator //TODO non-binary/non-linear operators
ast_changed = true;
Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
lit->decimal = 1;

lit->term.type = TermType_Literal;
if(expr->cursor->right){
if(expr->cursor->right->type == TermType_Operator){
//if in between operators, become child of higher precedence one, the AST shouldn't need to change
if(*OperatorFromTerm(expr->cursor) >= *OperatorFromTerm(expr->cursor->right)){
lit->term.flags = TermFlag_OpArgRight;
insert_last(expr->cursor, &lit->term);
}else{
lit->term.flags = TermFlag_OpArgLeft;
insert_first(expr->cursor->right, &lit->term);
}
}else if(expr->cursor->right->type == TermType_Literal){
//if there was already a right literal of the operator, give it to a right operator and split the expression,
//or make it dangling if there is no right operator
if(expr->cursor->right->right && expr->cursor->right->right->type == TermType_Operator){
change_parent_insert_first(expr->cursor->right->right, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgLeft);

split_at_operator(expr, expr->cursor);
}else{
change_parent_insert_last(&expr->term, expr->cursor->right);
RemoveOpArgs(expr->cursor->right->flags);
}
lit->term.flags = TermFlag_OpArgRight;
insert_last(expr->cursor, &lit->term);
}
}else{
//if right edge, insert as right child of the left operator
lit->term.flags = TermFlag_OpArgRight;
insert_last(expr->cursor, &lit->term);
}
insert_right(expr->cursor, &lit->term);
expr->cursor = &lit->term;
}else if(expr->cursor->right->type == TermType_Operator){ //left side of operator
ast_changed = true;
Literal* lit = (Literal*)memory_alloc(sizeof(Literal)); //TODO expression arena
lit->decimal = 1;

lit->term.type   = TermType_Literal;
lit->term.flags  = TermFlag_OpArgLeft;
insert_first(expr->cursor->right, &lit->term);
insert_right(expr->cursor, &lit->term);
expr->cursor = &lit->term;
}
}break;

case 'e':{
//TODO exponential literal input
}break;

//-///////////////////////////////////////////////////////////////////////////////////////////////
// @old_ast_insert_operator
case '*':{
Operator* op = make_operator(OpType_ExplicitMultiplication, expr);
if(op){
ast_changed = true;
expr->cursor = &op->term;
}
}break;

case '/':{
Operator* op = make_operator(OpType_Division, expr);
if(op){
ast_changed = true;
expr->cursor = &op->term;
}
}break;

case '+':{
Operator* op = make_operator(OpType_Addition, expr);
if(op){
ast_changed = true;
expr->cursor = &op->term;
}
}break;

case '-':{
Operator* op = make_operator(OpType_Subtraction, expr);
if(op){
ast_changed = true;
expr->cursor = &op->term;
}
}break;

case '=':{
Operator* op = make_operator(OpType_ExpressionEquals, expr);
if(op){
ast_changed = true;
expr->cursor = &op->term;
expr->equals = &op->term;
}
}break;

//-///////////////////////////////////////////////////////////////////////////////////////////////
// @old_ast_insert_letter ('e' is handled above in ast_insert_literal)
case 'a':case 'b':case 'c':case 'd':         case 'f':case 'g':case 'h':case 'i':case 'j':case 'k':case 'l':case 'm':
case 'n':case 'o':case 'p':case 'q':case 'r':case 's':case 't':case 'u':case 'v':case 'w':case 'x':case 'y':case 'z':{
//TODO variables
}break;
SWITCH_END();

return ast_changed;
}*/

/*//TODO remove duplication
b32 term_deletion(Expression* expr, b32 delete_right){
b32 ast_changed = false;
if(delete_right){
if(expr->cursor->right == 0) return false;
expr->cursor = expr->cursor->right;
}

SWITCH_START(expr->cursor->type);
case TermType_Expression:{
TODO expression deletion
}break;

//-///////////////////////////////////////////////////////////////////////////////////////////////
// @old_ast_delete_operator
case TermType_Operator:{ //TODO non-binary/non-linear operators
ast_changed = true;
Operator* op = OperatorFromTerm(expr->cursor);

if(expr->cursor->right){
b32 extra_operator = false;

//change parents of operator children
if(expr->cursor->child_count){
//if cursor operator is next to another operator, change child's parent to greater precedence operator
if(expr->cursor->left->type == TermType_Operator){
extra_operator = true;
if(expr->cursor->right->right && expr->cursor->right->right->type == TermType_Operator){
if(*OperatorFromTerm(expr->cursor->left) >= *OperatorFromTerm(expr->cursor->right->right)){
change_parent_insert_last(expr->cursor->left, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgRight);
}else{
change_parent_insert_first(expr->cursor->right->right, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgLeft);
}

attach_at_operators(expr->cursor->left, expr->cursor->right->right);
}else if(expr->cursor->right->type != TermType_Operator){ //if left and right are operators, simply remove the operator
change_parent_insert_first(expr->cursor->left, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgLeft);
}
}
if(expr->cursor->right->type == TermType_Operator){
extra_operator = true;
if(expr->cursor->left->left && expr->cursor->left->left->type == TermType_Operator){
if op is greater/equal to right op, op steals right literal, then attach the sides
if(*OperatorFromTerm(expr->cursor->left->left) >= *OperatorFromTerm(expr->cursor->right)){
change_parent_insert_last(expr->cursor->left->left, expr->cursor->left);
ReplaceOpArgs(expr->cursor->left->flags, TermFlag_OpArgRight);
}else{
change_parent_insert_first(expr->cursor->right, expr->cursor->left);
ReplaceOpArgs(expr->cursor->left->flags, TermFlag_OpArgLeft);
}

attach_at_operators(expr->cursor->left->left, expr->cursor->right);
}else if(expr->cursor->left->type != TermType_Operator){ //if left and right are operators, simply remove the operator
change_parent_insert_first(expr->cursor->right, expr->cursor->left);
ReplaceOpArgs(expr->cursor->left->flags, TermFlag_OpArgLeft);
}
}

if(!extra_operator){
//if cursor has a left child, and that lit has a left op, make the lit a child of that op
if(   expr->cursor->left == expr->cursor->first_child
&& expr->cursor->left->left
&& expr->cursor->left->left->type == TermType_Operator
&& *op >= *OperatorFromTerm(expr->cursor->left->left)){ //maybe redundant check of (expr->cursor->left == expr->cursor->first_child) since we dont have ownership of our left if we dont have precedence?
change_parent_insert_last(expr->cursor->left->left, expr->cursor->left);
ReplaceOpArgs(expr->cursor->left->flags, TermFlag_OpArgRight);
}
//if cursor has a right child, and that lit has a right op, make the lit a child of that op
if(   expr->cursor->right == expr->cursor->last_child
&& expr->cursor->right->right 
&& expr->cursor->right->right->type == TermType_Operator 
&& *op >= *OperatorFromTerm(expr->cursor->right->right)){
change_parent_insert_first(expr->cursor->right->right, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgLeft);
}
}
}
debug_print_term(&expr->term, expr->cursor);

//separate sides of expression by getting each sides parentmost and making them children of the expression
if(!extra_operator){
				//left side of cursor if it has one
if(expr->cursor->left){
Term* it = expr->cursor->left;
Term* it_prev = expr->cursor->left;
while(it->parent && it->parent->linear <= expr->cursor->linear){ it_prev = it; it = it->parent; }
if(it != &expr->term){
if(it == expr->cursor) it = it_prev; //NOTE the cursor should not be considered
change_parent_insert_last(&expr->term, it);
RemoveOpArgs(it->flags);
}
}
				//right side of cursor (we know it has one)
Term* it = expr->cursor->right;
Term* it_prev = expr->cursor->right;
while(it->parent && it->parent->linear >= expr->cursor->linear){ it_prev = it; it = it->parent; }
if(it != &expr->term){
if(it == expr->cursor) it = it_prev; //NOTE the cursor should not be considered
change_parent_insert_last(&expr->term, it);
RemoveOpArgs(it->flags);
}
}
debug_print_term(&expr->term, expr->cursor);
}

//remove this operator from AST
if(expr->cursor == expr->cursor->parent->first_child){
while(expr->cursor->first_child){
ReplaceOpArgs(expr->cursor->first_child->flags, TermFlag_OpArgLeft);
change_parent_insert_first(expr->cursor->parent, expr->cursor->first_child);
}
}else{
while(expr->cursor->first_child){
ReplaceOpArgs(expr->cursor->first_child->flags, TermFlag_OpArgRight);
change_parent_insert_last (expr->cursor->parent, expr->cursor->first_child);
}
}
remove_from_parent(expr->cursor);
remove_horizontally(expr->cursor);

expr->cursor = expr->cursor->left;
remove_leftright(expr->cursor->right);
if(&op->term == expr->equals) expr->equals = 0;
expr->term_count--;
memory_zfree(op);
}break;

	//-///////////////////////////////////////////////////////////////////////////////////////////////
// @old_ast_delete_literal
case TermType_Literal:{
ast_changed = true;
Literal* lit = LiteralFromTerm(expr->cursor);

		//if right or left edge, no need to reorganize things
if(expr->cursor->right && expr->cursor->left->type != TermType_Expression){
if(expr->cursor->left->type == TermType_Operator){
if(expr->cursor->right->type == TermType_Literal){ //NOTE if both left and right are operators, deleting this literal has no precedence/AST considerations
if(expr->cursor->right->parent->type == TermType_Operator){ //(detached expression: 1+1 2+2, deleting the right 1)
if(*OperatorFromTerm(expr->cursor->left) >= *OperatorFromTerm(expr->cursor->right->parent)){
							//if left op is greater/equal to right op, left op steals right literal and its parentmost becomes left child of right op
Term* stolen_lit_old_parent = expr->cursor->right->parent; //TODO maybe just expr->cursor->right->right?
change_parent_insert_last(expr->cursor->left, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgRight);
debug_print_term(&expr->term, expr->cursor);

attach_at_operators(expr->cursor->left, stolen_lit_old_parent);
}else{
							//if left op is less than right op, simply attach the sides
attach_at_operators(expr->cursor->left, expr->cursor->right->parent);
}
}else{
						//if right's parent is not an operator, its dangling, so just change replace cursor with it
change_parent_insert_last(expr->cursor->parent, expr->cursor->right);
ReplaceOpArgs(expr->cursor->right->flags, TermFlag_OpArgRight);
}
}
}else{
				//left term is not an operator/expression, so its a literal/var/func (detached expression: 1+1 2+2, deleting the left 2)
if(expr->cursor->right->type == TermType_Operator){ //NOTE if neither left or right are operators, deleting this literal has no precedence/AST considerations
if(expr->cursor->left->parent->type == TermType_Operator){
if(*OperatorFromTerm(expr->cursor->left->parent) >= *OperatorFromTerm(expr->cursor->right)){
							//if left op is greater/equal to right op, simply attach the sides
attach_at_operators(expr->cursor->left->parent, expr->cursor->right);
}else{
							//if left op is less than right op, right op steals this literal, then attach the sides
Term* stolen_lit_old_parent = expr->cursor->left->parent;
change_parent_insert_last(expr->cursor->right, expr->cursor->left);
ReplaceOpArgs(expr->cursor->left->flags, TermFlag_OpArgLeft);
debug_print_term(&expr->term, expr->cursor);

attach_at_operators(stolen_lit_old_parent, expr->cursor->right);
}
}else{
						//if left's parent is not an operator, left is dangling, so just change replace cursor with it
change_parent_insert_first(expr->cursor->parent, expr->cursor->left);
ReplaceOpArgs(expr->cursor->left->flags, TermFlag_OpArgLeft);
}
}
}
}

		//remove this literal from AST
remove_from_parent(expr->cursor);
remove_horizontally(expr->cursor);

expr->cursor = expr->cursor->left;
remove_leftright(expr->cursor->right);
expr->term_count--;
memory_zfree(lit);
}break;
SWITCH_END();

return ast_changed;
}*/

//#undef SWITCH_START
//#undef SWITCH_END

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @old_ast_graphing
/*
#if SUUGU_USE_GRAPHVIZ
Agraph_t* gvgraph = 0;
GVC_t* gvc = 0;
char* gout = 0;
u32 gout_size = 0;

struct gvNode {
	vec2 pos, siz;
	string label;
};

struct gvEdge {
	vec2 pos1, pos2, pos3, pos4;
};

struct gvGraph {
	array<gvNode> nodes;
	array<gvEdge> edges;
	f64 xmax=0,ymax=0;
};

gvGraph graph;
u32 i = 0;
void make_dot_file(TNode* node, Agnode_t* parent) {
	i++;
	u32 save = i;
	
	Agnode_t* me = agnode(gvgraph, toStr(i).str, 1);
	agset(me, "label", node->comment);
	TNode* stage = node;
	for_node(node->first_child){
		make_dot_file(it, me);
	}
	//if (stage->first_child)   make_dot_file(stage->first_child, me);
	//if (stage->next != stage) make_dot_file(stage->next, parent);
	
	if (parent)
		agedge(gvgraph, parent, me, "", 1);
	
}



void Parser::pretty_print(Expression& e) {
	
	if (!gvc) gvc = gvContext();
	if (!gout) gout = (char*)memtalloc(Kilobytes(1)); //arbitrary size, maybe should be changed later
	
	if (e.node.first_child) {
		graph.nodes.clear();
		graph.edges.clear();
		i=0;
		gvgraph = agopen("exp ast tree", Agdirected, 0);
		Agnode_t* prog = agnode(gvgraph, "head", 1);
		for_node(e.node.first_child){
			make_dot_file(it, prog);
		}
		//agattr(gvgraph, AGRAPH, "splines", "line");
		gvLayout(gvc, gvgraph, "dot");
		gvRenderData(gvc, gvgraph, "plain", &gout, &gout_size);
		
		FileReader reader = init_reader(gout, gout_size);
		
		forI(reader.lines.count) {
			next_line(reader);
			if (str_begins_with(reader.read, "node")) {
				gvNode node;
				chunk_line(reader, i, ' ');
				node.label = reader.chunks[6];
				if(node.label.beginsWith("\"")) node.label.erase(0);
				if(node.label.endsWith("\"")) node.label.erase(node.label.count-1);
				node.pos.x = stod(reader.chunks[2]);
				node.pos.y = stod(reader.chunks[3]);
				node.siz.x = stod(reader.chunks[4]);
				node.siz.y = stod(reader.chunks[5]);
				graph.xmax=Max(f64(graph.xmax),f64(node.pos.x+node.siz.x));
				graph.ymax=Max(f64(graph.ymax),f64(node.pos.y+node.siz.y));
				graph.nodes.add(node);
			}
			else if (str_begins_with(reader.read, "edge")){
				gvEdge edge;
				chunk_line(reader, i, ' ');
				edge.pos1.x = stod(reader.chunks[4]);
				edge.pos1.y = stod(reader.chunks[5]);
				edge.pos2.x = stod(reader.chunks[10]);
				edge.pos2.y = stod(reader.chunks[11]);
				graph.edges.add(edge);
				
			}
		}
	}
	else {
		using namespace UI;
		Begin("suuguprettyprint", UIWindowFlags_FitAllElements); {
			static f32 zoom = 17;
			static f32 ppi = 50; //pixels per inch, becasue graphviz's plaintext format returns everything in inches
			if(IsWinHovered()){
				ppi += 10 * DeshInput->scrollY;
			}
			for (gvNode node : graph.nodes) {
				vec2 np = vec2(
							   ppi*(node.pos.x),
							   ppi*(node.pos.y)
							   );
				vec2 ns = vec2(
							   ppi*(node.siz.x),
							   ppi*(node.siz.y)
							   );
				//convert from lowerleft origin to topleft origin
				np.y=ppi*graph.ymax-(np.y+node.siz.y);
				Rect(np, ns);
				f32 cts = CalcTextSize(node.label.str).x;
				f32 ts = ns.x / cts;
				
				PushVar(UIStyleVar_FontHeight, GetStyle().fontHeight*ts);
				Text(node.label.str,  GetLastItem()->position);
				PopVar();
			}
		}End();
	}
	return;
}

#else 
void Parser::pretty_print(Expression& e) {}//LogW("SUUGUDEBUG", "Function 'pretty_print' called from somewhere when BUILD_SLOW not set!");}
#endif
*/