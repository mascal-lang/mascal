#ifndef ASSEMBLY_LEXER_HPP
#define ASSEMBLY_LEXER_HPP

#include <string>
#include <vector>
#include <sstream>

enum X86AssemblyToken {

	X86EndOfFile = -1,
	X86Identifier = -2,
	X86Number = -3,

	X86Add = -4,
	X86Sub = -5,
	X86Xor = -6,
	X86Inc = -7,
	X86Mov = -8,
	X86Return = -9,
	X86Cmp = -10,
	X86Test = -11,
	X86Jmp = -12,

	X86Push = -13,
	X86Pop = -14,
	X86Lea = -15,
	X86Call = -16,

	X86Text = -17,
	X86Def = -18,
	X86Globl = -19,
	X86Set = -20,
	X86File = -21,
	X86P2Align = -22,

	X86String = -23,

	X86SEH = -24,
	X86SEHEnd = -25,
	X86SEHSetFrame = -26,
};

struct X86AssemblyLexer {

	static std::string Content;

	static std::string IdentifierStr;
	static std::string NumValString;
	static std::string StringString;

	static void AddContent(std::string c) {

		Content += c;
	}

	static int CurrentToken;
	static int Position;

	static int Line;
	static int Column;

	static std::string line_as_string;

	static std::vector<std::string> all_lines_vector;

	static int LastChar;

	static void Start() {

		Position = -1;
		Line = 1;
		Column = 1;
		LastChar = ' ';
	}

	static int Advance() {

		Position += 1;
		//Column += 1;

		//line_as_string += Content[Position];

		if (Content[Position] == '\n')
		{
			//Line += 1;
			//Column = 1;

			//all_lines_vector.push_back(line_as_string);

			//line_as_string.clear();
		}

		return Content[Position];
	}

	static void GoTo(int place) {

		if(place < Content.size()) {

			Position = place;
		}
	}

	static void GetNextToken() {

		CurrentToken = GetToken();
	}

	static int GetToken() {

		while (isspace(LastChar)) LastChar = Advance();

		if(isalpha(LastChar) || LastChar == '%' || LastChar == '_' || LastChar == '.' || LastChar == '@') return GetIdentifier();

		if(isdigit(LastChar) || LastChar == '$' || LastChar == '-') return GetNumber();

		if(LastChar == '\"') return GetString();

		if (LastChar == '#')
		{
			// Comment until end of line.
			do
			{
				LastChar = Advance();
			}
			while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

			if (LastChar != EOF) return GetToken();
		}

		if (LastChar == EOF) return X86AssemblyToken::X86EndOfFile;

		int ThisChar = LastChar;
		LastChar = Advance();

		// This is a fail-safe in case memory corruption appears.
		// Since at this point, we're looking for normal characters,
		// it makes no sense to find characters that are below space in
		// the ASCII table. Meaning that if we find one like that at this point,
		// its undefined behavior.
		if (ThisChar < 32) ThisChar = X86AssemblyToken::X86EndOfFile;

		return ThisChar;
	}

	static bool IsIdentifier(std::string s)
	{
		return IdentifierStr == s;
	}

	static bool IdentifierContains(std::string s) {

		return IdentifierStr.find(s) != std::string::npos;
	}

	static bool is_still_identifier(char c)
	{
		return isalnum(c) || c == '_' || c == '.';
	}

	static bool IsMultipleBitIdentifier(std::string s) {

		std::string bit8version = s + "b";
		std::string bit16version = s + "w";
		std::string bitlong32version = s + "l";
		std::string bit32version = s + "s";
		std::string bit64version = s + "q";
		std::string bit80version = s + "t";

		return IsIdentifier(s) || 
		IsIdentifier(bit8version) ||
		IsIdentifier(bit16version) ||
		IsIdentifier(bitlong32version) ||
		IsIdentifier(bit32version) ||
		IsIdentifier(bit64version) ||
		IsIdentifier(bit80version);
	}

	static bool IsIdentifierMetadata() {

		return CurrentToken == X86AssemblyToken::X86Text ||
		CurrentToken == X86AssemblyToken::X86Def ||
		CurrentToken == X86AssemblyToken::X86Globl ||
		CurrentToken == X86AssemblyToken::X86Set ||
		CurrentToken == X86AssemblyToken::X86File ||
		CurrentToken == X86AssemblyToken::X86P2Align ||
		CurrentToken == X86AssemblyToken::X86SEHSetFrame ||
		CurrentToken == X86AssemblyToken::X86SEH ||
		CurrentToken == X86AssemblyToken::X86SEHEnd;
	}

	static int GetIdentifier() {

		IdentifierStr = LastChar;

		while (is_still_identifier((LastChar = Advance())))
		{
			IdentifierStr += LastChar;
		}

		if(IsMultipleBitIdentifier("add")) return X86AssemblyToken::X86Add;
		else if(IsMultipleBitIdentifier("sub")) return X86AssemblyToken::X86Sub;
		else if(IsMultipleBitIdentifier("xor")) return X86AssemblyToken::X86Xor;
		else if(IsMultipleBitIdentifier("inc")) return X86AssemblyToken::X86Inc;
		else if(IsMultipleBitIdentifier("lea")) return X86AssemblyToken::X86Lea;
		else if(IsMultipleBitIdentifier("call")) return X86AssemblyToken::X86Call;
		else if(IsMultipleBitIdentifier("pop")) return X86AssemblyToken::X86Pop;
		else if(IsMultipleBitIdentifier("push")) return X86AssemblyToken::X86Push;
		else if(IsMultipleBitIdentifier("ret")) return X86AssemblyToken::X86Return;

		else if(IsMultipleBitIdentifier("mov")) return X86AssemblyToken::X86Mov;
		else if(IsMultipleBitIdentifier("cmp")) return X86AssemblyToken::X86Cmp;
		else if(IsMultipleBitIdentifier("test")) return X86AssemblyToken::X86Test;

		else if(IsIdentifier("jmp")) return X86AssemblyToken::X86Jmp;

		else if(IsIdentifier(".text")) return X86AssemblyToken::X86Text;
		else if(IsIdentifier(".def")) return X86AssemblyToken::X86Def;
		else if(IsIdentifier(".globl")) return X86AssemblyToken::X86Globl;
		else if(IsIdentifier(".set")) return X86AssemblyToken::X86Set;
		else if(IsIdentifier(".file")) return X86AssemblyToken::X86File;
		else if(IsIdentifier(".p2align")) return X86AssemblyToken::X86P2Align;

		else if(IsIdentifier(".seh_setframe")) { 
			return X86AssemblyToken::X86SEHSetFrame; 
		}

		else if(IsIdentifier(".seh_proc") || 
			IsIdentifier(".seh_pushreg") ||
			IsIdentifier(".seh_stackalloc")) { 
			return X86AssemblyToken::X86SEH; 
		}

		else if(IdentifierContains(".seh_endprologue") || IsIdentifier(".seh_endproc")) { 
			return X86AssemblyToken::X86SEHEnd; 
		}

		return X86AssemblyToken::X86Identifier;
	}

	template<typename T2, typename T1>
	static inline T2 LexicalCast(const T1 &in) {
	    T2 out;
	    std::stringstream ss;
	    ss << in;
	    ss >> out;
	    return out;
	}

	static int GetNumber()
	{
		std::string NumStr;

		do
		{
			NumStr += LastChar;
			LastChar = Advance();
		} while (isalnum(LastChar));

		if(NumStr.find("0x") != std::string::npos) {

			int lc = LexicalCast<int>(NumStr.c_str());
			NumStr = std::to_string(lc);
		}

		NumValString = NumStr;
		return X86AssemblyToken::X86Number;
	}

	static int GetString()
	{
		StringString = "";
		LastChar = Advance();

		do
		{
			//if(LastChar == '\\')
			//	StringSlash();

			StringString += LastChar;
			LastChar = Advance();
		} while(LastChar != '\"' && LastChar != X86AssemblyToken::X86EndOfFile && LastChar >= 32);

		if(LastChar == '\"') { LastChar = Advance(); }

		return X86AssemblyToken::X86String;
	}
};

#endif