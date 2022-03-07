/////////////////////
//// @local vars ////
/////////////////////
//master token
token curt;

array<token> tokens;

local map<Token_Type, ExpressionType> binaryOps{
	{Token_Multiplication,     Expression_BinaryOpMultiply},
	{Token_Division,           Expression_BinaryOpDivision},
	{Token_Negation,           Expression_BinaryOpMinus},
	{Token_Plus,               Expression_BinaryOpPlus},
	{Token_AND,                Expression_BinaryOpAND},
	{Token_OR,                 Expression_BinaryOpOR},
	{Token_LessThan,           Expression_BinaryOpLessThan},
	{Token_GreaterThan,        Expression_BinaryOpGreaterThan},
	{Token_LessThanOrEqual,    Expression_BinaryOpLessThanOrEqual},
	{Token_GreaterThanOrEqual, Expression_BinaryOpGreaterThanOrEqual},
	{Token_Equal,              Expression_BinaryOpEqual},
	{Token_NotEqual,           Expression_BinaryOpNotEqual},
	{Token_BitAND,             Expression_BinaryOpBitAND},
	{Token_BitOR,              Expression_BinaryOpBitOR},
	{Token_BitXOR,		       Expression_BinaryOpBitXOR},
	{Token_BitShiftLeft,       Expression_BinaryOpBitShiftLeft},
	{Token_BitShiftRight,      Expression_BinaryOpBitShiftRight},
	{Token_Modulo,             Expression_BinaryOpModulo},
};

local map<Token_Type, ExpressionType> unaryOps{
	{Token_BitNOT,     Expression_UnaryOpBitComp},
	{Token_LogicalNOT, Expression_UnaryOpLogiNOT},
	{Token_Negation,   Expression_UnaryOpNegate},
};

local array<Token_Type> typeTokens{
	Token_Signed32,
	Token_Signed64,
	Token_Unsigned32,
	Token_Unsigned64,
	Token_Float32,
	Token_Float64,
};

template<class... T>
inline b32 check_signature(T... in) {
	int i = 0;
	return ((tokens.peek(i++).type == in) && ...);
}


//////////////////////
//// @local funcs ////
//////////////////////
//These defines are mostly for conveinence and clarity as to what im doing
//#define token_next() curt = tokens.next()
#define token_last curt = tokens.prev()

void token_next(u32 count = 1) {
	curt = tokens.next(count);
}

#define token_peek tokens.peek()
#define token_look_back(i) tokens.lookback(i)

#define curr_atch(tok) (curt.type == tok)

#define ParseFail(error)\
std::cout << "\nError: " << error << "\n caused by token '" << curt.str << std::endl;

#define Expect(Token_Type)\
if(curt.type == Token_Type) 

#define ExpectOneOf(exp_type)\
if(exp_type.has(curt.type)) 

#define ElseExpect(Token_Type)\
else if (curt.type == Token_Type) 

#define ExpectSignature(...) if(check_signature(__VA_ARGS__))
#define ElseExpectSignature(...)  else if(check_signature(__VA_ARGS__))

#define ExpectFail(error)\
else { ParseFail(error); }

#define ExpectFailCode(failcode)\
else { failcode }

Expression* expression;

ParseArena arena;

template<typename... T> inline b32
next_match(T... in) {
	return ((tokens.peek().type == in) || ...);
}

enum ParseStage {
	psExpression,	// <exp>           :: = <id> "=" <exp> | <conditional>
	psConditional,	// <conditional>   :: = <logical or> [ "?" <exp> ":" <conditional> ]
	psLogicalOR,	// <logical or>    :: = <logical and> { "||" <logical and> } 
	psLogicalAND,	// <logical and>   :: = <bitwise or> { "&&" <bitwise or> } 
	psBitwiseOR,	// <bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
	psBitwiseXOR,	// <bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
	psBitwiseAND,	// <bitwise and>   :: = <equality> { "&" <equality> }
	psEquality,		// <equality>      :: = <relational> { ("!=" | "==") <relational> }
	psRelational,	// <relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
	psBitshift,		// <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
	psAdditive,		// <additive>      :: = <term> { ("+" | "-") <term> }
	psTerm,			// <term>          :: = <factor> { ("*" | "/" | "%") <factor> }
	psFactor,		// <factor>        :: = "(" <exp> ")" | <unary> <factor> | <int> | <id>
	psUnary,        // <unary>         :: = "!" | "~" | "-"
};

TNode* parser(ParseStage stage, TNode* node);

local map<Token_Type, ExpressionType> tokToExp{
	{Token_Multiplication,     Expression_BinaryOpMultiply},
	{Token_Division,           Expression_BinaryOpDivision},
	{Token_Negation,           Expression_BinaryOpMinus},
	{Token_Plus,               Expression_BinaryOpPlus},
	{Token_AND,                Expression_BinaryOpAND},
	{Token_OR,                 Expression_BinaryOpOR},
	{Token_LessThan,           Expression_BinaryOpLessThan},
	{Token_GreaterThan,        Expression_BinaryOpGreaterThan},
	{Token_LessThanOrEqual,    Expression_BinaryOpLessThanOrEqual},
	{Token_GreaterThanOrEqual, Expression_BinaryOpGreaterThanOrEqual},
	{Token_Equal,              Expression_BinaryOpEqual},
	{Token_NotEqual,           Expression_BinaryOpNotEqual},
	{Token_BitAND,             Expression_BinaryOpBitAND},
	{Token_BitOR,              Expression_BinaryOpBitOR},
	{Token_BitXOR,             Expression_BinaryOpBitXOR},
	{Token_BitShiftLeft,       Expression_BinaryOpBitShiftLeft},
	{Token_BitShiftRight,      Expression_BinaryOpBitShiftRight},
	{Token_Modulo,             Expression_BinaryOpModulo},
	{Token_BitNOT,             Expression_UnaryOpBitComp},
	{Token_LogicalNOT,         Expression_UnaryOpLogiNOT},
	{Token_Negation,           Expression_UnaryOpNegate},
};

template<typename... T>
TNode* binopParse(TNode* node, TNode* ret, ParseStage next_stage, T... tokcheck) {
	token_next();
	TNode* me = new_expression(curt.raw, tokToExp[curt.type], ExTypeStrings[*tokToExp.at(curt.type)]);
	change_parent(me, ret);
	insert_last(node, me);
	token_next();
	ret = parser(next_stage, me);

	while (next_match(tokcheck...)) {
		token_next();
		TNode* me2 = new_expression(curt.raw, tokToExp[curt.type], ExTypeStrings[tokToExp[curt.type]]);
		token_next();
		ret = parser(next_stage, node);

		change_parent(me2, me);
		change_parent(me2, ret);
		insert_last(node, me2);
		me = me2;
		
	}
	return me;
}
	
inline TNode* new_expression(const cstring& str, ExpressionType type, const string& node_str = "") {
	expression = (Expression*)arena.add(Expression());
	expression->expstr = str;
	expression->type = type;
	//expression->node.type = NodeType_Expression;
#if 0
	if (!node_str.count) expression->node.comment = (char*)ExTypeStrings[type];
	else                 expression->node.comment = node_str.str;
#endif
	return &expression->node;
}

TNode* parser(ParseStage state, TNode* node) {
	switch (state) {
		case psExpression: {/////////////////////////////////////////////////////////////////// @Expression
			switch (curt.type) {
				//left over from su, though we may allow varaibles later
				//case Token_Identifier: {
				//	if (next_match(Token_Assignment)) {
				//		new_expression(curt.str, Expression_IdentifierLHS);
				//		insert_first(node, &expression->node); token_next();
				//		new_expression(curt.str, Expression_BinaryOpAssignment);
				//		insert_first(node, &expression->node); token_next();
				//		new_expression(curt.str, ExpressionGuard_Assignment);
				//		insert_first(node, &expression->node);
				//		
				//		parser(psExpression, &expression->node);
				//	}
				//	else {
				//		new_expression(curt.str, ExpressionGuard_HEAD);
				//		insert_first(node, &expression->node);
				//		parser(psConditional, &expression->node);
				//	}
				//}break;

				default: {
					TNode* ret = parser(psConditional, node); 
					return ret;
				}break;
			}
		}break;
		
		case psConditional: {////////////////////////////////////////////////////////////////// @Conditional
			//again a bunch of code left over from su, but useful if we ever allow ternarys
			//Expect(Token_If) {
			//	Node* me = new_expression(curt.raw, Expression_TernaryConditional,  "if exp");
			//	insert_last(node, me);
			//	token_next();
			//	Expect(Token_OpenParen) {
			//		token_next();
			//		Define(psExpression, me);
			//		token_next();
			//		Expect(Token_CloseParen) {
			//			token_next();
			//			Define(psExpression, me);
			//			token_next();
			//			Expect(Token_Else) {
			//				token_next();
			//				Define(psExpression, me);
			//				return me;
			//			}ExpectFail("conditional if's are required to have an else");
			//		}ExpectFail("expected ) for if expression")
			//	}ExpectFail("expected ( for if expression")
			//}
			return parser(psLogicalOR, node);
		}break;
		
		case psLogicalOR: {//////////////////////////////////////////////////////////////////// @Logical OR
			TNode* ret = parser(psLogicalAND, node);
			if (!next_match(Token_OR))
				return ret;
			return binopParse(node, ret, psLogicalAND, Token_OR);
		}break;
		
		case psLogicalAND: {/////////////////////////////////////////////////////////////////// @Logical AND
			TNode* ret = parser(psBitwiseOR, node);
			if (!next_match(Token_AND))
				return ret;
			return binopParse(node, ret, psBitwiseOR);
		}break;
		
		case psBitwiseOR: {//////////////////////////////////////////////////////////////////// @Bitwise OR
			TNode* ret = parser(psBitwiseXOR, node);
			if (!next_match(Token_BitOR))
				return ret;
			return binopParse(node, ret, psBitwiseXOR, Token_BitOR);
		}break;
		
		case psBitwiseXOR: {/////////////////////////////////////////////////////////////////// @Bitwise XOR
			TNode* ret = parser(psBitwiseAND, node);
			if (!next_match(Token_BitXOR))
				return ret;
			return binopParse(node, ret, psBitwiseAND, Token_BitXOR);
		}break;
		
		case psBitwiseAND: {/////////////////////////////////////////////////////////////////// @Bitwise AND
			TNode* ret = parser(psEquality, node);
			if (!next_match(Token_BitAND))
				return ret;
			return binopParse(node, ret, psEquality, Token_BitAND);
		}break;
		
		case psEquality: {///////////////////////////////////////////////////////////////////// @Equality
			TNode* ret = parser(psRelational, node);
			if (!next_match(Token_NotEqual, Token_Equal))
				return ret;
			return binopParse(node, ret, psRelational, Token_NotEqual, Token_Equal);
		}break;
		
		case psRelational: {/////////////////////////////////////////////////////////////////// @Relational
			TNode* ret = parser(psBitshift, node);
			if (!next_match(Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual))
				return ret;
			return binopParse(node, ret, psBitshift, Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual);
		}break;
		
		case psBitshift: {///////////////////////////////////////////////////////////////////// @Bitshift
			TNode* ret = parser(psAdditive, node);
			if (!next_match(Token_BitShiftLeft, Token_BitShiftRight))
				return ret;
			return binopParse(node, ret, psAdditive, Token_BitShiftLeft, Token_BitShiftRight);
		}break;
		
		case psAdditive: {///////////////////////////////////////////////////////////////////// @Additive
			TNode* ret = parser(psTerm, node);
			if (!next_match(Token_Plus, Token_Negation))
				return ret;
			return binopParse(node, ret, psTerm, Token_Plus, Token_Negation);
		}break;
		
		case psTerm: {
			TNode* ret = parser(psFactor, node);
			if (!next_match(Token_Multiplication, Token_Division, Token_Modulo))
				return ret;
			return binopParse(node, ret, psFactor, Token_Multiplication, Token_Division, Token_Modulo);
		}break;
		
		case psFactor: {/////////////////////////////////////////////////////////////////////// @Factor
			switch (curt.type) {
				
				//TODO implicitly change types here when applicable, or do that where they're returned
				case Token_Literal: {
					TNode* var = new_expression(cstring{}, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " ", curt.str));
					expression->val = stod(curt.str); //TODO detect f64
					insert_last(node, &expression->node);
					return var;
				}break;
				
				
				case Token_OpenParen: {
					token_next();
					TNode* ret = parser(psExpression, &expression->node);
					change_parent(node, ret);
					token_next();
					Expect(Token_CloseParen) { return ret; }
					ExpectFail("expected a )");
				}break;
				
				case Token_Negation: {
					new_expression(curt.raw, Expression_UnaryOpNegate);
					insert_last(node, &expression->node);
					token_next();
					TNode* ret = &expression->node;
					parser(psFactor, &expression->node);
					return ret;
				}break;
				
				case Token_LogicalNOT: {
					new_expression(curt.raw, Expression_UnaryOpLogiNOT);
					insert_last(node, &expression->node);
					token_next();
					TNode* ret = &expression->node;
					parser(psFactor, &expression->node);
					return ret;
				}break;
				
				case Token_BitNOT: {
					new_expression(curt.raw, Expression_UnaryOpBitComp);
					insert_last(node, &expression->node);
					token_next();
					TNode* ret = &expression->node;
					parser(psFactor, &expression->node);
					return ret;
				}break;
				
				default: {
					ParseFail("unexpected token found in factor");
				}break;
			}
		}break;

		default:{
			LogE("parser", "Unknown ParseStage");
		}break;
	}
	return 0;
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
Expression Parser::parse(array<token> _tokens) {
	arena.init(Kilobytes(1));
	tokens = _tokens;
	curt = tokens[0];
	Expression exp("", Expression_NONE);
	
	parser(psExpression, &exp.node);
	pretty_print(exp);
	return exp;
}



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
