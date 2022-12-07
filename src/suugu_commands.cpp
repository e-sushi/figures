void init_suugu_commands(){
#define SUUGU_CMD_START(name, desc) suugu__last_cmd_desc = str8_lit(desc); auto suugu__cmd__##name = [](str8* args, u32 arg_count) -> void
#define SUUGU_CMD_END_NO_ARGS(name) ; cmd_add(suugu__cmd__##name, str8_lit(#name), suugu__last_cmd_desc, 0, 0)
#define SUUGU_CMD_END(name, ...) ; local Type deshi__cmd__##name##args[] = {__VA_ARGS__}; cmd_add(suugu__cmd__##name, str8_lit(#name), suugu__last_cmd_desc, suugu__cmd__##name##args, ArrayCount(suugu__cmd__##name##args))
	
	
	str8 suugu__last_cmd_desc;
	
	
	SUUGU_CMD_START(show_ast_tree, "Toggles the visibility of the debug AST tree visualizer"){
		ToggleBool(debug_draw_term_tree_context.visible);
		if(debug_draw_term_tree_context.expression){
			debug_draw_term_tree_context.expression->style.display = (debug_draw_term_tree_context.visible) ? 0 : display_hidden;
		}
	}SUUGU_CMD_END_NO_ARGS(show_ast_tree);
	
	SUUGU_CMD_START(show_ast_tree_simple, "Toggles the visibility of the debug AST tree visualizer"){
		//TODO(delle) remove this once show_ast_tree is functional
		ToggleBool(DEBUG_draw_term_simple_);
	}SUUGU_CMD_END_NO_ARGS(show_ast_tree_simple);
	
	SUUGU_CMD_START(print_ast_tree, "Toggles the debug AST tree logging"){
		ToggleBool(DEBUG_print_term_);
	}SUUGU_CMD_END_NO_ARGS(print_ast_tree);
	
	
#undef SUUGU_CMD_START
#undef SUUGU_CMD_END_NO_ARGS
#undef SUUGU_CMD_END
}
