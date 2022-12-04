void init_suugu_commands(){
#define SUUGU_CMD_START(name, desc) suugu__last_cmd_desc = str8_lit(desc); auto suugu__cmd__##name = [](str8* args, u32 arg_count) -> void
#define SUUGU_CMD_END_NO_ARGS(name) ; cmd_add(suugu__cmd__##name, str8_lit(#name), suugu__last_cmd_desc, 0, 0)
#define SUUGU_CMD_END(name, ...) ; local Type deshi__cmd__##name##args[] = {__VA_ARGS__}; cmd_add(suugu__cmd__##name, str8_lit(#name), suugu__last_cmd_desc, suugu__cmd__##name##args, ArrayCount(suugu__cmd__##name##args))
	
	
	str8 suugu__last_cmd_desc;
	
	
#undef SUUGU_CMD_START
#undef SUUGU_CMD_END_NO_ARGS
#undef SUUGU_CMD_END
}
