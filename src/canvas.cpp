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


////////////////////
//// @draw_term ////
////////////////////
struct DrawContext{
	vec2        bbx; // the bounding box formed by child nodes
	f32     midline; // 
	Vertex2* vstart; // we must save these for the parent node to readjust what the child node makes
	u32*     istart;
	u32 vcount, icount;
};

struct{ //information for the current instance of draw_term, this will need to be its own thing if we ever do multithreading here somehow
	// TODO(sushi) convert to new ui
	//UIItem_old* item;
	//UIDrawCmd drawCmd = UIDrawCmd(1);
	vec2 cursor_start;
	f32 cursor_y;
	b32 initialized = false;
}drawinfo;

struct{
	f32  additive_padding = 5;                 //padding between + or - and it's operands
	f32  multiplication_explicit_padding = 3;  //padding between * and it's operands
	f32  multiplication_implicit_padding = 3;  //padding between implicit multiplication operands
	f32  division_padding = 0;                 //padding between division's line and it's operands
	f32  division_scale = 0.8;                 //how much to scale division's operands by 
	f32  division_line_overreach = 3;          //how many pixels of over reach the division line has in both directions
	f32  division_line_thickness = 3;          //how thick the division line is 
	vec2 exponential_offset = Vec2(-4,-10);    //offset of exponent
	f32  exponential_scaling = 0.7;            //amount to scale the exponent by
}drawcfg;

//NOTE(sushi): in this setup we are depth-first drawing things and readjusting in parent nodes
//			   this means the memory is organized backwards and when we readjust we just interate from the start of
//			   the drawCmd's vertices to the overall count
//NOTE(sushi): we also dont abide by UIDRAWCMD_MAX_* macros here either, which may cause issues later. this is due 
//             to the setup described above and avoids chunking the data of UIDrawCmds and making readjusting them awkward
//NOTE(delle): the cursor is drawn to the right of the character it represents
//TODO(sushi) remove the expr arg and make it part of drawinfo
//TODO(sushi) copy how an empty expression looks from draw_term_old()
DrawContext draw_term(Expression* expr, Term* term){DPZoneScoped;
	FixMe;
	// using namespace UI;
	// DPTracyDynMessage(toStr("initialized: ", drawinfo.initialized));
	// if(term == 0) return DrawContext();
	// if(!drawinfo.initialized) return DrawContext();
	// //initializing internally has some issues so for now drawinfo must be initialized before calling this function
	// //if(!drawinfo.initialized){
	// //	drawinfo.item = BeginCustomItem();
	// //	drawinfo.drawCmd = UIDrawCmd();
	// //	drawinfo.initialized = true;
	// //	drawinfo.item->position = GetWinCursor();
	// //}
	// //drawcfg.additive_padding = 10 * (sin(DeshTotalTime/1000)+1)/2;
	// //drawcfg.division_padding = 10 * (sin(DeshTotalTime/1000)+1)/2;
	// //drawcfg.multiplication_explicit_padding = 10 * (sin(DeshTotalTime/1000)+1)/2;
	
	// UIItem_old* item   = drawinfo.item; //:)
	// UIDrawCmd& drawCmd = drawinfo.drawCmd;
	// UIStyle_old style  = GetStyle();
	// f32 fontHeight = style.fontHeight;
	// DrawContext drawContext;
	// drawContext.vcount = 0;
	// drawContext.vstart = drawCmd.vertices + u32(drawCmd.counts.x);
	// drawContext.istart = drawCmd.indices  + u32(drawCmd.counts.y);
	
	// const color textColor = style.colors[UIStyleCol_Text];
	// const vec2  textScale = vec2::ONE * style.fontHeight / (f32)style.font->max_height;
	// const vec2  spaceSize = CalcTextSize(str8l(" "));
	
	// //this function checks that the shape we are about to add to the drawcmd does not overrun its buffers
	// //if it will we just add the drawcmd to the item and make a new one
	// auto check_drawcmd = [&](u32 vcount, u32 icount){
	// 	if(drawCmd.counts.x + vcount > UIDRAWCMD_MAX_VERTICES || drawCmd.counts.y + icount > UIDRAWCMD_MAX_INDICES){
	// 		CustomItem_AddDrawCmd(item, drawCmd);
	// 		drawCmd.vertices = (Vertex2*)memtrealloc(drawCmd.vertices, drawCmd.counts.x*2);
	// 		drawCmd.indices  = (u32*)memtrealloc(drawCmd.indices, drawCmd.counts.y*2);
	// 		drawContext.vstart = drawCmd.vertices;
	// 		drawContext.istart = drawCmd.indices;
	// 	}
	// };
	
	// auto debug_rect = [&](vec2 pos, vec2 size){
	// 	check_drawcmd(8,24);
	// 	drawContext.vcount += 8;
	// 	drawContext.icount += 24;
	// 	CustomItem_DCMakeRect(drawCmd, pos, size, 1, Color_Red);
	// };
	
	// auto debug_line = [&](vec2 start, vec2 end){
	// 	check_drawcmd(4,6);
	// 	drawContext.vcount += 4;
	// 	drawContext.icount += 6;
	// 	CustomItem_DCMakeLine(drawCmd, start, end, 1, Color_Cyan);
	// };
	
	// switch(term->type){
	// 	case TermType_Expression:{
	// 		Expression* expr = ExpressionFromTerm(term);
	// 		vec2 mmbbx = vec2::ZERO; //expression bounding box
			
	// 		//draw leading space
	// 		check_drawcmd(4,6);
	// 		CustomItem_DCMakeText(drawCmd, str8l(" "), vec2::ZERO, textColor, textScale);
	// 		mmbbx.x += spaceSize.x;
	// 		mmbbx.y  = Max(mmbbx.y, spaceSize.y);
			
	// 		//draw right paren if its the cursor character
	// 		if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
	// 			str8 rightParen = str8l(")");
	// 			vec2 rightParenSize = CalcTextSize(rightParen);
	// 			check_drawcmd(4,6);
	// 			u32 voffset = (u32)drawCmd.counts.x;
	// 			CustomItem_DCMakeText(drawCmd, rightParen, vec2::ZERO, textColor, textScale);
	// 			drawCmd.vertices[voffset+0].pos.x += mmbbx.x;
	// 			drawCmd.vertices[voffset+1].pos.x += mmbbx.x;
	// 			drawCmd.vertices[voffset+2].pos.x += mmbbx.x;
	// 			drawCmd.vertices[voffset+3].pos.x += mmbbx.x;
	// 			mmbbx.x += rightParenSize.x;
	// 			mmbbx.y  = Max(mmbbx.y, rightParenSize.y);
	// 		}
			
	// 		if(term->child_count){
	// 			//draw first term
	// 			drawContext = draw_term(expr, term->first_child);
	// 			forI(drawContext.vcount){
	// 				(drawContext.vstart + i)->pos.x += mmbbx.x;
	// 			}
	// 			mmbbx.x += drawContext.bbx.x;
	// 			mmbbx.y = Max(mmbbx.y, drawContext.bbx.y);
				
	// 			//draw other terms separated by spaces if the expression is invalid
	// 			for_node(term->first_child->next){
	// 				check_drawcmd(4,6);
	// 				CustomItem_DCMakeText(drawCmd, str8l(" "), vec2::ZERO, textColor, textScale);
	// 				mmbbx.x += spaceSize.x;
	// 				mmbbx.y  = Max(mmbbx.y, spaceSize.y);
					
	// 				drawContext = draw_term(expr, it);
	// 				forI(drawContext.vcount){
	// 					(drawContext.vstart + i)->pos.x += mmbbx.x;
	// 				}
	// 				mmbbx.x += drawContext.bbx.x;
	// 				mmbbx.y = Max(mmbbx.y, drawContext.bbx.y);
	// 			}
	// 		}
			
	// 		//draw solution if it's valid
	// 		//TODO this
			
	// 		//draw trailing space
	// 		check_drawcmd(4,6);
	// 		CustomItem_DCMakeText(drawCmd, str8l(" "), vec2::ZERO, textColor, textScale);
	// 		mmbbx.x += spaceSize.x;
	// 		mmbbx.y  = Max(mmbbx.y, spaceSize.y);
			
	// 		//expression is the topmost node so drawing will always be finished when it finishes (i hope)
	// 		item->size = mmbbx;
	// 		CustomItem_AdvanceCursor(item);
	// 		CustomItem_AddDrawCmd(item, drawCmd);
			
	// 		//EndCustomItem();
	// 		//drawinfo.initialized = false;			
	// 	}break;
		
	// 	case TermType_Operator:{
	// 		switch(term->op_type){
	// 			case OpType_Parentheses:{
	// 				str8 syml = str8_lit("(");
	// 				str8 symr = str8_lit(")");
	// 				f32  ratio = 1; //ratio of parenthesis to drawn child nodes over y
	// 				vec2 symsize = CalcTextSize(syml); // i sure hope theres no font with different sizes for these
	// 				drawContext.vstart = drawCmd.vertices + u32(drawCmd.counts.x);
	// 				drawContext.istart = drawCmd.indices  + u32(drawCmd.counts.y);
	// 				if(term->child_count == 1){
	// 					DrawContext ret = draw_term(expr, term->first_child);
	// 					ratio = ret.bbx.y / symsize.y;
	// 					forI(ret.vcount){
	// 						(ret.vstart + i)->pos.x += symsize.x;
	// 					}
	// 					drawContext.bbx.x = ret.bbx.x + 2*symsize.x;
	// 					drawContext.bbx.y = Max(symsize.y, ret.bbx.y);
	// 					drawContext.vcount = ret.vcount+8;
	// 					drawContext.icount = ret.icount+12;
	// 					check_drawcmd(8,12);
	// 					CustomItem_DCMakeText(drawCmd, syml, Vec2(0, 0), textColor, Vec2(1, ratio) * textScale);
	// 					if(HasFlag(term->flags, TermFlag_LeftParenHasMatchingRightParen)){
	// 						CustomItem_DCMakeText(drawCmd, symr, Vec2(symsize.x + ret.bbx.x, 0), textColor, Vec2(1, ratio) * textScale);
	// 					}
	// 					else{
	// 						CustomItem_DCMakeText(drawCmd, symr, Vec2(symsize.x + ret.bbx.x, 0), textColor * 0.3, Vec2(1, ratio) * textScale);
	// 					}
	// 					drawContext.midline = ret.midline;
	// 				}
	// 				else if(!term->child_count){
	// 					drawContext.bbx.x = symsize.x*2;
	// 					drawContext.bbx.y = symsize.y;
	// 					drawContext.vcount = 8;
	// 					drawContext.icount = 12;
	// 					drawContext.midline = drawContext.bbx.y/2;
	// 					check_drawcmd(8,12);
	// 					CustomItem_DCMakeText(drawCmd, syml, Vec2(0, (drawContext.bbx.y - symsize.y) / 2), textColor, Vec2(1, ratio) * textScale);
	// 					CustomItem_DCMakeText(drawCmd, symr, Vec2(symsize.x, (drawContext.bbx.y - symsize.y) / 2), textColor, Vec2(1, ratio) * textScale);
	// 				}
	// 				else{
	// 					Log("", "Parenthesis has more than 1 child");
	// 				}
					
	// 			}break;
				
	// 			case OpType_Exponential:{
	// 				if(term->child_count == 2){
	// 					DrawContext retl = draw_term(expr, term->first_child);
	// 					DrawContext retr = draw_term(expr, term->last_child);
	// 					retr.bbx *= drawcfg.exponential_scaling;
	// 					forI(retr.vcount){
	// 						(retr.vstart + i)->pos.x *= drawcfg.exponential_scaling;
	// 						(retr.vstart + i)->pos.y *= drawcfg.exponential_scaling;
	// 						(retr.vstart + i)->pos.x += retl.bbx.x + drawcfg.exponential_offset.x;
	// 					}
	// 					forI(retl.vcount){
	// 						(retl.vstart + i)->pos.y += retr.bbx.y + drawcfg.exponential_offset.y;
	// 					}
	// 					drawContext.bbx.x = retl.bbx.x + retr.bbx.x + drawcfg.exponential_offset.x;
	// 					drawContext.bbx.y = retl.bbx.y + retr.bbx.y + drawcfg.exponential_offset.y;
	// 					drawContext.vcount = retl.vcount + retr.vcount;
	// 					drawContext.midline = retl.bbx.y/2 + retr.bbx.y + drawcfg.exponential_offset.y;
	// 				}
	// 				else if(term->child_count == 1){
	// 					vec2 size = Vec2(style.fontHeight*style.font->aspect_ratio,style.fontHeight);
	// 					DrawContext ret = draw_term(expr, term->first_child);
	// 					if(HasFlag(term->first_child->flags, TermFlag_OpArgLeft)){
	// 						forI(ret.vcount){
	// 							(ret.vstart + i)->pos.y += size.y*drawcfg.exponential_scaling + drawcfg.exponential_offset.y;
	// 						}
	// 						check_drawcmd(4,6);
	// 						CustomItem_DCMakeFilledRect(drawCmd, Vec2(ret.bbx.x+drawcfg.exponential_offset.x, 0), size*drawcfg.exponential_scaling, Color_DarkGrey);
	// 						drawContext.vcount = ret.vcount + 4;
	// 						drawContext.bbx.x = ret.bbx.x + size.x * drawcfg.exponential_scaling + drawcfg.exponential_offset.x;
	// 						drawContext.bbx.y = ret.bbx.y + size.y * drawcfg.exponential_scaling + drawcfg.exponential_offset.y;
	// 						drawContext.midline = ret.bbx.y/2 + size.y*drawcfg.exponential_scaling + drawcfg.exponential_offset.y; 
	// 					}
	// 					else if(HasFlag(term->first_child->flags, TermFlag_OpArgRight)){
	// 						Assert(!"The AST should not support placing a ^ when it's not preceeded by a valid term");
	// 					}
						
	// 				}
	// 				else if(!term->child_count){
	// 					Assert(!"why did this happen");
	// 				}
	// 			}break;
				
	// 			case OpType_Negation:{
	// 				str8 sym = str8_lit("-");
	// 				vec2 symsize = CalcTextSize(sym);
	// 				if(term->child_count == 1){
	// 					DrawContext ret = draw_term(expr, term->first_child);
	// 					forI(ret.vcount){
	// 						(ret.vstart + i)->pos.x += symsize.x;
	// 					}
	// 					drawContext.bbx.x = ret.bbx.x + symsize.x;
	// 					drawContext.bbx.y = Max(ret.bbx.y, symsize.y);
	// 					drawContext.vcount += ret.vcount + 4;
	// 					check_drawcmd(4,6);
	// 					CustomItem_DCMakeText(drawCmd, sym, vec2::ZERO, textColor, textScale);
	// 				}
	// 				else if(!term->child_count){
	// 					drawContext.bbx = symsize;
	// 					drawContext.vcount = 4;
	// 					check_drawcmd(4,6);
	// 					CustomItem_DCMakeText(drawCmd, sym, vec2::ZERO, textColor, textScale);
	// 				}
	// 				else Assert(!"unary op has more than 1 child");
	// 				return drawContext;
	// 			}break;
				
	// 			case OpType_ImplicitMultiplication:{
	// 				if(term->child_count == 2){
	// 					DrawContext retl = draw_term(expr, term->first_child);
	// 					DrawContext retr = draw_term(expr, term->last_child);
	// 					vec2 refbbx = Max(retl.bbx,retr.bbx);
	// 					forI(retl.vcount){
	// 						(retl.vstart + i)->pos.y += (refbbx.y - retl.bbx.y)/2;
	// 					}
	// 					forI(retr.vcount){
	// 						(retr.vstart + i)->pos.x += retl.bbx.x + drawcfg.multiplication_implicit_padding;
	// 						(retr.vstart + i)->pos.y += (refbbx.y - retr.bbx.y) / 2;
	// 					}
	// 					drawContext.vcount = retl.vcount + retr.vcount;
	// 					drawContext.bbx.x = retl.bbx.x+retr.bbx.x+drawcfg.multiplication_implicit_padding;
	// 					drawContext.bbx.y = Max(retl.bbx.y, retr.bbx.y);
	// 				}
	// 				else{
	// 					Assert(!"please tell me (sushi) if this happens");
	// 				}
	// 			}break;
				
	// 			case OpType_ExplicitMultiplication:{
	// 				f32 radius = 4;
	// 				if(term->child_count == 2){
	// 					DrawContext retl = draw_term(expr, term->first_child);
	// 					DrawContext retr = draw_term(expr, term->last_child);
	// 					vec2 refbbx = Max(retl.bbx,retr.bbx);
	// 					forI(retl.vcount){
	// 						(retl.vstart + i)->pos.y += (refbbx.y - retl.bbx.y)/2;
	// 					}
	// 					forI(retr.vcount){
	// 						(retr.vstart + i)->pos.x += retl.bbx.x + 2*radius + drawcfg.multiplication_explicit_padding*2;
	// 						(retr.vstart + i)->pos.y += (refbbx.y - retr.bbx.y) / 2;
	// 					}
	// 					drawContext.vcount = retl.vcount + retr.vcount + render_make_circle_counts(15).x;
	// 					drawContext.bbx.x = retl.bbx.x + retr.bbx.r + 2*radius + 2*drawcfg.multiplication_explicit_padding;
	// 					drawContext.bbx.y = Max(retl.bbx.y, Max(retr.bbx.y, radius*2));
	// 					check_drawcmd(render_make_circle_counts(15).x,render_make_circle_counts(15).y);
	// 					CustomItem_DCMakeFilledCircle(drawCmd, Vec2(retl.bbx.x+drawcfg.multiplication_explicit_padding, drawContext.bbx.y/2), radius, 15, textColor); 
	// 				}
	// 				else if(term->child_count == 1){
	// 					DrawContext ret = draw_term(expr, term->first_child);
	// 					drawContext.bbx = Vec2(ret.bbx.x+radius*2+drawcfg.additive_padding*2, Max(radius*2, ret.bbx.y));
	// 					if(HasFlag(term->first_child->flags, TermFlag_OpArgLeft)){
	// 						check_drawcmd(render_make_circle_counts(15).x,render_make_circle_counts(15).y);
	// 						CustomItem_DCMakeFilledCircle(drawCmd, Vec2(ret.bbx.x+drawcfg.multiplication_explicit_padding, drawContext.bbx.y/2), radius, 15, textColor);
	// 					}
	// 					else if(HasFlag(term->first_child->flags, TermFlag_OpArgRight)){
	// 						forI(ret.vcount){
	// 							(ret.vstart + i)->pos.x += radius*2+drawcfg.additive_padding*2;
	// 						}
	// 						check_drawcmd(render_make_circle_counts(15).x,render_make_circle_counts(15).y);
	// 						CustomItem_DCMakeFilledCircle(drawCmd, Vec2(radius+drawcfg.multiplication_explicit_padding, drawContext.bbx.y/2), radius, 15, textColor);
	// 					}
	// 					drawContext.vcount = ret.vcount + render_make_circle_counts(15).x;
	// 				}
	// 				else if(!term->child_count){
	// 					drawContext.bbx = Vec2(style.fontHeight*style.font->aspect_ratio,style.fontHeight);
	// 					check_drawcmd(render_make_circle_counts(15).x,render_make_circle_counts(15).y);
	// 					drawContext.vcount = render_make_circle_counts(15).x;
	// 					CustomItem_DCMakeFilledCircle(drawCmd, Vec2(radius+drawcfg.multiplication_explicit_padding, drawContext.bbx.y/2), radius, 15, textColor);
	// 				}
	// 			}break;
				
	// 			case OpType_Division:{
	// 				if(term->child_count == 2){
	// 					DrawContext retl = draw_term(expr, term->first_child);
	// 					DrawContext retr = draw_term(expr, term->last_child);
	// 					retl.bbx *= drawcfg.division_scale;
	// 					retr.bbx *= drawcfg.division_scale; 
	// 					vec2 refbbx = Max(retl.bbx, retr.bbx);
	// 					refbbx.x += drawcfg.division_line_overreach*2;
	// 					for(Vertex2* v = retl.vstart; v != retr.vstart; v++){
	// 						v->pos.x *= drawcfg.division_scale;
	// 						v->pos.y *= drawcfg.division_scale;
	// 						v->pos.x += (refbbx.x - retl.bbx.x) / 2; 
	// 					}
	// 					forI(retr.vcount){
	// 						(retr.vstart + i)->pos.x *= drawcfg.division_scale;
	// 						(retr.vstart + i)->pos.y *= drawcfg.division_scale;
	// 						(retr.vstart + i)->pos.x += (refbbx.x - retr.bbx.x) / 2;
	// 						(retr.vstart + i)->pos.y += retl.bbx.y + drawcfg.division_padding;
	// 					}
	// 					drawContext.vcount = retl.vcount + retr.vcount + 4;
	// 					drawContext.bbx = Vec2(refbbx.x, retl.bbx.y+drawcfg.division_padding+retr.bbx.y);
	// 					check_drawcmd(4,6);
	// 					f32 liney = retl.bbx.y+(drawcfg.division_padding-drawcfg.division_line_thickness/2);
	// 					drawContext.midline = liney;
	// 					CustomItem_DCMakeLine(drawCmd, Vec2(0, liney),  Vec2(drawContext.bbx.x, liney), drawcfg.division_line_thickness, textColor);
	// 				}
	// 				else if(term->child_count == 1){
	// 					DrawContext ret = draw_term(expr, term->first_child);
	// 					drawContext.bbx = Vec2(ret.bbx.x+drawcfg.division_line_overreach*2, ret.bbx.y*2+drawcfg.division_padding);
	// 					f32 liney = (drawContext.bbx.y-drawcfg.division_line_thickness)/2;
	// 					drawContext.midline = liney;
	// 					if(HasFlag(term->first_child->flags, TermFlag_OpArgLeft)){
	// 						forI(ret.vcount){
	// 							(ret.vstart + i)->pos.x += (drawContext.bbx.x-ret.bbx.x)/2;
	// 						}
	// 						check_drawcmd(4,6);
	// 						CustomItem_DCMakeLine(drawCmd, Vec2(0, liney),Vec2(drawContext.bbx.x, liney), drawcfg.division_line_thickness, textColor);
	// 					}
	// 					else if(HasFlag(term->first_child->flags, TermFlag_OpArgRight)){
	// 						forI(ret.vcount){
	// 							(ret.vstart + i)->pos.x += (drawContext.bbx.x-ret.bbx.x)/2;
	// 							(ret.vstart + i)->pos.y += drawContext.bbx.y-ret.bbx.y;
	// 						}
	// 						check_drawcmd(4,6);
	// 						CustomItem_DCMakeLine(drawCmd, Vec2(0, liney),Vec2(drawContext.bbx.x, liney), drawcfg.division_line_thickness, textColor);
	// 					}
	// 					drawContext.vcount = ret.vcount + 4;
	// 					drawContext.icount = ret.icount + 6;
	// 				}
	// 				else if(!term->child_count){
	// 					drawContext.bbx = Vec2(style.fontHeight*style.font->aspect_ratio+2*drawcfg.division_line_overreach, style.fontHeight*2+drawcfg.division_padding);
	// 					drawContext.vcount = 4;
	// 					drawContext.icount = 6;
	// 					drawContext.midline = drawContext.bbx.y / 2;
	// 					check_drawcmd(4,6);
	// 					CustomItem_DCMakeLine(drawCmd, Vec2(0, drawContext.bbx.y/2),Vec2(drawContext.bbx.x, drawContext.bbx.y/2), 1, textColor);
	// 				}
	// 			}break;
				
	// 			case OpType_Modulo:
	// 			case OpType_Addition:
	// 			case OpType_Subtraction:{
	// 				str8 sym;
	// 				if(term->op_type == OpType_Addition)         sym = STR8("+");
	// 				else if(term->op_type == OpType_Subtraction) sym = STR8("−");
	// 				else if(term->op_type == OpType_Modulo)      sym = STR8("%");
	// 				vec2 symsize = CalcTextSize(sym);
					
	// 				//this can maybe be a switch
	// 				//both children exist so proceed normally
	// 				if(term->child_count == 2){
	// 					DrawContext retl = draw_term(expr, term->first_child);
	// 					DrawContext retr = draw_term(expr, term->last_child);
	// 					b32 leftdominant = retl.midline > retr.midline;
	// 					f32 maxmidl = (leftdominant ? retl.midline : retr.midline);
	// 					vec2 refbbx = Max(retl.bbx,retr.bbx);
	// 					f32 loffset = retr.midline - retl.midline;
	// 					f32 roffset = -loffset;
	// 					//if the left side isnt the largest we dont need to worry about adjusting the left side at all
	// 					if(!leftdominant)
	// 						forI(retl.vcount)
	// 					(retl.vstart + i)->pos.y += loffset;
						
	// 					forI(retr.vcount){
	// 						(retr.vstart + i)->pos.x += retl.bbx.x + symsize.x + drawcfg.additive_padding*2;
	// 						if(leftdominant) 
	// 						(retr.vstart + i)->pos.y += roffset;
	// 					}
	// 					drawContext.bbx.x = retl.bbx.x+retr.bbx.x+symsize.x+drawcfg.additive_padding*2;
	// 					if(leftdominant){
	// 						drawContext.bbx.y = Max(retl.bbx.y, Max(retr.bbx.y+roffset, symsize.y));
	// 					}else{
	// 						drawContext.bbx.y = Max(retl.bbx.y+loffset, Max(retr.bbx.y, symsize.y));
	// 					}
	// 					//drawContext.bbx = Vec2(retl.bbx.x+retr.bbx.x+symsize.x+drawcfg.additive_padding*2, Max(retl.bbx.y, Max(retr.bbx.y, symsize.y)));
	// 					drawContext.vcount = retl.vcount + retr.vcount + 4;
	// 					drawContext.icount = retl.icount + retr.icount + 6; 
	// 					drawContext.midline = maxmidl;
	// 					check_drawcmd(4,6);
	// 					CustomItem_DCMakeText(drawCmd, sym, Vec2(retl.bbx.x+drawcfg.additive_padding, maxmidl - symsize.y/2), textColor, textScale);
	// 				}
	// 				//operator has a first child but it isnt followed by anything
	// 				else if(term->child_count == 1){
	// 					DrawContext ret = draw_term(expr, term->first_child);
	// 					drawContext.bbx = Vec2(ret.bbx.x+symsize.x+drawcfg.additive_padding*2, Max(symsize.y, ret.bbx.y));
	// 					if(HasFlag(term->first_child->flags, TermFlag_OpArgLeft)){
	// 						check_drawcmd(4,6);
	// 						CustomItem_DCMakeText(drawCmd, sym, Vec2(ret.bbx.x+drawcfg.additive_padding, ret.midline - symsize.y/2), textColor, textScale);
	// 					}
	// 					else if(HasFlag(term->first_child->flags, TermFlag_OpArgRight)){
	// 						forI(ret.vcount){
	// 							(ret.vstart + i)->pos.x += symsize.x+drawcfg.additive_padding*2;
	// 						}
	// 						check_drawcmd(4,6);
	// 						CustomItem_DCMakeText(drawCmd, sym, Vec2(0, ret.midline - symsize.y/2), textColor, textScale);
	// 					}
	// 					drawContext.vcount = ret.vcount + 4;
	// 					drawContext.icount = ret.icount + 6;
	// 				}
	// 				else if(!term->child_count){
	// 					drawContext.bbx = symsize;
	// 					drawContext.vcount = 4;
	// 					drawContext.icount = 6;
	// 					drawContext.midline = drawContext.bbx.y / 2;
	// 					check_drawcmd(4,6);
	// 					CustomItem_DCMakeText(drawCmd, sym, vec2::ZERO, textColor, textScale);
	// 				}
	// 				else Assert(!"binop has more than 2 children");
	// 			}break;
				
	// 			case OpType_ExpressionEquals:{
					
	// 			}break;
				
	// 			default: Assert(!"operator type drawing not setup"); break;
	// 		}
	// 		if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
				
	// 		}
	// 	}break;
		
	// 	case TermType_Literal:
	// 	case TermType_Variable:{
	// 		s64 term_raw_length = str8_length(term->raw);
	// 		drawContext.bbx = CalcTextSize(term->raw);
	// 		drawContext.vcount = term_raw_length * 4;
	// 		drawContext.icount = term_raw_length * 6;
	// 		drawContext.midline = drawContext.bbx.y / 2;
	// 		check_drawcmd(drawContext.vcount,drawContext.icount);
	// 		CustomItem_DCMakeText(drawCmd, term->raw, vec2::ZERO, textColor, textScale);
	// 	}break;
		
	// 	case TermType_FunctionCall:{
	// 		//TODO(sushi) support multi arg functions when implemented
	// 		if(term->first_child){
	// 			s64 term_raw_length = str8_length(term->raw);
	// 			DrawContext ret = draw_term(expr, term->first_child);
	// 			drawContext.vcount = term_raw_length * 4 + ret.vcount;
	// 			drawContext.icount = term_raw_length * 6 + ret.icount;
	// 			check_drawcmd(drawContext.vcount, drawContext.icount);
	// 			vec2 tsize = UI::CalcTextSize(term->raw);
	// 			forI(ret.vcount){
	// 				(ret.vstart + i)->pos.x += tsize.x;
	// 			}
	// 			CustomItem_DCMakeText(drawCmd, term->raw, Vec2(0, (ret.bbx.y-tsize.y)/2), textColor, textScale);
	// 			drawContext.bbx.x = tsize.x + ret.bbx.x;
	// 			drawContext.bbx.y = Max(tsize.y, ret.bbx.y);
	// 			drawContext.midline = drawContext.bbx.y / 2;
	// 		}else{
	// 			Assert(!"huh?");
	// 		}
	// 		return drawContext;
	// 	}break;
		
	// 	case TermType_Logarithm:{
			
	// 	}break;
		
	// 	default: LogE("exrend", "Custom rendering does not support term type:", OpTypeStrs(term->type)); break;//Assert(!"term type drawing not setup"); break;
	// }
	////debug_rect(vec2::ZERO, drawContext.bbx);
	////debug_line(Vec2(0, drawContext.midline), Vec2(drawContext.bbx.x, drawContext.midline));
	//return drawContext;
	return {};
}

//NOTE(delle) raw cursor is drawn to the left of the character
//NOTE(delle) term cursor is drawn to the right of the term
void draw_term_old(Expression* expr, Term* term, vec2& cursor_start, f32& cursor_y){
	FixMe;
// #define CursorAfterLastItem() (cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(), cursor_y = -UI::GetLastItemSize().y)
// #define CursorBeforeLastItem() (cursor_start = UI::GetLastItemPos(), cursor_y = UI::GetLastItemSize().y)
	
// 	if(term == 0) return;
// 	switch(term->type){
// 		case TermType_Expression:{
// 			Expression* expr = ExpressionFromTerm(term);
// 			UI::TextOld(str8_lit(" "), UITextFlags_NoWrap); UI::SameLine();
// 			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
// 				UI::TextOld(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
// 				if(expr->raw_cursor_start == 1) CursorBeforeLastItem();
// 			}
// 			if(term->child_count){
// 				draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 				for_node(term->first_child->next){
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
// 			switch(term->op_type){
// 				case OpType_Parentheses:{
// 					UI::TextOld(str8_lit("("), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == term->raw.str) CursorBeforeLastItem();
// 					if(term->first_child){
// 						draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 						for_node(term->first_child->next){
// 							UI::TextOld(str8_lit(" "), UITextFlags_NoWrap); UI::SameLine();
// 							draw_term_old(expr, it, cursor_start, cursor_y);
// 						}
// 					}
// 					if(HasFlag(term->flags, TermFlag_LeftParenHasMatchingRightParen)){
// 						UI::TextOld(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
// 						if(expr->raw.str + expr->raw_cursor_start == term->raw.str + term->raw.count) CursorBeforeLastItem();
// 					}
// 				}break;
				
// 				case OpType_Exponential:{
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("^"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == term->raw.str) CursorBeforeLastItem();
// 					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
// 				}break;
				
// 				case OpType_Negation:{
// 					UI::TextOld(str8_lit("-"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == term->raw.str) CursorBeforeLastItem();
// 					draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 				}break;
				
// 				case OpType_ImplicitMultiplication:{
// 					draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 					draw_term_old(expr, term->last_child, cursor_start, cursor_y);
// 				}break;
				
// 				case OpType_ExplicitMultiplication:{
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("*"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == term->raw.str) CursorBeforeLastItem();
// 					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
// 				}break;
// 				case OpType_Division:{
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("/"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == term->raw.str) CursorBeforeLastItem();
// 					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
// 				}break;
// 				case OpType_Modulo:{
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("%"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == term->raw.str) CursorBeforeLastItem();
// 					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
// 				}break;
				
// 				case OpType_Addition:{
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("+"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == term->raw.str) CursorBeforeLastItem();
// 					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
// 				}break;
// 				case OpType_Subtraction:{
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("-"), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == term->raw.str) CursorBeforeLastItem();
// 					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
// 				}break;
				
// 				case OpType_ExpressionEquals:{
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
// 					UI::TextOld(str8_lit("="), UITextFlags_NoWrap); UI::SameLine();
// 					if(expr->raw.str + expr->raw_cursor_start == term->raw.str) CursorBeforeLastItem();
// 					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
// 					if(term->last_child) for_node(term->last_child->next) draw_term_old(expr, it, cursor_start, cursor_y);
// 				}break;
				
// 				default: Assert(!"operator type drawing not setup"); break;
// 			}
// 			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
// 				UI::TextOld(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
// 				if(expr->raw.str + expr->raw_cursor_start == term->raw.str + term->raw.count) CursorBeforeLastItem();
// 			}
// 		}break;
		
// 		case TermType_Literal:
// 		case TermType_Variable:{
// 			//TODO italics for variables (make this an option)
// 			UI::TextOld(str8{(u8*)term->raw.str, (s64)term->raw.count}, UITextFlags_NoWrap); UI::SameLine();
// 			if((term->raw.str <= expr->raw.str + expr->raw_cursor_start) && (expr->raw.str + expr->raw_cursor_start < term->raw.str + term->raw.count)){
// 				f32 x_offset = UI::CalcTextSize(str8{(u8*)term->raw.str, s64(expr->raw.str + expr->raw_cursor_start - term->raw.str)}).x;
// 				cursor_start = UI::GetLastItemPos() + vec2{x_offset,0}; cursor_y = UI::GetLastItemSize().y;
// 			}
// 			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
// 				UI::TextOld(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
// 				if(expr->raw.str + expr->raw_cursor_start == term->raw.str + term->raw.count){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
// 			}
// 		}break;
		
// 		case TermType_FunctionCall:{
// 			UI::TextOld(str8{(u8*)term->raw.str, (s64)term->raw.count}, UITextFlags_NoWrap); UI::SameLine();
// 			if((term->raw.str <= expr->raw.str + expr->raw_cursor_start) && (expr->raw.str + expr->raw_cursor_start < term->raw.str + term->raw.count)){
// 				f32 x_offset = UI::CalcTextSize(str8{(u8*)term->raw.str, s64(expr->raw.str + expr->raw_cursor_start - term->raw.str)}).x;
// 				cursor_start = UI::GetLastItemPos() + vec2{x_offset,0}; cursor_y = UI::GetLastItemSize().y;
// 			}
// 			for_node(term->first_child) draw_term_old(expr, it, cursor_start, cursor_y);
// 		}break;
		
// 		case TermType_Logarithm:{
// 			UI::TextOld(str8{(u8*)term->raw.str, (s64)term->raw.count}, UITextFlags_NoWrap); UI::SameLine();
// 			if((term->raw.str <= expr->raw.str + expr->raw_cursor_start) && (expr->raw.str + expr->raw_cursor_start < term->raw.str + term->raw.count)){
// 				f32 x_offset = UI::CalcTextSize(str8{(u8*)term->raw.str, s64(expr->raw.str + expr->raw_cursor_start - term->raw.str)}).x;
// 				cursor_start = UI::GetLastItemPos() + vec2{x_offset,0}; cursor_y = UI::GetLastItemSize().y;
// 			}
// 			for_node(term->first_child) draw_term_old(expr, it, cursor_start, cursor_y);
// 		}break;
		
// 		default: Assert(!"term type drawing not setup"); break;
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

void debug_draw_term_tree(Expression* expr, Term* term){DPZoneScoped;
	FixMe;
// #define ctx debug_draw_term_tree_context
// 	auto highlight_border_when_focused = [](uiItem* item){
// 		if(g_ui->active == item){
// 			item->style.border_color = Color_Grey;
// 		}else{
// 			item->style.border_color = Color_VeryDarkGrey;
// 		}
// 	};
	
// 	if(term == 0) return;
	
// 	str8 term_text{};
// 	switch(term->type){
// 		case TermType_Expression:{
// 			//reset the context
// 			ctx.depth = 0;
// 			if(ctx.expression) uiItemR(ctx.expression);
			
// 			//fill the term style
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
// 			Expression* expr = ExpressionFromTerm(term);
// 			if(term->child_count){
// 				ctx.depth += 1;
// 				debug_draw_term_tree(expr, term->first_child);
// 				for_node(term->first_child->next){
// 					debug_draw_term_tree(expr, it);
// 				}
// 				ctx.depth -= 1;
// 			}
			
// 			//position the term items
			
			
// 			//draw lines between terms and their parents
			
			
// 			//end the expression container
// 			uiItemE();
// 		}break;
		
// 		case TermType_Operator:{
// 			switch(term->op_type){
// 				case OpType_Parentheses:{
// 					if(term->first_child){
// 						ctx.depth += 1;
// 						debug_draw_term_tree(expr, term->first_child);
// 						for_node(term->first_child->next){
// 							debug_draw_term_tree(expr, it);
// 						}
// 						ctx.depth -= 1;
// 					}
// 					term_text = STR8("(");
// 				}break;
// 				case OpType_Exponential:{
// 					ctx.depth += 1;
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, term->first_child);
// 					if(term->last_child  && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, term->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("^");
// 				}break;
// 				case OpType_Negation:{
// 					ctx.depth += 1;
// 					debug_draw_term_tree(expr, term->first_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("NEG");
// 				}break;
// 				case OpType_ImplicitMultiplication:{
// 					ctx.depth += 1;
// 					debug_draw_term_tree(expr, term->first_child);
// 					debug_draw_term_tree(expr, term->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("*i");
// 				}break;
// 				case OpType_ExplicitMultiplication:{
// 					ctx.depth += 1;
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, term->first_child);
// 					if(term->last_child  && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, term->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("*e");
// 				}break;
// 				case OpType_Division:{
// 					ctx.depth += 1;
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, term->first_child);
// 					if(term->last_child  && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, term->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("/");
// 				}break;
// 				case OpType_Modulo:{
// 					ctx.depth += 1;
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, term->first_child);
// 					if(term->last_child  && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, term->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("%");
// 				}break;
// 				case OpType_Addition:{
// 					ctx.depth += 1;
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, term->first_child);
// 					if(term->last_child  && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, term->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("+");
// 				}break;
// 				case OpType_Subtraction:{
// 					ctx.depth += 1;
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, term->first_child);
// 					if(term->last_child  && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, term->last_child);
// 					ctx.depth -= 1;
// 					term_text = STR8("-");
// 				}break;
// 				case OpType_ExpressionEquals:{
// 					ctx.depth += 1;
// 					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) debug_draw_term_tree(expr, term->first_child);
// 					if(term->last_child  && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) debug_draw_term_tree(expr, term->last_child);
// 					if(term->last_child) for_node(term->last_child->next) debug_draw_term_tree(expr, it);
// 					ctx.depth -= 1;
// 					term_text = STR8("=");
// 				}break;
// 				default: Assert(!"operator type drawing not setup"); break;
// 			}
// 		}break;
		
// 		case TermType_Literal:{
// 			term_text = ToString8(deshi_temp_allocator, term->lit_value);
// 		}break;
		
// 		case TermType_Variable:{
// 			term_text = term->raw;
// 		}break;
		
// 		case TermType_FunctionCall:{
// 			ctx.depth += 1;
// 			for_node(term->first_child) debug_draw_term_tree(expr, it);
// 			ctx.depth -= 1;
// 			term_text = term->func->text;
// 		}break;
		
// 		case TermType_Logarithm:{
// 			ctx.depth += 1;
// 			for_node(term->first_child) debug_draw_term_tree(expr, it);
// 			ctx.depth -= 1;
// 			term_text = ToString8(deshi_temp_allocator, STR8("log"), term->log_base);
// 		}break;
		
// 		default: Assert(!"term type drawing not setup"); break;
// 	}
	
// 	if(term_text){
// 		if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
// 			term_text = ToString8(deshi_temp_allocator, term_text, STR8(")"));
// 		}
		
// 		//create the term text item
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
	
	canvas.ui.font.math = assets_font_create_from_file(str8_lit("STIXTwoMath-Regular.otf"), 100);
	Assert(canvas.ui.font.math != assets_font_null(), "Canvas math font failed to load");
	canvas.ui.font.debug = assets_font_create_from_file(str8l("gohufont-11.bdf"), 11);
	Assert(canvas.ui.font.debug != assets_font_null(), "Canvas debug font failed to load");

	canvas.ui.root = uiItemB();
	canvas.ui.root->id = STR8("suugu.canvas");
	canvas.ui.root->style.sizing = size_percent; // canvas is always the size of the window
	canvas.ui.root->style.size = {100,100};

	canvas.ui.debug = uiItemB();
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
	uiItemE(); // debug
	uiItemE(); // root
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
		uiImmediateBP(canvas.ui.debug); {
			uiItem* item = uiItemB();
			item->id = str8l("suugu.canvas.debug.pencil");
			item->style.sizing = size_auto;
			item->style.border_color = Color_White;
			item->style.border_style = border_solid;
			item->style.border_width = 1;
			{
				uiText* text = uiGetText(uiTextML(""));
				str8 out = ToString8(deshi_temp_allocator, 
					"stroke size:   ", canvas.tool.pencil.stroke.size, "\n",
					"stroke color:  ", canvas.tool.pencil.stroke.color, "\n",
					"stroke start:  ", canvas.tool.pencil.stroke.start_pos, "\n",
					"stroke index:  ", canvas.tool.pencil.stroke.idx, "\n",
					"stroke skip:   ", canvas.tool.pencil.skip_amount, "\n"
				);
				text_insert_string(&text->text, out);
				if(canvas.tool.pencil.stroke.idx) {
					out = ToString8(deshi_temp_allocator, 
						"stroke points: ", array_count(array_last(canvas.tool.pencil.strokes)->pencil_points)
					);
					text_insert_string(&text->text, out);
				}
				u32 total_points = 0;
				forX_array(stroke, canvas.tool.pencil.strokes){
					total_points += array_count(stroke->pencil_points);
				}
				out = ToString8(deshi_temp_allocator,
					"total points: ", total_points
				);
				text_insert_string(&text->text, out);
			}
			uiItemE();
		}uiImmediateE();
	}
	if(canvas.tool.active == CanvasTool_Expression){
		uiImmediateBP(canvas.ui.debug); {
			uiItem* item = uiItemB();
			item->id = str8l("suugu.canvas.debug.expression");
			item->style.sizing = size_auto;
			item->style.border_color = Color_Grey;
			item->style.border_style = border_solid;
			item->style.border_width = 1;
			item->style.padding = {5,5,5,5};

			uiTextM(ToString8(deshi_temp_allocator, "elements: ", array_count(canvas.element.arr)));
			if(canvas.element.selected) {
				uiTextM(ToString8(deshi_temp_allocator,
					"selected: ", canvas.element.selected, "\n",
					"position: ", canvas.element.selected->pos, "\n",
					"size:     ", canvas.element.selected->size, "\n"
				));
			}
			uiItemE();
		}uiImmediateE();	

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
				Element* element = make_element();
				*array_push(canvas.element.arr) = element;
				element->type = ElementType_Expression;
				element->x         = canvas.world.mouse_pos.x;
				element->y         = canvas.world.mouse_pos.y;
				element->height    = (320*canvas.camera.zoom) / (f32)canvas.ui.root->width;
				element->width     = element->height / 2.0;
				element->type      = ElementType_Expression;
				element->item = uiItemM();
				element->item->id = ToString8(deshi_allocator, "suugu.canvas.expression", array_count(canvas.element.arr));
				element->item->style = element_default_style;
				element->expression.term_cursor_start = &element->expression.term;
				element->expression.raw_cursor_start  = 1;
				str8_builder_init(&element->expression.raw, str8l(" "), deshi_allocator);
				
				*array_push(canvas.element.arr) = element;
				canvas.element.selected = element;
			}
			
			if(canvas.element.selected && canvas.element.selected->type == ElementType_Expression){
				Expression* expr = &canvas.element.selected->expression;
				expr->changed = false;
				
				//// @input_expression_cursor ////
				
				if(expr->raw_cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorLeft)){
					expr->raw_cursor_start -= 1;
				}
				if(expr->raw_cursor_start < expr->raw.count && key_pressed(CanvasBind_Expression_CursorRight)){
					expr->raw_cursor_start += 1;
				}
				if(expr->raw_cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorWordLeft)){
					if(*(expr->raw.str+expr->raw_cursor_start-1) == ')'){
						while(expr->raw_cursor_start > 1 && *(expr->raw.str+expr->raw_cursor_start-1) != '('){
							expr->raw_cursor_start -= 1;
						}
						if(*(expr->raw.str+expr->raw_cursor_start-1) == '(') expr->raw_cursor_start -= 1;
					}else if(ispunct(*(expr->raw.str+expr->raw_cursor_start-1)) && *(expr->raw.str+expr->raw_cursor_start-1) != '.'){
						expr->raw_cursor_start -= 1;
					}else{
						while(expr->raw_cursor_start > 1 && (isalnum(*(expr->raw.str+expr->raw_cursor_start-1)) || *(expr->raw.str+expr->raw_cursor_start-1) == '.')){
							expr->raw_cursor_start -= 1;
						}
					}
				}
				if(expr->raw_cursor_start < expr->raw.count && key_pressed(CanvasBind_Expression_CursorWordRight)){
					if(*(expr->raw.str+expr->raw_cursor_start) == '('){
						while(expr->raw_cursor_start < expr->raw.count && *(expr->raw.str+expr->raw_cursor_start) != ')'){
							expr->raw_cursor_start += 1;
						}
						if(*(expr->raw.str+expr->raw_cursor_start) == ')') expr->raw_cursor_start += 1;
					}else if(ispunct(*(expr->raw.str+expr->raw_cursor_start)) && *(expr->raw.str+expr->raw_cursor_start) != '.'){
						expr->raw_cursor_start += 1;
					}else{
						while(expr->raw_cursor_start < expr->raw.count && (isalnum(*(expr->raw.str+expr->raw_cursor_start)) || *(expr->raw.str+expr->raw_cursor_start) == '.')){
							expr->raw_cursor_start += 1;
						}
					}
				}
				if(key_pressed(CanvasBind_Expression_CursorHome)){
					expr->raw_cursor_start = 1;
				}
				if(key_pressed(CanvasBind_Expression_CursorEnd)){
					expr->raw_cursor_start = expr->raw.count;
				}
				
				
				/*
				if(expr->term_cursor_start->left && key_pressed(CanvasBind_Expression_CursorLeft)){
					expr->term_cursor_start    = expr->term_cursor_start->left;
					expr->raw_cursor_start     = expr->term_cursor_start->raw.str - expr->raw.str;
					if(HasFlag(expr->term_cursor_start->flags, TermFlag_DanglingClosingParenToRight)){
						expr->raw_cursor_start  += 1;
						expr->right_paren_cursor = true;
					}else{
						expr->right_paren_cursor = false;
					}
				}
				if(key_pressed(CanvasBind_Expression_CursorRight)){
					if(HasFlag(expr->term_cursor_start->flags, TermFlag_DanglingClosingParenToRight)){
						expr->raw_cursor_start  += 1;
						expr->right_paren_cursor = true;
					}else if(expr->term_cursor_start->right){
						expr->term_cursor_start  = expr->term_cursor_start->right;
						expr->raw_cursor_start   = expr->term_cursor_start->raw.str - expr->raw.str;
						expr->right_paren_cursor = false;
					}
				}
				*/
				
				//character based input (letters, numbers, symbols)
				//// @input_expression_insertion ////
				if(DeshInput->charCount){
					expr->changed = true;
					str8_builder_insert_byteoffset(&expr->raw, expr->raw_cursor_start, str8{DeshInput->charIn, DeshInput->charCount});
					expr->raw_cursor_start += DeshInput->charCount;
				}
				
				//// @input_expression_deletion ////
				if(expr->raw_cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorDeleteLeft)){
					expr->changed = true;
					str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start-1);
					expr->raw_cursor_start -= 1;
				}
				if(expr->raw_cursor_start < expr->raw.count-1 && key_pressed(CanvasBind_Expression_CursorDeleteRight)){
					expr->changed = true;
					str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start);
				}
				if(expr->raw_cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorDeleteWordLeft)){
					expr->changed = true;
					if(*(expr->raw.str+expr->raw_cursor_start-1) == ')'){
						while(expr->raw_cursor_start > 1 && *(expr->raw.str+expr->raw_cursor_start-1) != '('){
							str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start-1);
							expr->raw_cursor_start -= 1;
						}
						if(*(expr->raw.str+expr->raw_cursor_start-1) == '('){
							str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start-1);
							expr->raw_cursor_start -= 1;
						}
					}else if(ispunct(*(expr->raw.str+expr->raw_cursor_start-1)) && *(expr->raw.str+expr->raw_cursor_start-1) != '.'){
						str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start-1);
						expr->raw_cursor_start -= 1;
					}else{
						while(expr->raw_cursor_start > 1 && (isalnum(*(expr->raw.str+expr->raw_cursor_start-1)) || *(expr->raw.str+expr->raw_cursor_start-1) == '.')){
							str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start-1);
							expr->raw_cursor_start -= 1;
						}
					}
				}
				if(expr->raw_cursor_start < expr->raw.count-1 && key_pressed(CanvasBind_Expression_CursorDeleteWordRight)){
					expr->changed = true;
					if(*(expr->raw.str+expr->raw_cursor_start) == '('){
						while(expr->raw_cursor_start < expr->raw.count && *(expr->raw.str+expr->raw_cursor_start) != ')'){
							str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start);
						}
						if(*(expr->raw.str+expr->raw_cursor_start) == ')') str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start);
					}else if(ispunct(*(expr->raw.str+expr->raw_cursor_start)) && *(expr->raw.str+expr->raw_cursor_start) != '.'){
						str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start);
					}else{
						while(expr->raw_cursor_start < expr->raw.count && (isalnum(*(expr->raw.str+expr->raw_cursor_start)) || *(expr->raw.str+expr->raw_cursor_start) == '.')){
							str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->raw_cursor_start);
						}
					}
				}
				
				if(expr->changed){
					expr->valid = parse(expr);
					solve(&expr->term);
					debug_draw_term_tree(expr, &expr->term);
					debug_print_term(&expr->term);
				}
			}
		}break;
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
// 						draw_term_old(expr, &expr->term, cursor_start, cursor_y);
// 					}else{
// 						//NOTE(sushi): drawinfo initialization is temporarily done outside the draw_term function and ideally will be added back later
// 						//             or we make a drawinfo struct and pass it in to (possibly) support parallelizing this
// 						drawinfo.drawCmd     = UIDrawCmd();
// 						drawinfo.initialized = true;
// 						drawinfo.item        = UI::BeginCustomItem();
// 						draw_term(expr, &expr->term);
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
// 					debug_draw_term_simple(&expr->term);
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