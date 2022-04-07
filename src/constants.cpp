map<str8, Constant> constants;

void load_constants(){
	File f = open_file(toStr(Assets::dirData() + "constants.txt").str, FileAccess_Read);
	cstring data = read_file(f);
	str8 wdata = {(u8*)data.str, 0};
	defer{memzfree(data.str);};
	
	enum : u32{
		ReadNames,
		ReadValue,
		ReadUnits,
		SkipLine,
	};

	u32 state = ReadNames;
	while(wdata.str < ((u8*)data.str + data.count)){
		DecodedCodepoint dcp = decoded_codepoint_from_utf8(wdata.str, 4);
		u16 ch;  
		utf16_from_codepoint(&ch, dcp.codepoint);
		wchar ok = (wchar)ch;
		switch(state){
			case ReadNames:{
				if(ch==u'#'){
					state = SkipLine;
					continue;
				}
			}break;
		}
		wdata.str += dcp.advance;
	}
	
   
}