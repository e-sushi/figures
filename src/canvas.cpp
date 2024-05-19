/* Index:
@tools
@binds
@pencil
@camera
@context
@draw_term
@canvas
@input
@input_tool
@input_navigation
@input_context
@input_expression
@input_expression_deletion
@input_expression_cursor
@input_expression_insertion
@input_pencil
@draw_elements
@draw_elements_expression
@draw_elements_graph
@draw_elements_workspace
@draw_elements_text
@draw_pencil
@draw_canvas_info
*/

// organized collection of data relating to suugu's canvas
local struct{
	
	struct{ // ui
		uiItem* root; // item encompassing the entire space of the canvas
		uiItem* debug; // debug panel which immediate info is drawn into
		struct{ // font
			Font* math;
			Font* text;
			Font* debug;
		}font;
	}ui;

	struct{ // tool
		CanvasTool active;
		CanvasTool previous;

		struct{// pencil
			PencilStroke* strokes;
			u32 skip_amount = 4;
			struct{ // stroke
				u32   idx  = 0;
				f32   size = 1;
				color color = PackColorU32(249,195,69,255);
				vec2  start_pos;
			}stroke;
		}pencil;
	}tool;

	struct{ // camera
		vec2 pos{0,0};
		f64  zoom = 1.0;
		vec2 pan_start_pos;
		vec2 pan_mouse_pos;
		b32  pan_active = false;
	}camera;

	struct{ // world
		vec2 mouse_pos;
	}world;

	struct{
		Element** arr; // kigu array
		Element* selected;
	}element;
}canvas; 

local vec2 
ToScreen(vec2 point){
	point -= canvas.camera.pos;
	point /= canvas.camera.zoom;
	point.y *= -f64(canvas.ui.root->width) / f64(canvas.ui.root->height);
	point += vec2{1.0, 1.0};
	point.x *= f64(canvas.ui.root->width); point.y *= f64(canvas.ui.root->height);
	point /= 2.0;
	return Vec2(point.x, point.y);
}FORCE_INLINE vec2 ToScreen(f32 x, f32 y){ return ToScreen({x,y}); }

local vec2
ToWorld(vec2 _point){
	vec2 point{_point.x, _point.y};
	point.x /= f32(DeshWindow->dimensions.x); point.y /= f32(DeshWindow->dimensions.y);
	point *= 2.0;
	point -= vec2{1.0, 1.0};
	point.y /= -f32(DeshWindow->width) / f32(DeshWindow->height);
	point *= canvas.camera.zoom;
	point += canvas.camera.pos;
	return point;
}FORCE_INLINE vec2 ToWorld(f32 x, f32 y){ return ToWorld({x,y}); }

//returns the width and height of the area in world space that the user can currently see as a vec2
local vec2 
WorldViewArea(){
	return Vec2(2 * canvas.camera.zoom, 2 * canvas.camera.zoom * (float)DeshWindow->height / DeshWindow->width);
}

//////////////////
//// @context ////
//////////////////
local char context_input_buffer[256] = {};
local u32 context_dropdown_selected_index = 0;
local const char* context_dropdown_option_strings[] = {
	"Tool: Navigation", "Tool: Expression", "Tool: Pencil",
	"Add: Graph",
};

void scale_render_part(Element* element, RenderPart* part, vec2 scale) {
	uiDrawCmdPtrs draw_cmd_ptrs = ui_drawcmd_get_ptrs(element->item->drawcmds);
	part->bbx.x *= scale.x;
	part->bbx.y *= scale.y;
	forI(part->vcount) {
		(draw_cmd_ptrs.vertexes + part->vstart + i)->pos.x *= scale.x;
		(draw_cmd_ptrs.vertexes + part->vstart + i)->pos.y *= scale.y;
	}
}

void offset_render_part_position(Element* element, RenderPart* part, vec2 offset) {
	part->position += offset;
	if(part->type == RenderPart_Group) {
		forI(part->group_children) {
			offset_render_part_position(element, part + i + 1, offset);
		}
	} 
}

void offset_render_part(Element* element, RenderPart* part, vec2 offset) {
	uiDrawCmdPtrs draw_cmd_ptrs = ui_drawcmd_get_ptrs(element->item->drawcmds);
	forI(part->vcount){
		(draw_cmd_ptrs.vertexes + part->vstart + i)->pos.x += offset.x;
		(draw_cmd_ptrs.vertexes + part->vstart + i)->pos.y += offset.y;
	}
	offset_render_part_position(element, part, offset);
}

#if 1
#define RenderLog(...) Log("suugu_render", __VA_ARGS__)
#else
#define RenderLog(...)
#endif


/*
	Rendering a term consists of creating RenderParts, which represent the position, bounding box, 
	vertexes and indexes required to render the Term. 

	When a Term is to be rendered, it pushes all of the render parts it will need onto the 'stack'
	and then further instructions give an index to which render part it will be dealing with.

*/

#define RenderError(...) LogE("suugu_render", term->mathobj->name, ":", current_token->line, ":", current_token->column, " ", __VA_ARGS__)

RenderPart render_term(Term* term, Element* element) {
	logger_push_indent(1);
	defer{ logger_pop_indent(1); };
	
	uiDrawCmd* draw_cmd = element->item->drawcmds;
	uiDrawCmdPtrs draw_cmd_ptrs = ui_drawcmd_get_ptrs(draw_cmd);
	
	Token* current_token = term->mathobj->display.instruction_tokens;

	u32 current_render_part_index = array_count(element->expression.rendered_parts);
	u32* proper_indexes = array_create(u32, 8, deshi_temp_allocator);
	u32 parts_rendered = 0;

	struct PositionReference {
		b32 is_scalar;
		u32 x_axis; // true if the scalar represents a value along the x_axis, false if over y
		RenderPart* part;
		union {
			f32 scalar;
			vec2 vec;
		};
	};

	// TODO(sushi) this needs to return a vec2 no matter what because aligning relies on it 
	const auto eval_position = [&](b32 force_vector = false) {
		RenderLog("eval_position");
		enum{
			State_Vector=1<<0,
			State_Function=1<<1,
		};
		u32 state = 0;
		auto impl = [&](auto& recur) -> PositionReference {
			switch(current_token->type) {
				case Token_Backtick: {
					current_token++;
					if(current_token->type != Token_Integer) {
						RenderError("need an integer after '`'");
						return PositionReference{0,{}};
					}
					RenderPart* part = element->expression.rendered_parts + proper_indexes[stoi(current_token->raw)];
					current_token++;
					PositionReference out;
					out.part = part;
					switch(current_token->type) {
						case Token_bottom:   out.scalar = part->position.y+part->bbx.y;   out.is_scalar = true;  out.x_axis = 0; break;
						case Token_left:     out.scalar = part->position.x;               out.is_scalar = true;  out.x_axis = 1; break;
						case Token_top:      out.scalar = part->position.y;               out.is_scalar = true;  out.x_axis = 0; break;
						case Token_right:    out.scalar = part->position.x+part->bbx.x;   out.is_scalar = true;  out.x_axis = 1; break;
						case Token_center:   out.vec = (part->position + part->bbx) / 2;  out.is_scalar = false; out.x_axis = 0; break;
						case Token_center_x: out.scalar = part->position.x+part->bbx.x/2; out.is_scalar = true;  out.x_axis = 1; break;
						case Token_center_y: out.scalar = part->position.y+part->bbx.y/2; out.is_scalar = true;  out.x_axis = 0; break;
						case Token_origin:   out.vec = (part->position + part->bbx) / 2;  out.is_scalar = false; out.x_axis = 0; break;
						case Token_origin_x: out.scalar = part->position.x+part->bbx.x/2; out.is_scalar = true;  out.x_axis = 1; break;
						case Token_origin_y: out.scalar = part->position.y+part->bbx.y/2; out.is_scalar = true;  out.x_axis = 0; break;

						default: {
							RenderError("invalid position, expected one of (top, bottom, left, right, center, center_x, center_y, origin, origin_x, origin_y)");
							return {};
						}break;
					}
					current_token++;
					return out;
				}break;

				case Token_OpenParen: {
					if(HasFlag(state, State_Vector)) {
						RenderError("cannot start a vector within a vector");
						return {};
					}
					AddFlag(state, State_Vector);
					current_token++;
					PositionReference out;
					PositionReference l = recur(recur);
					if(!l.is_scalar) {
						RenderError("vector arguments must be scalars");
						return {};
					}
					if(current_token->type != Token_Comma) {
						RenderError("expected a ',' after first parameter of vector");
						return {};
					}
					current_token++;
					PositionReference r = recur(recur);
					if(!r.is_scalar) {
						RenderError("vector arguments must be scalars");
						return {};
					}
					out.is_scalar = false;
					out.vec.x = l.scalar;
					out.vec.y = r.scalar;
					if(current_token->type != Token_CloseParen) {
						RenderError("expected ')' to close vector");
					}
					current_token++;
					RemoveFlag(state, State_Vector);
					return out;
				}break;

				case Token_max: {
					current_token++;
					if(current_token->type != Token_OpenParen) {
						RenderError("expected '(' after token 'max'");
						return {};
					}
					current_token++;
					PositionReference l = recur(recur);
					if(current_token->type != Token_Comma) {
						RenderError("expected ',' after first parameter of max");
						return {};
					}
					current_token++;
					PositionReference r = recur(recur);
					if(current_token->type != Token_CloseParen) {
						RenderError("expected ')' to close function max");
						return {};
					}
					current_token++;
					if(l.is_scalar != r.is_scalar) {
						RenderError("cannot perform max between a vector and a scalar");
						return {};
					}
					PositionReference out;
					out.is_scalar = l.is_scalar;
					if(l.is_scalar) {
						if(l.scalar > r.scalar) {
							out = l;
						} else {
							out = r;
						}
					} else { // NOTE(sushi) we cannot decide on either part when we take the max of 2 vectors
						out.vec = Max(l.vec, r.vec);
					}
					return out;
				}break; 

				case Token_min: {
					current_token++;
					if(current_token->type != Token_OpenParen) {
						RenderError("expected '(' after token 'min'");
						return {};
					}
					current_token++;
					PositionReference l = recur(recur);
					if(current_token->type != Token_Comma) {
						RenderError("expected ',' after first parameter of min");
						return {};
					}
					current_token++;
					PositionReference r = recur(recur);
					if(current_token->type != Token_CloseParen) {
						RenderError("expected ')' to close function min");
						return {};
					}
					current_token++;
					if(l.is_scalar != r.is_scalar) {
						RenderError("cannot perform min between a vector and a scalar");
						return {};
					}
					PositionReference out;
					out.is_scalar = l.is_scalar;
					if(l.is_scalar) {
						if(l.scalar < r.scalar) {
							out = l;
						} else {
							out = r;
						}
					} else { // NOTE(sushi) we cannot decide on either part when we take the min of 2 vectors
						out.vec = Min(l.vec, r.vec);
					}
					return out;
				}break; 

				case Token_avg: {
					current_token++;
					if(current_token->type != Token_OpenParen) {
						RenderError("expected '(' after token 'avg'");
						return {};
					}
					current_token++;
					PositionReference l = recur(recur);
					if(current_token->type != Token_Comma) {
						RenderError("expected ',' after first parameter of avg");
						return {};
					}
					current_token++;
					PositionReference r = recur(recur);
					if(current_token->type != Token_CloseParen) {
						RenderError("expected ')' to close function avg");
						return {};
					}
					current_token++;
					if(l.is_scalar != r.is_scalar) {
						RenderError("cannot perform avg between a vector and a scalar");
						return {};
					}
					PositionReference out;
					out.is_scalar = l.is_scalar;
					if(l.is_scalar) {
						out.scalar = (l.scalar + r.scalar) / 2;
					} else {
						out.vec = (l.vec + r.vec) / 2;
					}

					return out;
				}break;

				default: {
					RenderError("need a position expression");
				}break;
			}
			return PositionReference{0,{}};
		};
		return impl(impl);
	};

	while(1) {
		switch(current_token->type) {
			case Token_render: {
				current_token++;
				switch(current_token->type) {
					case Token_child: {
						current_token++;
						RenderLog("rendering child ", stoi(current_token->raw));
						if(current_token->type != Token_Integer) {
							RenderError("need an integer after 'child' token.");
							return {};
						}
						
						Term* node = term->first_child;
						forI(stoi(current_token->raw)){
							node = node->next;
						}
						
						*array_push(proper_indexes) = array_count(element->expression.rendered_parts);
						render_term(node, element);
						
						parts_rendered++;
						current_token++;
					}break;
					
					case Token_text: {
						RenderLog("drawing text");
						current_token++;
						
						str8 text;
						switch(current_token->type) {
							case Token_String: {
								text = current_token->raw;
							}break;
							case Token_term_raw:{
								text = term->raw.buffer.fin;
							}break;
							default: {
								RenderError("need a string or 'term_raw' after 'text' token");
								return {};
							}break;
						}
						
						*array_push(proper_indexes) = array_count(element->expression.rendered_parts);
						array_push(element->expression.rendered_parts);
						
						f32 scale = element->item->style.font_height / element->item->style.font->max_height;
						vec2i text_counts = ui_put_text_counts(str8_length(text));
						
						RenderPart* rp = element->expression.rendered_parts + proper_indexes[parts_rendered++];
						rp->bbx = font_visual_size(element->item->style.font, text) * scale;
						rp->position = vec2_ZERO();
						rp->vcount = text_counts.x;
						rp->vstart = draw_cmd->counts_used.x;
						rp->icount = text_counts.y;
						rp->istart = draw_cmd->counts_used.y;
						rp->term = term;
						
						draw_cmd_ptrs = ui_drawcmd_realloc(draw_cmd, draw_cmd->counts_used + text_counts);
						draw_cmd->counts_used += ui_put_text(draw_cmd_ptrs.vertexes, draw_cmd_ptrs.indexes, draw_cmd->counts_used, text, element->item->style.font, {0,0}, Color_White, vec2{scale, scale});
						
						current_token++;
					}break;
					
					case Token_shape: {
						current_token++;
						switch(current_token->type){
							case Token_line: {
								RenderLog("drawing line");
								current_token++;
								
								PositionReference start = eval_position();
								PositionReference end = eval_position();
								RenderLog(start.vec, " -> ", end.vec);
								
								*array_push(proper_indexes) = array_count(element->expression.rendered_parts);
								array_push(element->expression.rendered_parts);
								
								vec2i line_counts = ui_put_line_counts();
								
								RenderPart* rp = element->expression.rendered_parts + proper_indexes[parts_rendered++];
								rp->position = vec2_ZERO();
								rp->vcount = line_counts.x;
								rp->vstart = draw_cmd->counts_used.x;
								rp->icount = line_counts.y;
								rp->istart = draw_cmd->counts_used.y;
								rp->term = term;
								rp->cursor_ignore = true;
								
								draw_cmd_ptrs = ui_drawcmd_realloc(draw_cmd, draw_cmd->counts_used + line_counts);
								draw_cmd->counts_used += ui_put_line(draw_cmd_ptrs.vertexes, draw_cmd_ptrs.indexes, draw_cmd->counts_used, start.vec, end.vec, 1, Color_White);
								
								rp->bbx = vec2_ZERO();
								forI(rp->vcount){
									uiVertex* vertex = draw_cmd_ptrs.vertexes + rp->vstart + i;
									rp->bbx.x = Max(rp->bbx.x, vertex->pos.x);
									rp->bbx.y = Max(rp->bbx.y, vertex->pos.y);									
								}
								
								current_token++;
							}break;	
						}						
					}break;
				}
			}break;
			
			case Token_align: {
				current_token++;
				RenderLog("aligning");
				PositionReference offset0 = eval_position();
				PositionReference offset1 = eval_position();
				vec2 o0, o1;
				if(offset0.x_axis) {
					o0.x = offset0.scalar;
					o0.y = 0;
				} else {
					o0.x = 0;
					o0.y = offset0.scalar;
				}
				if(offset1.x_axis) {
					o1.x = offset1.scalar;
					o1.y = 0;
				} else {
					o1.x = 0;
					o1.y = offset1.scalar;;
				}
				RenderLog("  ", offset0.part->term->mathobj->name, " ", offset0.part - element->expression.rendered_parts, "  ->  ", offset1.part->term->mathobj->name, " ", offset1.part - element->expression.rendered_parts);
				offset_render_part(element, offset0.part, o1-o0);
				RenderLog("  by offset ", o1-o0);
			}break;
		}
		if(current_token->type == Token_EOF) break;
	}

	RenderPart* fin = element->expression.rendered_parts + current_render_part_index;
	if(parts_rendered > 1) {
		RenderPart* group = array_insert(element->expression.rendered_parts, current_render_part_index);
		group->type = RenderPart_Group;
		group->term = term;
		group->vstart = MAX_S32;
		group->istart = MAX_S32;
		group->group_children = parts_rendered;
		RenderPart* iter = fin + 1;
		forI(parts_rendered) {
			RenderLog("child ", i);
			RenderLog("  raw: ", iter->term->raw.buffer.fin);
			RenderLog("  pos: ", iter->position);
			RenderLog("  bbx: ", iter->bbx);
			RenderLog("  vco: ", iter->vcount);
			RenderLog("  ico: ", iter->icount);
			fin->position = Min(fin->position, iter->position);
			fin->bbx = Max(fin->bbx, fin->position+iter->position+iter->bbx);
			fin->vcount += iter->vcount;
			fin->icount += iter->icount;
			fin->vstart = Min(fin->vstart, iter->vstart);
			fin->istart = Min(fin->istart, iter->istart);
			if(iter->type == RenderPart_Group) {
				iter += iter->group_children+1;
			} else iter += 1;
		}
		return *group;
	}
	return *fin;
}

void render_element(uiItem* item) {
	Element* element = (Element*)item->userVar;
	switch(element->type) {
		case ElementType_Expression: {
			//NOTE: drawcmd filled out during item evaluation
		}break;
		case ElementType_Graph: {

		}break;
		case ElementType_NULL: {

		}break;
	}
}

void evaluate_element(uiItem* item) {
	RenderLog("", "-------------------------------- render start");
	Element* element = (Element*)item->userVar;
	array_deinit(element->expression.rendered_parts);
	array_init(element->expression.rendered_parts, 4, deshi_allocator);
	array_deinit(element->expression.position_map.x);
	array_deinit(element->expression.position_map.y);
	array_init(element->expression.position_map.x, 4, deshi_allocator);
	array_init(element->expression.position_map.y, 4, deshi_allocator);

	switch(element->type) {
		case ElementType_Expression:{ 
			if(element->expression.handle.root.mathobj){
				RenderPart rp = render_term(&element->expression.handle.root, element);
				item->size = {rp.bbx.x, rp.bbx.y};

				forI(array_count(element->expression.rendered_parts)) {
					RenderPart rp = element->expression.rendered_parts[i];
					if(rp.type == RenderPart_Group) {
						RenderLog(i, " group");
					} else {
						RenderLog(i, " indiv");
					}
					RenderLog("  raw: ", rp.term->raw.buffer.fin);
					RenderLog("  pos: ", rp.position);
					RenderLog("  bbx: ", rp.bbx);
					RenderLog("    v: ", rp.vstart, " -> ", rp.vstart + rp.vcount);
					RenderLog("    i: ", rp.istart, " -> ", rp.istart + rp.icount);

					
					if(rp.type == RenderPart_Group || rp.cursor_ignore == true) continue;
					position_map_insert(element, rp.term, rp.position);
				}

				RenderLog("", "pos x: ");
				forI(array_count(element->expression.position_map.x)) {
					RenderLog("", "  ", element->expression.position_map.x[i].pos, " ", element->expression.position_map.x[i].term->raw.buffer.fin);
					TermPos* curr = element->expression.position_map.x + i;
					TermPos* prev = (i? curr - 1 : 0);
					TermPos* next = (i != array_count(element->expression.position_map.x) - 1? curr + 1 : 0);
					if(next) {
						curr->term->movement.right = next->term;
						next->term->movement.left  = curr->term;
					}
					if(prev) {
						curr->term->movement.left  = prev->term;
						prev->term->movement.right = curr->term;
					}
				}

				RenderLog("", "pos y: ");
				forI(array_count(element->expression.position_map.y)) {
					RenderLog("", "  ", element->expression.position_map.y[i].pos, " ", element->expression.position_map.y[i].term->raw.buffer.fin);
					TermPos* curr = element->expression.position_map.y + i;
					TermPos* prev = 0;
					TermPos* next = 0;
					forX_reverse(j, i-1) {
						TermPos* scan = element->expression.position_map.y + j;
						if(scan->pos != curr->pos) prev = scan;
					}
					forX(j, array_count(element->expression.position_map.y) - i - 1) {
						TermPos* scan = element->expression.position_map.y + j;
						if(scan->pos != curr->pos) next = scan;
					}
					if(next) {
						curr->term->movement.up   = next->term;
						next->term->movement.down = curr->term;
					}
					if(prev) {
						curr->term->movement.down = prev->term;
						prev->term->movement.up   = curr->term;
					}
				}
			}
		}break;
		case ElementType_Graph: {

		}break;
		case ElementType_NULL: {

		}break;
	}
}

//NOTE(delle) raw cursor is drawn to the left of the character
//NOTE(delle) mathobj cursor is drawn to the right of the mathobj
void draw_term_old(Expression* expr, Term* mathobj, vec2& cursor_start, f32& cursor_y){
	FixMe;
// #define CursorAfterLastItem() (cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(), cursor_y = -UI::GetLastItemSize().y)
// #define CursorBeforeLastItem() (cursor_start = UI::GetLastItemPos(), cursor_y = UI::GetLastItemSize().y)
	
// 	if(mathobj == 0) return;
// 	switch(mathobj->type){
// 		case TermType_Expression:{
// 			Expression* expr = ExpressionFromTerm(mathobj);
// 			UI::TextOld(str8_lit(" "), UITextFlags_NoWrap); UI::SameLine();
// 			if(HasFlag(mathobj->flags, TermFlag_DanglingClosingParenToRight)){
// 				UI::TextOld(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
// 				if(expr->raw_cursor_start == 1) CursorBeforeLastItem();
// 			}
// 			if(mathobj->child_count){
// 				draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 				for_node(mathobj->first_child->next){
// 					UI::TextOld(str8_lit(" "), UITextFlags_NoWrap); UI::SameLine();
// 					draw_term_old(expr, it, cursor_start, cursor_y);
// 				}
// 			}
// 			if(expr->raw_cursor_start == expr->raw.count) CursorAfterLastItem();
			
// 			//draw solution if its valid
// 			if(expr->valid){
// 				UI::PushColor(UIStyleCol_Text, Color_Grey);
// 				if(expr->equals){
// 					if(expr->solution == MAX_F64){
// 						UI::TextOld(str8_lit("ERROR"));
// 					}else{
// 						UI::TextF(str8_lit("%g"), expr->solution);
// 					}
// 					UI::SameLine();
// 				}else if(expr->solution != MAX_F64){
// 					UI::TextOld(str8_lit("="), UITextFlags_NoWrap); UI::SameLine();
// 					UI::TextF(str8_lit("%g"), expr->solution);
// 					UI::SameLine();
// 				}
// 				UI::PopColor();
// 			}
// 			UI::TextOld(str8_lit(" "), UITextFlags_NoWrap);
// 		}break;
		
// 		case TermType_Operator:{
// 			switch(mathobj->op_type){
// 				case OpType_Parentheses:{
// 					UI::TextOld(str8_lit("("), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str) CursorBeforeLastItem();
// 					if(mathobj->first_child){
// 						draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 						for_node(mathobj->first_child->next){
// 							UI::TextOld(str8_lit(" "), UITextFlags_NoWrap); UI::SameLine();
// 							draw_term_old(expr, it, cursor_start, cursor_y);
// 						}
// 					}
// 					if(HasFlag(mathobj->flags, TermFlag_LeftParenHasMatchingRightParen)){
// 						UI::TextOld(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
// 						if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str + mathobj->raw.count) CursorBeforeLastItem();
// 					}
// 				}break;
				
// 				case OpType_Exponential:{
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("^"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str) CursorBeforeLastItem();
// 					if(mathobj->last_child && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, mathobj->last_child, cursor_start, cursor_y);
// 				}break;
				
// 				case OpType_Negation:{
// 					UI::TextOld(str8_lit("-"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str) CursorBeforeLastItem();
// 					draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 				}break;
				
// 				case OpType_ImplicitMultiplication:{
// 					draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 					draw_term_old(expr, mathobj->last_child, cursor_start, cursor_y);
// 				}break;
				
// 				case OpType_ExplicitMultiplication:{
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("*"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str) CursorBeforeLastItem();
// 					if(mathobj->last_child && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, mathobj->last_child, cursor_start, cursor_y);
// 				}break;
// 				case OpType_Division:{
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("/"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str) CursorBeforeLastItem();
// 					if(mathobj->last_child && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, mathobj->last_child, cursor_start, cursor_y);
// 				}break;
// 				case OpType_Modulo:{
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("%"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str) CursorBeforeLastItem();
// 					if(mathobj->last_child && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, mathobj->last_child, cursor_start, cursor_y);
// 				}break;
				
// 				case OpType_Addition:{
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("+"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str) CursorBeforeLastItem();
// 					if(mathobj->last_child && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, mathobj->last_child, cursor_start, cursor_y);
// 				}break;
// 				case OpType_Subtraction:{
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("-"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str) CursorBeforeLastItem();
// 					if(mathobj->last_child && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, mathobj->last_child, cursor_start, cursor_y);
// 				}break;
				
// 				case OpType_ExpressionEquals:{
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, mathobj->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("="), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str) CursorBeforeLastItem();
// 					if(mathobj->last_child && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, mathobj->last_child, cursor_start, cursor_y);
// 					if(mathobj->last_child) for_node(mathobj->last_child->next) draw_term_old(expr, it, cursor_start, cursor_y);
// 				}break;
				
// 				default: Assert(!"operator type drawing not setup"); break;
// 			}
// 			if(HasFlag(mathobj->flags, TermFlag_DanglingClosingParenToRight)){
// 				UI::TextOld(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
// 				if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str + mathobj->raw.count) CursorBeforeLastItem();
// 			}
// 		}break;
		
// 		case TermType_Literal:
// 		case TermType_Variable:{
// 			//TODO italics for variables (make this an option)
// 			UI::TextOld(str8{(u8*)mathobj->raw.str, (s64)mathobj->raw.count}, UITextFlags_NoWrap); UI::SameLine();
// 			if((mathobj->raw.str <= expr->raw.str + expr->raw_cursor_start) && (expr->raw.str + expr->raw_cursor_start < mathobj->raw.str + mathobj->raw.count)){
// 				f32 x_offset = UI::CalcTextSize(str8{(u8*)mathobj->raw.str, s64(expr->raw.str + expr->raw_cursor_start - mathobj->raw.str)}).x;
// 				cursor_start = UI::GetLastItemPos() + vec2{x_offset,0}; cursor_y = UI::GetLastItemSize().y;
// 			}
// 			if(HasFlag(mathobj->flags, TermFlag_DanglingClosingParenToRight)){
// 				UI::TextOld(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
// 				if(expr->raw.str + expr->raw_cursor_start == mathobj->raw.str + mathobj->raw.count){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
// 			}
// 		}break;
		
// 		case TermType_FunctionCall:{
// 			UI::TextOld(str8{(u8*)mathobj->raw.str, (s64)mathobj->raw.count}, UITextFlags_NoWrap); UI::SameLine();
// 			if((mathobj->raw.str <= expr->raw.str + expr->raw_cursor_start) && (expr->raw.str + expr->raw_cursor_start < mathobj->raw.str + mathobj->raw.count)){
// 				f32 x_offset = UI::CalcTextSize(str8{(u8*)mathobj->raw.str, s64(expr->raw.str + expr->raw_cursor_start - mathobj->raw.str)}).x;
// 				cursor_start = UI::GetLastItemPos() + vec2{x_offset,0}; cursor_y = UI::GetLastItemSize().y;
// 			}
// 			for_node(mathobj->first_child) draw_term_old(expr, it, cursor_start, cursor_y);
// 		}break;
		
// 		case TermType_Logarithm:{
// 			UI::TextOld(str8{(u8*)mathobj->raw.str, (s64)mathobj->raw.count}, UITextFlags_NoWrap); UI::SameLine();
// 			if((mathobj->raw.str <= expr->raw.str + expr->raw_cursor_start) && (expr->raw.str + expr->raw_cursor_start < mathobj->raw.str + mathobj->raw.count)){
// 				f32 x_offset = UI::CalcTextSize(str8{(u8*)mathobj->raw.str, s64(expr->raw.str + expr->raw_cursor_start - mathobj->raw.str)}).x;
// 				cursor_start = UI::GetLastItemPos() + vec2{x_offset,0}; cursor_y = UI::GetLastItemSize().y;
// 			}
// 			for_node(mathobj->first_child) draw_term_old(expr, it, cursor_start, cursor_y);
// 		}break;
		
// 		default: Assert(!"mathobj type drawing not setup"); break;
// 	}
// #undef CursorAfterLastItem
// #undef CursorBeforeLastItem
}

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @debug_draw_term_tree //@@
//NOTE(delle) for simplicity, currently supports only the active expression
local struct{
	b32 visible = false;
	int depth;
	uiStyle term_style{};
	uiItem* expression;
	
	struct{
		int depth;
		uiItem* root;
	}**term_array; //sorted by deepest first
}debug_draw_term_tree_context;

void debug_draw_term_tree(Expression* expr, Term* mathobj){DPZoneScoped;
	FixMe;
// #define ctx debug_draw_term_tree_context
// 	auto highlight_border_when_focused = [](uiItem* item){
// 		if(g_ui->active == item){
// 			item->style.border_color = Color_Grey;
// 		}else{
// 			item->style.border_color = Color_VeryDarkGrey;
// 		}
// 	};
	
// 	if(mathobj == 0) return;
	
// 	str8 term_text{};

// 	switch(mathobj->type){
// 		case TermType_Expression:{
// 			//reset the context
// 			ctx.depth = 0;
// 			if(ctx.expression) uiItemR(ctx.expression);
			
// 			//fill the mathobj style
// 			ctx.term_style.positioning   = pos_static; //change to pos_absolute when actually positioning items
// 			ctx.term_style.margin        = Vec4(2,2,2,2);
// 			ctx.term_style.font          = assets_font_create_from_file(STR8("gohufont-uni-14.ttf"),14);
// 			ctx.term_style.font_height   = 12;
// 			ctx.term_style.text_color    = Color_LightGrey;
// 			ctx.term_style.content_align = Vec2(0.5f,0.5f);
			
// 			//begin the expression container
// 			ctx.expression = uiItemB();
// 			ctx.expression->id = STR8("expression");
// 			ctx.expression->style.positioning      = pos_draggable_absolute;
// 			ctx.expression->style.anchor           = anchor_bottom_left;
// 			ctx.expression->style.size             = Vec2(g_window->height/2, g_window->height/2);
// 			ctx.expression->style.background_color = Color_DarkCyan;
// 			ctx.expression->style.border_style     = border_solid;
// 			ctx.expression->style.border_color     = Color_VeryDarkGrey;
// 			ctx.expression->style.border_width     = 5;
// 			ctx.expression->style.focus            = true;
// 			ctx.expression->style.display          = (ctx.visible) ? 0 : display_hidden;
// 			ctx.expression->style.overflow         = overflow_scroll;
// 			ctx.expression->action         = highlight_border_when_focused;
// 			ctx.expression->action_trigger = action_act_always;
			
// 			//gather the terms and their depths
// 			arrsetlen(ctx.term_array, expr->terms.count);
// 			Expression* expr = ExpressionFromTerm(mathobj);
// 			if(mathobj->child_count){
// 				ctx.depth += 1;
// 				debug_draw_term_tree(expr, mathobj->first_child);
// 				for_node(mathobj->first_child->next){
// 					debug_draw_term_tree(expr, it);
// 				}
// 				ctx.depth -= 1;
// 			}
			
// 			//position the mathobj items
			
			
// 			//draw lines between terms and their parents
			
			
// 			//end the expression container
// 			uiItemE();
// 		}break;
		
// 		case TermType_Operator:{
// 			switch(mathobj->op_type){
// 				case OpType_Parentheses:{
// 					if(mathobj->first_child){
// 						ctx.depth += 1;
// 						debug_draw_term_tree(expr, mathobj->first_child);
// 						for_node(mathobj->first_child->next){
// 							debug_draw_term_tree(expr, it);
// 						}
// 						ctx.depth -= 1;
// 					}
// 					term_text = STR8("(");
// 				}break;
// 				case OpType_Exponential:{
// 					ctx.depth += 1;
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, mathobj->first_child);
// 					if(mathobj->last_child  && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, mathobj->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("^");
// 				}break;
// 				case OpType_Negation:{
// 					ctx.depth += 1;
// 					debug_draw_term_tree(expr, mathobj->first_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("NEG");
// 				}break;
// 				case OpType_ImplicitMultiplication:{
// 					ctx.depth += 1;
// 					debug_draw_term_tree(expr, mathobj->first_child);
// 					debug_draw_term_tree(expr, mathobj->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("*i");
// 				}break;
// 				case OpType_ExplicitMultiplication:{
// 					ctx.depth += 1;
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, mathobj->first_child);
// 					if(mathobj->last_child  && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, mathobj->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("*e");
// 				}break;
// 				case OpType_Division:{
// 					ctx.depth += 1;
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, mathobj->first_child);
// 					if(mathobj->last_child  && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, mathobj->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("/");
// 				}break;
// 				case OpType_Modulo:{
// 					ctx.depth += 1;
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, mathobj->first_child);
// 					if(mathobj->last_child  && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, mathobj->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("%");
// 				}break;
// 				case OpType_Addition:{
// 					ctx.depth += 1;
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, mathobj->first_child);
// 					if(mathobj->last_child  && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, mathobj->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("+");
// 				}break;
// 				case OpType_Subtraction:{
// 					ctx.depth += 1;
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, mathobj->first_child);
// 					if(mathobj->last_child  && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, mathobj->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("-");
// 				}break;
// 				case OpType_ExpressionEquals:{
// 					ctx.depth += 1;
// 					if(mathobj->first_child && HasFlag(mathobj->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, mathobj->first_child);
// 					if(mathobj->last_child  && HasFlag(mathobj->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, mathobj->last_child);
// 					if(mathobj->last_child) for_node(mathobj->last_child->next) debug_draw_term_tree(expr, it);
// 					ctx.depth -= 1;
// 					term_text = STR8("=");
// 				}break;
// 				default: Assert(!"operator type drawing not setup"); break;
// 			}
// 		}break;
		
// 		case TermType_Literal:{
// 			term_text = ToString8(deshi_temp_allocator, mathobj->lit_value);
// 		}break;
		
// 		case TermType_Variable:{
// 			term_text = mathobj->raw;
// 		}break;
		
// 		case TermType_FunctionCall:{
// 			ctx.depth += 1;
// 			for_node(mathobj->first_child) debug_draw_term_tree(expr, it);
// 			ctx.depth -= 1;
// 			term_text = mathobj->func->text;
// 		}break;
		
// 		case TermType_Logarithm:{
// 			ctx.depth += 1;
// 			for_node(mathobj->first_child) debug_draw_term_tree(expr, it);
// 			ctx.depth -= 1;
// 			term_text = ToString8(deshi_temp_allocator, STR8("log"), mathobj->log_base);
// 		}break;
		
// 		default: Assert(!"mathobj type drawing not setup"); break;
// 	}
	
// 	if(term_text){
// 		if(HasFlag(mathobj->flags, TermFlag_DanglingClosingParenToRight)){
// 			term_text = ToString8(deshi_temp_allocator, term_text, STR8(")"));
// 		}
		
// 		//create the mathobj text item
// 		uiItem* term_item = uiTextMS(&debug_draw_term_tree_context.term_style, term_text);
// 		term_item->id = STR8("HELLO!");
		
// 		//binary insertion sort
		
// 	}
// #undef ctx
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @canvas


void init_canvas(){
	f32 world_height  = WorldViewArea().y;
	// default_graph.element.height  = world_height / 2.f;
	// default_graph.element.width   = world_height / 2.f;
	// default_graph.element.y       =  default_graph.element.height / 2.f;
	// default_graph.element.x       = -default_graph.element.width  / 2.f;
	// default_graph.element.type    = ElementType_Graph;
	// default_graph.cartesian_graph = ui_graph_make_cartesian();
	// default_graph.cartesian_graph->x_axis_label = str8_lit("x");
	// default_graph.cartesian_graph->y_axis_label = str8_lit("y");
	// elements.add(&default_graph.element);
	
	//library_load(STR8("test.slib"));

	array_init(canvas.element.arr, 4, deshi_allocator);

#if 0
	//debug testing ui
	//maximized, this runs at around 500-600 fps
	
	const u32 n = 5;
	static uiItem* items[n];
	uiItem* inside;
	
	forI(n){
		items[i] = uiItemB();
		items[i]->style.sizing = size_auto;
		items[i]->style.padding = {5,5,5,5};
		items[i]->style.background_color = {100, 50, u8(255 * f32(i)/n), 255};
	}
	
	inside = uiItemM();
	inside->style.size = {10,10};
	inside->action_trigger = action_act_always;
	inside->style.positioning = pos_absolute;
	inside->action = [](uiItem* item){
		item->style.pos = {BoundTimeOsc(0, 80), BoundTimeOsc(0,80)};
	};
	
	forI(n){
		uiItemE();
	}
	
	items[0]->style.positioning = pos_fixed;
	items[0]->style.pos = {200,200};
	
#endif
	
	canvas.ui.font.math = assets_font_create_from_path(str8_lit("data/fonts/STIXTwoMath-Regular.otf"), 100);
	Assert(canvas.ui.font.math != assets_font_null(), "Canvas math font failed to load");
	canvas.ui.font.debug = assets_font_create_from_path(str8l("data/fonts/gohufont-11.bdf"), 11);
	Assert(canvas.ui.font.debug != assets_font_null(), "Canvas debug font failed to load");

	canvas.ui.root = ui_begin_item(0);
	canvas.ui.root->id = STR8("suugu.canvas");
	canvas.ui.root->style.sizing = size_percent; // canvas is always the size of the window
	canvas.ui.root->style.size = {100,100};

	canvas.ui.debug = ui_begin_item(0);
	canvas.ui.debug->id = str8l("suugu.canvas.debug");
	{uiStyle* s = &canvas.ui.debug->style;
		s->sizing = size_auto;
		s->background_color = Color_Black;
		s->font = canvas.ui.font.debug;
		s->font_height = 11;
		s->text_color = Color_White;
		s->text_wrap = text_wrap_word;
		s->border_style = border_solid;
		s->border_color = Color_White;
		s->border_width = 1;
		s->padding_left = 5;
		s->padding_top = 5;
		s->padding_right = 5;
		s->padding_bottom = 5;
		//s->padding = {5,5,5,5};
		s->positioning = pos_draggable_relative;
		s->display = display_horizontal;
	}
	ui_end_item(); // debug
	ui_end_item(); // root
}

void update_canvas(){
 	//-///////////////////////////////////////////////////////////////////////////////////////////////
 	//// @input
 	canvas.world.mouse_pos = ToWorld(input_mouse_position());
	
	//// @input_tool ////
	if      (key_pressed(CanvasBind_SetTool_Navigation) && canvas.tool.active != CanvasTool_Navigation){
		canvas.tool.previous = canvas.tool.active;
		canvas.tool.active   = CanvasTool_Navigation;
	}else if(key_pressed(CanvasBind_SetTool_Context)    && canvas.tool.active != CanvasTool_Context){
		canvas.tool.previous = canvas.tool.active;
		canvas.tool.active   = CanvasTool_Context;
	}else if(key_pressed(CanvasBind_SetTool_Expression) && canvas.tool.active != CanvasTool_Expression){
		canvas.tool.previous = canvas.tool.active;
		canvas.tool.active   = CanvasTool_Expression;
	}else if(key_pressed(CanvasBind_SetTool_Pencil)     && canvas.tool.active != CanvasTool_Pencil){
		canvas.tool.previous = canvas.tool.active;
		canvas.tool.active   = CanvasTool_Pencil;
	}else if(key_pressed(CanvasBind_SetTool_Graph)){
		FixMe;
		//active_graph   = (active_graph) ? 0 : &default_graph;
	}else if(key_pressed(CanvasBind_SetTool_Previous)){
		Swap(canvas.tool.previous, canvas.tool.active);
	}
	
	//// @input_camera ////
	if(key_pressed(CanvasBind_Camera_Pan)){
		canvas.camera.pan_active = true;
		canvas.camera.pan_mouse_pos = input_mouse_position();
		canvas.camera.pan_start_pos = canvas.camera.pos;
	}
	if(key_down(CanvasBind_Camera_Pan)){
		canvas.camera.pos = canvas.camera.pan_start_pos + (ToWorld(canvas.camera.pan_mouse_pos) - canvas.world.mouse_pos);
	}
	if(key_released(CanvasBind_Camera_Pan)){
		canvas.camera.pan_active = false;
	}
	if(DeshInput->scrollY != 0 && input_mods_down(InputMod_None) && g_ui->hovered == canvas.ui.root){
		//if(!active_graph){
			canvas.camera.zoom -= canvas.camera.zoom / 10.0 * DeshInput->scrollY;
			canvas.camera.zoom = Clamp(canvas.camera.zoom, 1e-37, 1e37);
		//}else{
			//active_graph->cartesian_graph->camera_zoom -= 0.2 * active_graph->cartesian_graph->camera_zoom * DeshInput->scrollY;
		//}
	}
	
#if 1 //NOTE temp ui
	
	if(canvas.tool.active == CanvasTool_Pencil){
		ui_begin_immediate_branch(canvas.ui.debug); {
			uiItem* item = ui_begin_item(0);
			item->id = str8l("suugu.canvas.debug.pencil");
			item->style.sizing = size_auto;
			item->style.border_color = Color_White;
			item->style.border_style = border_solid;
			item->style.border_width = 1;
			{
				uiText* text = (uiText*)ui_make_text(str8null, 0);
				str8 out = to_dstr8v(deshi_temp_allocator, 
					"stroke size:   ", canvas.tool.pencil.stroke.size, "\n",
					"stroke color:  ", canvas.tool.pencil.stroke.color, "\n",
					"stroke start:  ", canvas.tool.pencil.stroke.start_pos, "\n",
					"stroke index:  ", canvas.tool.pencil.stroke.idx, "\n",
					"stroke skip:   ", canvas.tool.pencil.skip_amount, "\n"
				).fin;
				text_insert_string(&text->text, out);
				if(canvas.tool.pencil.stroke.idx) {
					text_insert_string(&text->text, to_dstr8v(deshi_temp_allocator, 
						"stroke points: ", array_count(array_last(canvas.tool.pencil.strokes)->pencil_points)
					).fin);
				}
				u32 total_points = 0;
				forX_array(stroke, canvas.tool.pencil.strokes){
					total_points += array_count(stroke->pencil_points);
				}
				text_insert_string(&text->text, to_dstr8v(deshi_temp_allocator,
					"total points: ", total_points
				).fin);
			}
			ui_end_item();
		}ui_end_immediate_branch();
	}
	if(canvas.tool.active == CanvasTool_Expression){
		ui_begin_immediate_branch(canvas.ui.debug); {
			uiItem* item = ui_begin_item(0);
			item->id = str8l("suugu.canvas.debug.expression");
			item->style.sizing = size_auto;
			item->style.border_color = Color_Grey;
			item->style.border_style = border_solid;
			item->style.border_width = 1;
			item->style.padding = {5,5,5,5};

			// TODO(sushi) fix the deshi_ui_allocator bug and then use it here
			ui_make_text(to_dstr8v(deshi_allocator, "elements: ", array_count(canvas.element.arr)).fin, 0);
			if(canvas.element.selected) {
				uiItem* text = ui_make_text(to_dstr8v(deshi_temp_allocator,
					"selected: ", canvas.element.selected, "\n",
					"position: ", canvas.element.selected->pos, "\n",
					"size:     ", canvas.element.selected->size, "\n"
				).fin, 0);
				text->id = str8l("suugu.canvas.debug.expression.selected_text");
			}
			ui_end_item();
		}ui_end_immediate_branch();	

		// UI::Begin(str8_lit("expression_debug"), {200,10}, {200,200}, UIWindowFlags_FitAllElements);
		// UI::TextF(str8_lit("Elements: %d"), elements.count);
		// if(selected_element){
		// 	UI::TextF(str8_lit("Selected: %#x"), selected_element);
		// 	UI::TextF(str8_lit("Position: (%g,%g)"), selected_element->x,selected_element->y);
		// 	UI::TextF(str8_lit("Size:     (%g,%g)"), selected_element->width,selected_element->height);
		// 	UI::TextF(str8_lit("Cursor:   %d"), (selected_element) ? ((Expression*)selected_element)->raw_cursor_start : 0);
		// }
		// UI::End();
	}
	//if(active_graph){
		// UI::Begin(str8_lit("graph_debug"), {200,10}, {200,200}, UIWindowFlags_FitAllElements);
		// UI::TextOld(str8_lit("Graph Info"));
		// UI::TextF(str8_lit("Element Pos:   (%g,%g)"), active_graph->element.x,active_graph->element.y);
		// UI::TextF(str8_lit("Element Size:  (%g,%g)"), active_graph->element.width,active_graph->element.height);
		// UI::TextF(str8_lit("Camera Pos:    (%g,%g)"), active_graph->cartesian_graph->camera_position.x,active_graph->cartesian_graph->camera_position.y);
		// UI::TextF(str8_lit("Camera Zoom:   %g"),      active_graph->cartesian_graph->camera_zoom);
		// UI::TextF(str8_lit("Dims per Unit: (%g,%g)"), active_graph->cartesian_graph->unit_length.x,active_graph->cartesian_graph->unit_length.y);
		// UI::End();
	//}
#endif
	
	switch(canvas.tool.active){
		//// @input_navigation ////
		case CanvasTool_Navigation: if(g_ui->hovered == canvas.ui.root){
			if(key_pressed(CanvasBind_Navigation_Pan)){
				canvas.camera.pan_active = true;
				canvas.camera.pan_mouse_pos = input_mouse_position();
				// if(!active_graph){
				 	canvas.camera.pan_start_pos = canvas.camera.pos;
				// }else{
				//	canvas.camera.pan_start_pos = vec2f64{active_graph->cartesian_graph->camera_position.x, active_graph->cartesian_graph->camera_position.y};
				//}
			}
			if(key_down(CanvasBind_Navigation_Pan)){
				//if(!active_graph){
					canvas.camera.pos = canvas.camera.pan_start_pos + (ToWorld(canvas.camera.pan_mouse_pos) - canvas.world.mouse_pos);
				// }else{
				// 	active_graph->cartesian_graph->camera_position.x = (canvas.camera.pan_start_pos.x + (canvas.camera.pan_mouse_pos.x - DeshInput->mouseX) / active_graph->cartesian_graph->unit_length.x);
				// 	active_graph->cartesian_graph->camera_position.y = (canvas.camera.pan_start_pos.y + (canvas.camera.pan_mouse_pos.y - DeshInput->mouseY) / active_graph->cartesian_graph->unit_length.y);
				// }
			}
			if(key_released(CanvasBind_Navigation_Pan)){
				canvas.camera.pan_active = false;
			}
			if(key_pressed(CanvasBind_Navigation_ResetPos)){
				//if(!active_graph){
					canvas.camera.pos = {0,0};
				// }else{
				// 	active_graph->cartesian_graph->camera_position = vec2g{0,0};
				// }
			}
			if(key_pressed(CanvasBind_Navigation_ResetZoom)){
				//if(!active_graph){
					canvas.camera.zoom = 1.0;
				// }else{
				// 	active_graph->cartesian_graph->camera_zoom = 5.0;
				// }
			}
		}break;
		
		//// @input_context ////
		case CanvasTool_Context:{
			NotImplemented;
			//if(UI::BeginContextMenu("canvas_context_menu")){
			//UI::EndContextMenu();
			//}
		}break;
		
		//// @input_expression ////
		case CanvasTool_Expression: {
			if(key_pressed(CanvasBind_Expression_Select)){
				canvas.element.selected = 0;
				//TODO inverse the space transformation here since mouse pos is screen space, which is less precise being
				//  elevated to higher precision, instead of higher precision world space getting transformed to screen space
				for_array(canvas.element.arr){
					Element* element = *it;
					if(Math::PointInRectangle(canvas.world.mouse_pos, element->pos.toVec2(), element->size.toVec2())){
						canvas.element.selected = element;
						break;
					}
				}
			}
			
			if(key_pressed(CanvasBind_Expression_Create)){
				Element* element = create_element();
				*array_push(canvas.element.arr) = element;
				element->type = ElementType_Expression;
				element->x      = canvas.world.mouse_pos.x;
				element->y      = canvas.world.mouse_pos.y;
				element->height = (320*canvas.camera.zoom) / (f32)canvas.ui.root->width;
				element->width  = element->height / 2.0;
				element->type   = ElementType_Expression;
				
				ui_push_item(canvas.ui.root);{
					element->item = ui_make_item(0);
					element->item->id = to_dstr8v(deshi_allocator, "suugu.canvas.element", array_count(canvas.element.arr)).fin;  // TODO(sushi) fix the error with deshi_ui_allocator's resize and then use it here
					element->item->style = element_default_style;
					element->item->style.sizing = size_auto;
					element->item->style.font = canvas.ui.font.math;
					element->item->style.font_height = 100;
					element->item->style.pos = {100,100}; //{element->x, element->y};
					element->item->__generate = render_element;
					element->item->__evaluate = evaluate_element;
					element->item->style.background_color = Color_Grey;
					element->item->userVar = (u64)element;
				}ui_pop_item(1);
				
				element->expression.handle.term_cursor_start = &element->expression.handle.root;
				element->expression.handle.raw_cursor_start  = 1;
				dstr8_init(&element->expression.handle.raw, str8l(""), deshi_allocator);
				element->expression.handle.root.raw = text_init(str8l(""), deshi_allocator);
				array_init(element->expression.position_map.x, 1, deshi_allocator);
				array_init(element->expression.position_map.y, 1, deshi_allocator);
				array_init(element->expression.rendered_parts, 1, deshi_allocator);
				
				*array_push(canvas.element.arr) = element;
				canvas.element.selected = element;
			}

			
			if(any_key_down() && canvas.element.selected){
				ast_input(&canvas.element.selected->expression.handle);
			}
			
			// if(canvas.element.selected && canvas.element.selected->type == ElementType_Expression){
			// 	Expression* expr = &canvas.element.selected->expression;
			// 	expr->changed = false;
				
			// 	//// @input_expression_cursor ////
			// 	if(expr->raw_cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorLeft)){
			// 		expr->raw_cursor_start -= 1;
			// 	}
			// 	if(expr->raw_cursor_start < expr->raw.count && key_pressed(CanvasBind_Expression_CursorRight)){
			// 		expr->raw_cursor_start += 1;
			// 	}
			// 	if(expr->raw_cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorWordLeft)){
			// 		if(*(expr->raw.str+expr->raw_cursor_start-1) == ')'){
			// 			while(expr->raw_cursor_start > 1 && *(expr->raw.str+expr->raw_cursor_start-1) != '('){
			// 				expr->raw_cursor_start -= 1;
			// 			}
			// 			if(*(expr->raw.str+expr->raw_cursor_start-1) == '(') expr->raw_cursor_start -= 1;
			// 		}else if(ispunct(*(expr->raw.str+expr->raw_cursor_start-1)) && *(expr->raw.str+expr->raw_cursor_start-1) != '.'){
			// 			expr->raw_cursor_start -= 1;
			// 		}else{
			// 			while(expr->raw_cursor_start > 1 && (isalnum(*(expr->raw.str+expr->raw_cursor_start-1)) || *(expr->raw.str+expr->raw_cursor_start-1) == '.')){
			// 				expr->raw_cursor_start -= 1;
			// 			}
			// 		}
			// 	}
			// 	if(expr->raw_cursor_start < expr->raw.count && key_pressed(CanvasBind_Expression_CursorWordRight)){
			// 		if(*(expr->raw.str+expr->raw_cursor_start) == '('){
			// 			while(expr->raw_cursor_start < expr->raw.count && *(expr->raw.str+expr->raw_cursor_start) != ')'){
			// 				expr->raw_cursor_start += 1;
			// 			}
			// 			if(*(expr->raw.str+expr->raw_cursor_start) == ')') expr->raw_cursor_start += 1;
			// 		}else if(ispunct(*(expr->raw.str+expr->raw_cursor_start)) && *(expr->raw.str+expr->raw_cursor_start) != '.'){
			// 			expr->raw_cursor_start += 1;
			// 		}else{
			// 			while(expr->raw_cursor_start < expr->raw.count && (isalnum(*(expr->raw.str+expr->raw_cursor_start)) || *(expr->raw.str+expr->raw_cursor_start) == '.')){
			// 				expr->raw_cursor_start += 1;
			// 			}
			// 		}
			// 	}
			// 	if(key_pressed(CanvasBind_Expression_CursorHome)){
			// 		expr->raw_cursor_start = 1;
			// 	}
			// 	if(key_pressed(CanvasBind_Expression_CursorEnd)){
			// 		expr->raw_cursor_start = expr->raw.count;
			// 	}
				
				
			// 	/*
			// 	if(expr->term_cursor_start->left && key_pressed(CanvasBind_Expression_CursorLeft)){
			// 		expr->term_cursor_start    = expr->term_cursor_start->left;
			// 		expr->raw_cursor_start     = expr->term_cursor_start->raw.str - expr->raw.str;
			// 		if(HasFlag(expr->term_cursor_start->flags, TermFlag_DanglingClosingParenToRight)){
			// 			expr->raw_cursor_start  += 1;
			// 			expr->right_paren_cursor = true;
			// 		}else{
			// 			expr->right_paren_cursor = false;
			// 		}
			// 	}
			// 	if(key_pressed(CanvasBind_Expression_CursorRight)){
			// 		if(HasFlag(expr->term_cursor_start->flags, TermFlag_DanglingClosingParenToRight)){
			// 			expr->raw_cursor_start  += 1;
			// 			expr->right_paren_cursor = true;
			// 		}else if(expr->term_cursor_start->right){
			// 			expr->term_cursor_start  = expr->term_cursor_start->right;
			// 			expr->raw_cursor_start   = expr->term_cursor_start->raw.str - expr->raw.str;
			// 			expr->right_paren_cursor = false;
			// 		}
			// 	}
			// 	*/
				
			// 	//character based input (letters, numbers, symbols)
			// 	//// @input_expression_insertion ////
			// 	if(DeshInput->charCount){
			// 		expr->changed = true;
			// 		dstr8_insert_byteoffset(&expr->raw, expr->raw_cursor_start, str8{DeshInput->charIn, DeshInput->charCount});
			// 		expr->raw_cursor_start += DeshInput->charCount;
			// 		Log("", expr->raw.fin);
			// 	}
				
			// 	//// @input_expression_deletion ////
			// 	if(expr->raw_cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorDeleteLeft)){
			// 		expr->changed = true;
			// 		dstr8_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start-1);
			// 		expr->raw_cursor_start -= 1;
			// 	}
			// 	if(expr->raw_cursor_start < expr->raw.count-1 && key_pressed(CanvasBind_Expression_CursorDeleteRight)){
			// 		expr->changed = true;
			// 		dstr8_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start);
			// 	}
			// 	if(expr->raw_cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorDeleteWordLeft)){
			// 		expr->changed = true;
			// 		if(*(expr->raw.str+expr->raw_cursor_start-1) == ')'){
			// 			while(expr->raw_cursor_start > 1 && *(expr->raw.str+expr->raw_cursor_start-1) != '('){
			// 				dstr8_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start-1);
			// 				expr->raw_cursor_start -= 1;
			// 			}
			// 			if(*(expr->raw.str+expr->raw_cursor_start-1) == '('){
			// 				dstr8_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start-1);
			// 				expr->raw_cursor_start -= 1;
			// 			}
			// 		}else if(ispunct(*(expr->raw.str+expr->raw_cursor_start-1)) && *(expr->raw.str+expr->raw_cursor_start-1) != '.'){
			// 			dstr8_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start-1);
			// 			expr->raw_cursor_start -= 1;
			// 		}else{
			// 			while(expr->raw_cursor_start > 1 && (isalnum(*(expr->raw.str+expr->raw_cursor_start-1)) || *(expr->raw.str+expr->raw_cursor_start-1) == '.')){
			// 				dstr8_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start-1);
			// 				expr->raw_cursor_start -= 1;
			// 			}
			// 		}
			// 	}
			// 	if(expr->raw_cursor_start < expr->raw.count-1 && key_pressed(CanvasBind_Expression_CursorDeleteWordRight)){
			// 		expr->changed = true;
			// 		if(*(expr->raw.str+expr->raw_cursor_start) == '('){
			// 			while(expr->raw_cursor_start < expr->raw.count && *(expr->raw.str+expr->raw_cursor_start) != ')'){
			// 				dstr8_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start);
			// 			}
			// 			if(*(expr->raw.str+expr->raw_cursor_start) == ')') dstr8_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start);
			// 		}else if(ispunct(*(expr->raw.str+expr->raw_cursor_start)) && *(expr->raw.str+expr->raw_cursor_start) != '.'){
			// 			dstr8_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start);
			// 		}else{
			// 			while(expr->raw_cursor_start < expr->raw.count && (isalnum(*(expr->raw.str+expr->raw_cursor_start)) || *(expr->raw.str+expr->raw_cursor_start) == '.')){
			// 				dstr8_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start);
			// 			}
			// 		}
			// 	}
				

			// 	if(expr->changed){
			// 		expr->valid = parse(expr);
			// 		solve(&expr->root);
			// 		debug_draw_term_tree(expr, &expr->root);
			// 		debug_print_term(&expr->root);
			// 	}
			// }
		}break;
		case CanvasTool_Pencil:{}break;
	}
		
// 		////////////////////////////////////////////////////////////////////////////////////////////////
// 		//// @input_pencil
// 		case CanvasTool_Pencil: if(!UI::AnyWinHovered()){
// 			if(key_pressed(CanvasBind_Pencil_Stroke)){
// 				PencilStroke new_stroke;
// 				new_stroke.size  = pencil_stroke_size;
// 				new_stroke.color = pencil_stroke_color;
// 				pencil_strokes.add(new_stroke);
// 				pencil_stroke_start_pos = mouse_pos_world;
// 			}
// 			if(key_down(CanvasBind_Pencil_Stroke)){
// 				pencil_strokes[pencil_stroke_idx].pencil_points.add(mouse_pos_world);
// 			}
// 			if(key_released(CanvasBind_Pencil_Stroke)){
// 				pencil_stroke_idx += 1;
// 			}
// 			if(key_pressed(CanvasBind_Pencil_DeletePrevious)){ 
// 				if(pencil_strokes.count){
// 					pencil_strokes.pop();
// 					if(pencil_stroke_idx) pencil_stroke_idx -= 1;
// 				}
// 			}
// 			if     (DeshInput->scrollY > 0 && key_down(Key_LSHIFT)){ pencil_stroke_size += 1; }
// 			else if(DeshInput->scrollY > 0 && key_down(Key_LCTRL)) { pencil_stroke_size += 5; }
// 			else if(DeshInput->scrollY < 0 && key_down(Key_LSHIFT)){ pencil_stroke_size -= 1; }
// 			else if(DeshInput->scrollY < 0 && key_down(Key_LCTRL)) { pencil_stroke_size -= 5; }
// 			pencil_stroke_size = ((pencil_stroke_size < 1) ? 1 : ((pencil_stroke_size > 100) ? 100 : (pencil_stroke_size)));
// 			if     (key_pressed(CanvasBind_Pencil_DetailIncrementBy1)){ pencil_draw_skip_amount -= 1; }
// 			else if(key_pressed(CanvasBind_Pencil_DetailIncrementBy5)){ pencil_draw_skip_amount -= 5; }
// 			else if(key_pressed(CanvasBind_Pencil_DetailDecrementBy1)){ pencil_draw_skip_amount += 1; }
// 			else if(key_pressed(CanvasBind_Pencil_DetailDecrementBy5)){ pencil_draw_skip_amount += 5; }
// 			pencil_draw_skip_amount = Clamp(pencil_draw_skip_amount, 1, 100);
// 		}break;
// 	}
	
// 	//// @draw_elements ////
// 	for(Element* el : elements){
// 		switch(el->type){
// 			///////////////////////////////////////////////////////////////////////////////////////////////
// 			//// @draw_elements_expression
// 			case ElementType_Expression:{
// 				Expression* expr = ElementToExpression(el);
// 				UIWindow* window = 0;
// 				UI::PushFont(math_font);
// 				UI::PushScale(vec2::ONE * el->height / camera_zoom * 2.0);
// 				UI::PushColor(UIStyleCol_Border, (el == selected_element) ? Color_Yellow : Color_White);
// 				UI::PushVar(UIStyleVar_WindowMargins,    vec2{5,5});
// 				UI::PushVar(UIStyleVar_FontHeight,       80);
// 				UI::SetNextWindowPos(ToScreen(el->x, el->y));
// 				string s = stringf(deshi_temp_allocator, "expression_0x%p",el);
// 				UI::Begin(str8{(u8*)s.str, (s64)s.count}, vec2::ZERO, vec2::ZERO, UIWindowFlags_NoInteract|UIWindowFlags_FitAllElements);{
// 					window = UI::GetWindow();
					
// 					vec2 cursor_start; f32 cursor_y;
// 					persist b32 tog = 0;
// 					if(key_pressed(Key_UP)) ToggleBool(tog);
// 					if(tog){
// 						draw_term_old(expr, &expr->mathobj, cursor_start, cursor_y);
// 					}else{
// 						//NOTE(sushi): drawinfo initialization is temporarily done outside the draw_term function and ideally will be added back later
// 						//             or we make a drawinfo struct and pass it in to (possibly) support parallelizing this
// 						drawinfo.drawCmd     = UIDrawCmd();
// 						drawinfo.initialized = true;
// 						drawinfo.item        = UI::BeginCustomItem();
// 						draw_term(expr, &expr->mathobj);
// 						UI::EndCustomItem();
// 						drawinfo.initialized = false;
// 						//if(expr->raw.str){
// 						//	UI::SetNextItemActive();
// 						//	UI::InputText("textrenderdebugdisplay", expr->raw.str, expr->raw.count, 0, UIInputTextFlags_FitSizeToText | UIInputTextFlags_NoEdit);
// 						//	UI::GetInputTextState("textrenderdebugdisplay")->cursor = expr->cursor_start;
// 						//}
// 					}
					
// 					//draw cursor
// 					if(selected_element == el){
// 						UI::Line(cursor_start, cursor_start + vec2{0,cursor_y}, 2, Color_White * abs(sin(DeshTime->totalTime / 1000.f)));
// 					}
// 				}UI::End();
// 				UI::PopVar(2);
// 				UI::PopColor();
				
// 				//draw AST
// 				if(selected_element == el && DEBUG_draw_term_simple_){
// 					UI::SetNextWindowPos(window->x, window->y + window->height);
// 					debug_draw_term_simple(&expr->mathobj);
// 				}
// 				UI::PopScale();
// 				UI::PopFont();
// 			}break;
			
// 			///////////////////////////////////////////////////////////////////////////////////////////////
// 			//// @draw_elements_graph
// 			case ElementType_Graph:{
// 				GraphElement* ge = ElementToGraphElement(el);
// 				vec2 tl = ToScreen(ge->element.x, ge->element.y);
// 				vec2 br = ToScreen(ge->element.x + ge->element.width, ge->element.y - ge->element.height);
// 				ge->cartesian_graph->item.style.pos  = tl;
// 				ge->cartesian_graph->item.style.size = vec2_subtract(br, tl);
// 			}break;
			
// 			///////////////////////////////////////////////////////////////////////////////////////////////
// 			//// @draw_elements_workspace
// 			//case ElementType_Workspace:{}break;
			
// 			///////////////////////////////////////////////////////////////////////////////////////////////
// 			//// @draw_elements_text
// 			//case ElementType_Text:{}break;
			
// 			default:{
// 				NotImplemented;
// 			}break;
// 		}
// 	}
	
// 	//// @draw_pencil //// //TODO smooth line drawing
// 	UI::Begin(str8_lit("pencil_layer"), vec2::ZERO, Vec2(DeshWindow->width,DeshWindow->height), UIWindowFlags_Invisible | UIWindowFlags_NoInteract);
// 	//UI::PushScale(vec2::ONE * camera_zoom * 2.0);
// 	forE(pencil_strokes){
// 		if(it->pencil_points.count > 1){
			
// 			//arrayT<vec2> pps(it->pencil_points.count);
// 			//forI(it->pencil_points.count) pps.add(ToScreen(it->pencil_points[i]));
// 			//Render::DrawLines2D(pps, it->size / camera_zoom, it->color, 4, vec2::ZERO, DeshWindow->dimensions);
			
// 			UI::CircleFilled(ToScreen(it->pencil_points[0]), it->size/2.f, 16, it->color);
// 			for(int i = 1; i < it->pencil_points.count; ++i){
// 				UI::CircleFilled(ToScreen(it->pencil_points[i]), it->size/2.f, 16, it->color);
// 				UI::Line(ToScreen(it->pencil_points[i-1]), ToScreen(it->pencil_points[i]), it->size, it->color);
// 			}
// 		}
// 	}
// 	//UI::PopScale();
// 	UI::End();
	
// 	//// @draw_canvas_info ////
// 	UI::TextF(str8_lit("%.3f FPS"), F_AVG(50, 1 / (DeshTime->deltaTime / 1000)));
// 	UI::TextF(str8_lit("Active Tool:   %s"), canvas_tool_strings[canvas.tool.active]);
// 	UI::TextF(str8_lit("Previous Tool: %s"), canvas_tool_strings[previous_tool]);
// 	UI::TextF(str8_lit("Selected Element: %d"), u64(selected_element));
// 	UI::TextF(str8_lit("campos:  (%g, %g)"),camera_pos.x,camera_pos.y);
// 	UI::TextF(str8_lit("camzoom: %g"), camera_zoom);
// 	UI::TextF(str8_lit("camarea: (%g, %g)"), WorldViewArea().x, WorldViewArea().y);
// 	UI::TextF(str8_lit("graph active: %s"), (active_graph) ? "true" : "false");
	
// 	UI::End();
// 	UI::PopVar();
}
