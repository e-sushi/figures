/////////////////////
//// @local vars ////
/////////////////////
//master token
local token curt;
local array<token> tokens;
local u32 layer = 0;
local bool master_logger = true;

local map<TokenType, ExpressionType> binaryOps{
	{tok_Multiplication,     Expression_BinaryOpMultiply},
	{tok_Division,           Expression_BinaryOpDivision},
	{tok_Negation,           Expression_BinaryOpMinus},
	{tok_Plus,               Expression_BinaryOpPlus},
	{tok_AND,                Expression_BinaryOpAND},
	{tok_OR,                 Expression_BinaryOpOR},
	{tok_LessThan,           Expression_BinaryOpLessThan},
	{tok_GreaterThan,        Expression_BinaryOpGreaterThan},
	{tok_LessThanOrEqual,    Expression_BinaryOpLessThanOrEqual},
	{tok_GreaterThanOrEqual, Expression_BinaryOpGreaterThanOrEqual},
	{tok_Equal,              Expression_BinaryOpEqual},
	{tok_NotEqual,           Expression_BinaryOpNotEqual},
	{tok_BitAND,             Expression_BinaryOpBitAND},
	{tok_BitOR,              Expression_BinaryOpBitOR},
	{tok_BitXOR,		     ExpressionGuard_BitXOR},
	{tok_BitShiftLeft,       Expression_BinaryOpBitShiftLeft},
	{tok_BitShiftRight,      Expression_BinaryOpBitShiftRight},
	{tok_Modulo,             Expression_BinaryOpModulo},
};

local map<TokenType, ExpressionType> unaryOps{
	{tok_BitwiseComplement, Expression_UnaryOpBitComp},
	{tok_LogicalNOT,        Expression_UnaryOpLogiNOT},
	{tok_Negation,          Expression_UnaryOpNegate},
};


//////////////////////
//// @local funcs ////
//////////////////////
#define token_next curt = tokens.next()
#define token_last curt = tokens.prev()

#define token_peek tokens.peek()
#define token_look_back(i) tokens.lookback(i)

#define Expect(tok_type)\
if(curt.type == tok_type) {

#define ElseExpect(tok_type)\
} else if(curt.type == tok_type) {

#define ExpectFail(fail_code)\
} else { fail_code }

#define ParseOut(out)\
LOG(out);


#define ExpectOneOf(expect)\
if(expect.has(curt.type)) {

#define PrettyPrint(...)\
if(master_logger){ for(int i = 0; i < layer; i++)\
if(i % 2 == 0) std::cout << "|   ";\
else std::cout << "!   ";\
std::cout << TOSTRING("~", __VA_ARGS__, ":").str << std::endl;}

#define EOICheck 0\
//if(tokens.iter > tokens.last){ return; }

local void parse_term(array<Expression>*expressions);
local void parse_expressions(array<Expression>*expressions);

local void parse_factor(array<Expression>* expressions) {
	layer++;
	
	PrettyPrint("factor");
	EOICheck;
    
	switch (curt.type) {
        
		case tok_Literal: {
			PrettyPrint("literal ", curt.str);
			expressions->add(Expression(curt.str, Expression_Literal));
			expressions->last->literalValue = stod(curt.str); 
			layer--;
			return;
		}break;
        
		case tok_OpenParen: {
			PrettyPrint("OPEN (");
            
			expressions->add(Expression(curt.str, ExpressionGuard_Factor));
			
			token_next;
			parse_expressions(&expressions->last->expressions);
			
			Expect(tok_CloseParen)
				PrettyPrint("CLOSE )");
			layer--;
            return;
			ExpectFail((void)0;)
		}break;
        
		case tok_Identifier: {
			//expressions->add(Expression(curt.str, Expression_Literal));
			return;
		}break;
        
		case tok_Negation:
		case tok_LogicalNOT:
		case tok_BitwiseComplement: {
			PrettyPrint("unaop ", ExpTypeStrings[*binaryOps.at(curt.type)]);
            
			expressions->add(Expression(curt.str, *unaryOps.at(curt.type)));
            
			token_next;
			parse_factor(&expressions->last->expressions);
			layer--;
            
			return;
		}break;
        
        
		//by default if no valid token is found we call it empty
		default: {
			PrettyPrint("empty");
			expressions->add(Expression(curt.str, Expression_Empty));
			layer--;
			return;
		}break;
	}
    
	layer--;
}

local void parse_term(array<Expression>* expressions){
	layer++;
	PrettyPrint("term");
	EOICheck;
    
	expressions->add(Expression(curt.str, ExpressionGuard_Term));
	parse_factor(&expressions->last->expressions);
    
	while (token_peek.type == tok_Multiplication || token_peek.type == tok_Division ||
           token_peek.type == tok_Modulo) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[*binaryOps.at(curt.type)]);
        
		//operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
        
		token_next;
		expressions->add(Expression(curt.str, ExpressionGuard_Term));
		parse_factor(&expressions->last->expressions);
	}
    
	layer--;
}

local void parse_additive(array<Expression>* expressions){
	layer++;
	PrettyPrint("additive");
	EOICheck;
    
	expressions->add(Expression(curt.str, ExpressionGuard_Additive));
	parse_term(&expressions->last->expressions);
    
	while (token_peek.type == tok_Plus || token_peek.type == tok_Negation) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[*binaryOps.at(curt.type)]);
		
		//operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
        
		token_next; //skip over binop token
		expressions->add(Expression(curt.str, ExpressionGuard_Additive));
		parse_term(&expressions->last->expressions);
	}
    
	layer--;
}


local void parse_bitwise_shift(array<Expression>* expressions){
	layer++;
	PrettyPrint("bitshift");
	EOICheck;
    
	expressions->add(Expression(curt.str, ExpressionGuard_BitShift));
	parse_additive(&expressions->last->expressions);
    
	while (token_peek.type == tok_BitShiftRight || token_peek.type == tok_BitShiftLeft) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[*binaryOps.at(curt.type)]);
        
		//operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
        
		token_next;
		expressions->add(Expression(curt.str, ExpressionGuard_BitShift));
		parse_additive(&expressions->last->expressions);
	}
    
	layer--;
}

local void parse_relational(array<Expression>* expressions){
	layer++;
	PrettyPrint("relational");
	EOICheck;
    
	expressions->add(Expression(curt.str, ExpressionGuard_Relational));
	parse_bitwise_shift(&expressions->last->expressions);
    
	while (token_peek.type == tok_LessThan ||
           token_peek.type == tok_GreaterThan ||
           token_peek.type == tok_LessThanOrEqual ||
           token_peek.type == tok_GreaterThanOrEqual) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[*binaryOps.at(curt.type)]);
        
		//operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));
        
		token_next; 
		expressions->add(Expression(curt.str, ExpressionGuard_Relational));
		parse_bitwise_shift(&expressions->last->expressions);
	}
    
	layer--;
}

local void parse_equality(array<Expression>* expressions){
	layer++;
	PrettyPrint("equality");
	EOICheck;
    
	expressions->add(Expression(curt.str, ExpressionGuard_Equality));
	parse_relational(&expressions->last->expressions);
    
	while (token_peek.type == tok_NotEqual || token_peek.type == tok_Equal) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[*binaryOps.at(curt.type)]);
        
		//operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpEqual));
        
		token_next; 
		expressions->add(Expression(curt.str, ExpressionGuard_Equality));
		parse_relational(&expressions->last->expressions);
	}
    
	layer--;
}

local void parse_bitwise_and(array<Expression>* expressions){
	layer++;
	PrettyPrint("bit and");
	EOICheck;
    
	expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
	parse_equality(&expressions->last->expressions);
    
	while (token_peek.type == tok_BitAND) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[*binaryOps.at(curt.type)]);
        
		//operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpBitAND));
        
		token_next; 
		expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
		parse_equality(&expressions->last->expressions);
	}
	layer--;
}

local void parse_bitwise_xor(array<Expression>* expressions){
	layer++;
	PrettyPrint("bit xor");
	EOICheck;
    
	expressions->add(Expression(curt.str, ExpressionGuard_BitXOR));
	parse_bitwise_and(&expressions->last->expressions);
    
	while (token_peek.type == tok_BitXOR) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[*binaryOps.at(curt.type)]);
        
		//operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpXOR));
        
		token_next;
		expressions->add(Expression(curt.str, ExpressionGuard_BitXOR));
		parse_bitwise_and(&expressions->last->expressions);
	}
	layer--;
}

local void parse_bitwise_or(array<Expression>* expressions) {
	layer++;
	PrettyPrint("bit or");
	EOICheck;
    
	expressions->add(Expression(curt.str, ExpressionGuard_BitOR));
	parse_bitwise_xor(&expressions->last->expressions);
    
	while (token_peek.type == tok_BitOR) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[*binaryOps.at(curt.type)]);
        
		//operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpBitOR));
        
		token_next; 
		expressions->add(Expression(curt.str, ExpressionGuard_BitOR));
		parse_bitwise_xor(&expressions->last->expressions);
	}
	layer--;
}

local void parse_expressions(array<Expression>* expressions) {
	PrettyPrint("expression preface");
	EOICheck;
    
	expressions->add(Expression(curt.str, ExpressionGuard_Preface));
	parse_bitwise_or(&expressions->last->expressions);
}

#undef token_next
#undef token_last
#undef token_peek
#undef token_look_back
#undef Expect
#undef ElseExpect
#undef ExpectFail
#undef ParseOut
#undef ExpectOneOf
#undef PrettyPrint
#undef EOICheck


///////////////////////
//// @global funcs ////
///////////////////////
Expression Parser::parse(array<token> _tokens){
	layer = 0;
	tokens = _tokens;
	curt = tokens[0];
	Expression exp;
    
	parse_expressions(&exp.expressions);
	
	return exp;
}

//pretty printing vars
local f32 vertical_separation = 1;
local f32 horizontal_separation = 20;
local f32 node_margins = 10;


vec2 pretty_print_recur(Expression& e) {
	//we move through the tree using depth-first search
	//we walk through it twice, first to calculate the positions of all nodes, and second to actually ask
	//UI to place things in those positions. I'm not sure how to combine these 2 things at least with how
	//I'm currently doing it.
	vec2 size;
	forI(e.expressions.count) {
		//our first child defines the left side of the bounding box
		vec2 ret = pretty_print_recur(e.expressions[i]);
		e.cbbx_size.x = Max(e.cbbx_size.x, ret.x);
		e.cbbx_size.y = Max(e.cbbx_size.y, ret.y);
	}

	//if this expression has no children or has run through them all, we can start reporting sizes and pos

	if (e.type == Expression_Literal) {
		e.text = TOSTRING("literal: ", e.expstr);
	}
	else {
		e.text = TOSTRING(ExpTypeStrings[e.type]);
	}

	e.size = UI::CalcTextSize(e.text) + vec2::ONE * node_margins;

	if (!e.expressions.count) 
		e.cbbx_size = e.size;
	else {

		//now that we've finished doing a bunch of sizing, we have to walk our child elements again to give them
		//their relative positions

		vec2 sum;
		forI(e.expressions.count) {
			Expression& ex = e.expressions[i];
			ex.pos = vec2(sum.x, e.size.y + vertical_separation);
			sum.x += ex.cbbx_size.x;
			sum.y = Max(sum.y, ex.cbbx_size.y);
		}

		

		e.cbbx_size = sum;
		e.cbbx_size += vec2(0, e.size.y + vertical_separation);
	}

	return e.cbbx_size;
}

local void pretty_print_final(Expression& e, vec2 parent_pos) {
	forE(e.expressions) pretty_print_final(*it, e.pos + parent_pos);
	vec2 pos = (e.pos + parent_pos).xAdd(-e.size.x/2);// -e.cbbx_size.ySet(0) / 2).xAdd(-e.size.x / 2);
	pos.x = floor(pos.x);
	pos.y = floor(pos.y);


	UI::RectFilled(pos, e.size, color(25, 144, 130));
	UI::Text(TOSTRING(e.cbbx_size).str, pos + vec2::ONE * node_margins / 2);

}

void Parser::pretty_print(Expression& e) {
#if DESHI_SLOW
	
	//local Font* ppfont = Storage::CreateFontFromFileTTF("STIXTwoText-Regular.otf", 30).second;


	if (e.expressions.count) {
		pretty_print_recur(e);
		
		//UI::PushVar(UIStyleVar_FontHeight, 30);
		UI::Begin(TOSTRING("ASTPRETTYPRINT", (char*)&e).str, vec2(300, 300), vec2(300, 1500), UIWindowFlags_FitAllElements);
			
		pretty_print_final(e, vec2(150, 0));
	
		UI::End();
		//UI::PopVar();
	}

#else
	LogW("SUUGUDEBUG", "Function 'pretty_print_AST' called from somewhere when DESHI_SLOW not set!");
#endif
}