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

#define next_match(tok) (tokens.peek().type == tok)
#define next_match_any(tok_type) (tok_type.has(tokens.peek().type))
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

inline void new_expression(char* str, ExpressionType type) {
	expression = (Expression*)arena.add(Expression(str, type));
	expression->node.prev = expression->node.next = &expression->node;
}


enum ParseState {
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

TreeNode* debugprogramnode = 0;

#define EarlyOut goto emergency_exit
void parser(ParseState state, TreeNode* node) {

	switch (state) {

		case psExpression: {/////////////////////////////////////////////////////////////////// @Expression
			switch (curt.type) {
				case Token_Identifier: {
					if (next_match(Token_Assignment)) {
						
						new_expression(curt.str, Expression_IdentifierLHS);
						TreeNodeInsertChild(node, &expression->node, ExTypeStrings[Expression_IdentifierLHS]); token_next();
						new_expression(curt.str, Expression_BinaryOpAssignment);
						TreeNodeInsertChild(node, &expression->node, ExTypeStrings[Expression_BinaryOpAssignment]); token_next();
						new_expression(curt.str, ExpressionGuard_Assignment);
						TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Assignment]);

						parser(psExpression, &expression->node);
					}
					else {
						//new_expression_on_expression(curt.str, ExpressionGuard_HEAD);
						new_expression(curt.str, ExpressionGuard_HEAD);
						TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_HEAD]);
						parser(psConditional, &expression->node);
					}
				}break;
				default: {
					
					new_expression(curt.str, ExpressionGuard_HEAD);
					TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_HEAD]);
					parser(psConditional, &expression->node);
				}break;

			}
			//pop_expression();
		}break;

		case psConditional: {////////////////////////////////////////////////////////////////// @Conditional
			
			new_expression(curt.str, ExpressionGuard_Conditional);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Conditional]);
			parser(psLogicalOR, &expression->node);

			//while (next_match(Token_QuestionMark)) {
			//	token_next();
			//	
			//	new_expression(curt.str, Expression_TernaryConditional);
			//	token_next();
			//	TreeNodeInsertChild(node, &expression->node, ExTypeStrings[Expression_TernaryConditional]);
			//	parser(psExpression, &expression->node);
			//	token_next();
			//	Expect(Token_Colon) {
			//		new_expression(curt.str, ExpressionGuard_Conditional);
			//		token_next();
			//		TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Conditional]);
			//		parser(psConditional, &expression->node);
			//	}ExpectFail("Expected : for ternary conditional")
			//}
		}break;

		case psLogicalOR: {//////////////////////////////////////////////////////////////////// @Logical OR
			
			new_expression(curt.str, ExpressionGuard_LogicalOR);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_LogicalOR]);
			parser(psLogicalAND, &expression->node);

			while (next_match(Token_OR)) {
				token_next();
				
				new_expression(curt.str, Expression_BinaryOpOR);
				token_next();
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_LogicalOR]);
				parser(psLogicalAND, &expression->node);
			}
		}break;

		case psLogicalAND: {/////////////////////////////////////////////////////////////////// @Logical AND
			
			new_expression(curt.str, ExpressionGuard_LogicalAND);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_LogicalAND]);
			parser(psBitwiseOR, &expression->node);

			while (next_match(Token_AND)) {
				token_next();
				
				new_expression(curt.str, Expression_BinaryOpAND);
				token_next();
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_LogicalAND]);
				parser(psBitwiseOR, &expression->node);
			}
		}break;

		case psBitwiseOR: {//////////////////////////////////////////////////////////////////// @Bitwise OR
			
			new_expression(curt.str, ExpressionGuard_BitOR);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitOR]);
			parser(psBitwiseXOR, &expression->node);

			while (next_match(Token_BitOR)) {
				token_next();
				
				new_expression(curt.str, Expression_BinaryOpBitOR);
				token_next();
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitOR]);
				parser(psBitwiseXOR, &expression->node);
			}
		}break;

		case psBitwiseXOR: {/////////////////////////////////////////////////////////////////// @Bitwise XOR
			
			new_expression(curt.str, ExpressionGuard_BitXOR);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitXOR]);
			parser(psBitwiseAND, &expression->node);

			while (next_match(Token_BitXOR)) {
				token_next();
				
				new_expression(curt.str, Expression_BinaryOpBitXOR);
				token_next();
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitXOR]);
				parser(psBitwiseAND, &expression->node);
			}
		}break;

		case psBitwiseAND: {/////////////////////////////////////////////////////////////////// @Bitwise AND
			
			new_expression(curt.str, ExpressionGuard_BitAND);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitAND]);
			parser(psEquality, &expression->node);


			while (next_match(Token_BitAND)) {
				token_next();
				
				new_expression(curt.str, Expression_BinaryOpBitAND);
				token_next();
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitAND]);
				parser(psEquality, &expression->node);
			}
		}break;

		case psEquality: {///////////////////////////////////////////////////////////////////// @Equality
			new_expression(curt.str, ExpressionGuard_Equality);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Equality]);
			parser(psRelational, &expression->node);
			while (next_match(Token_NotEqual) || next_match(Token_Equal)) {
				token_next();
				
				new_expression(curt.str, *binaryOps.at(curt.type));
				token_next();
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Equality]);
				parser(psRelational, &expression->node);
			}
		}break;

		case psRelational: {/////////////////////////////////////////////////////////////////// @Relational
			new_expression(curt.str, ExpressionGuard_Relational);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Relational]);
			parser(psBitshift, &expression->node);

			while (next_match(Token_LessThan) || next_match(Token_GreaterThan) || next_match(Token_LessThanOrEqual) || next_match(Token_GreaterThanOrEqual)) {
				token_next();
				
				new_expression(curt.str, *binaryOps.at(curt.type));
				token_next();
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Relational]);
				parser(psBitshift, &expression->node);
			}
		}break;

		case psBitshift: {///////////////////////////////////////////////////////////////////// @Bitshift
			new_expression(curt.str, ExpressionGuard_BitShift);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitShift]);
			parser(psAdditive, &expression->node);

			while (next_match(Token_BitShiftLeft) || next_match(Token_BitShiftRight)) {
				token_next();
				
				new_expression(curt.str, *binaryOps.at(curt.type));
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[*binaryOps.at(curt.type)]); token_next();
				new_expression(curt.str, ExpressionGuard_BitShift);
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_BitShift]);
				parser(psAdditive, &expression->node);
			}
		}break;

		case psAdditive: {///////////////////////////////////////////////////////////////////// @Additive
			new_expression(curt.str, ExpressionGuard_Additive);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Additive]);
			parser(psTerm, &expression->node);

			while (next_match(Token_Plus) || next_match(Token_Negation)) {
				token_next();
				
				new_expression(curt.str, *binaryOps.at(curt.type));
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[*binaryOps.at(curt.type)]); token_next();
				new_expression(curt.str, ExpressionGuard_Additive);
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Additive]);
				parser(psTerm, &expression->node);
			}
		}break;

		case psTerm: {///////////////////////////////////////////////////////////////////////// @Term
			new_expression(curt.str, ExpressionGuard_Term);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Term]);
			parser(psFactor, &expression->node);

			while (next_match(Token_Multiplication) || next_match(Token_Division) || next_match(Token_Modulo)) {
				token_next();
				
				new_expression(curt.str, *binaryOps.at(curt.type));
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[*binaryOps.at(curt.type)]); token_next();
				new_expression(curt.str, ExpressionGuard_Term);
				TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Term]);
				parser(psFactor, &expression->node);
			}
		}break;

		case psFactor: {/////////////////////////////////////////////////////////////////////// @Factor
			
			new_expression(curt.str, ExpressionGuard_Factor);
			TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_Factor]);
			node = &expression->node;

			switch (curt.type) {
				case Token_Literal: {
					
					new_expression(curt.str, Expression_IntegerLiteral);
					TreeNodeInsertChild(node, &expression->node, toStr(ExTypeStrings[Expression_IntegerLiteral], " ", curt.str));
				}break;

				case Token_OpenParen: {
					
					//new_expression(curt.str, ExpressionGuard_HEAD);
					//TreeNodeInsertChild(node, &expression->node, ExTypeStrings[ExpressionGuard_HEAD]);
					token_next();
					parser(psExpression, &expression->node);
					token_next();
					Expect(Token_CloseParen) {}
					ExpectFail("expected a )");

				}break;

				case Token_Identifier: {
					new_expression(curt.str, Expression_IdentifierRHS);
					TreeNodeInsertChild(node, &expression->node, toStr(ExTypeStrings[Expression_IdentifierRHS], " ", curt.str));
				}break;

				default: {
					ExpectOneOf(unaryOps) {
						new_expression(curt.str, *unaryOps.at(curt.type));
						TreeNodeInsertChild(node, &expression->node, ExTypeStrings[*unaryOps.at(curt.type)]);
						token_next();
						parser(psFactor, &expression->node);
					}
					ExpectFail("unexpected token found in factor");
				}break;
			}


		}break;

		case psUnary: {//////////////////////////////////////////////////////////////////////// @Unary
			
		}break;
	}
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
	Expression exp("", Expression_Empty);
	exp.node.debug_str = "start";

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
};

gvGraph graph;

void make_dot_file(TreeNode* node, Agnode_t* parent) {
	static u32 i = 0;
	i++;
	u32 save = i;

	string send = node->debug_str;
	send.replace('&', "&amp;");
	send.replace('<', "&lt;");
	send.replace('>', "&gt;");

	Agnode_t* me = agnode(gvgraph, (send + "-" + to_string(i)).str, 1);
	TreeNode* stage = node;

	if (stage->first_child)   make_dot_file(stage->first_child, me);
	if (stage->next != stage) make_dot_file(stage->next, parent);

	if (parent)
		agedge(gvgraph, parent, me, "", 1);

}



void Parser::pretty_print(Expression& e) {

	if (!gvc) gvc = gvContext();
	if (!gout) gout = (char*)memtalloc(Kilobytes(2)); //arbitrary size, maybe should be changed later

	if (e.node.first_child) {
		graph.nodes.clear();
		graph.edges.clear();

		gvgraph = agopen("exp ast tree", Agdirected, 0);
		make_dot_file(&e.node, 0);
		agattr(gvgraph, AGRAPH, "splines", "line");
		gvLayout(gvc, gvgraph, "dot");
		gvRenderData(gvc, gvgraph, "plain", &gout, &gout_size);

		FileReader reader = init_reader(gout, gout_size);

		forI(reader.lines.count) {
			next_line(reader);
			if (str_begins_with(reader.read, "node")) {
				gvNode node;
				chunk_line(reader, i, ' ');
				node.label = reader.chunks[1];
				node.pos.x = stod(reader.chunks[2]);
				node.pos.y = stod(reader.chunks[3]);
				node.siz.x = stod(reader.chunks[4]);
				node.siz.y = stod(reader.chunks[5]);
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
		Begin("suuguprettyprint"); {
			static f32 zoom = 20;
			zoom += 10 * DeshInput->scrollY;
			for (gvNode node : graph.nodes) {
				Rect((node.pos - vec2(0, node.siz.y)) * zoom, node.siz * zoom);
				Text(node.label.str, (node.pos - vec2(0, node.siz.y)) * zoom);
			}
		}End();
	}
	return;
}

#else 
void Parser::pretty_print(Expression& e) {}//LogW("SUUGUDEBUG", "Function 'pretty_print' called from somewhere when DESHI_SLOW not set!");}
#endif
