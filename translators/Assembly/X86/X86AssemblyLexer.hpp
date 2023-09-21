#ifndef ASSEMBLY_LEXER_HPP
#define ASSEMBLY_LEXER_HPP

#include <string>
#include <vector>
#include <sstream>

enum X86AssemblyToken {

	X86EndOfFile = -1,
	X86Identifier = -2,
	X86Number = -3,

	X86AddL = -4,
	X86AddQ = -5,

	X86MovL = -6,

	X86Return = -7,

	X86PushQ = -8,
	X86SubQ = -9,
	X86LeaQ = -10,
	X86CallQ = -11,

	X86PopQ = -12,

	X86Text = -13,
	X86Def = -14,
	X86Globl = -15,
	X86Set = -16,
	X86File = -17,
	X86P2Align = -18,

	X86String = -19,
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
		Column += 1;

		line_as_string += Content[Position];

		if (Content[Position] == '\n')
		{
			Line += 1;
			Column = 1;

			all_lines_vector.push_back(line_as_string);

			line_as_string.clear();
		}

		return Content[Position];
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

	static bool is_still_identifier(char c)
	{
		return isalnum(c) || c == '_' || c == '.';
	}

	static int GetIdentifier() {

		IdentifierStr = LastChar;

		while (is_still_identifier((LastChar = Advance())))
		{
			IdentifierStr += LastChar;
		}

		if(IsIdentifier("addl")) return X86AssemblyToken::X86AddL;

		else if(IsIdentifier("addq")) return X86AssemblyToken::X86AddQ;
		else if(IsIdentifier("subq")) return X86AssemblyToken::X86SubQ;
		else if(IsIdentifier("leaq")) return X86AssemblyToken::X86LeaQ;
		else if(IsIdentifier("callq")) return X86AssemblyToken::X86CallQ;
		else if(IsIdentifier("popq")) return X86AssemblyToken::X86PopQ;
		else if(IsIdentifier("pushq")) return X86AssemblyToken::X86PushQ;
		else if(IsIdentifier("retq")) return X86AssemblyToken::X86Return;

		else if(IsIdentifier("movl")) return X86AssemblyToken::X86MovL;

		else if(IsIdentifier(".text")) return X86AssemblyToken::X86Text;
		else if(IsIdentifier(".def")) return X86AssemblyToken::X86Def;
		else if(IsIdentifier(".globl")) return X86AssemblyToken::X86Globl;
		else if(IsIdentifier(".set")) return X86AssemblyToken::X86Set;
		else if(IsIdentifier(".file")) return X86AssemblyToken::X86File;
		else if(IsIdentifier(".p2align")) return X86AssemblyToken::X86P2Align;

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