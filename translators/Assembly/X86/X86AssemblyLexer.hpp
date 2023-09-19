#ifndef ASSEMBLY_LEXER_HPP
#define ASSEMBLY_LEXER_HPP

#include <string>
#include <vector>

enum X86AssemblyToken {

	X86EndOfFile = -1,
	X86Identifier = -2,
	X86Number = -3,

	X86AddL = -4,

	X86Return = -5,
};

struct X86AssemblyLexer {

	static std::string Content;

	static std::string IdentifierStr;
	static std::string NumValString;

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

		if(isalpha(LastChar) || LastChar == '%') return GetIdentifier();

		if(isdigit(LastChar) || LastChar == '$') return GetNumber();

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
		return isalnum(c) || c == '_';
	}

	static int GetIdentifier() {

		IdentifierStr = LastChar;

		while (is_still_identifier((LastChar = Advance())))
		{
			IdentifierStr += LastChar;
		}

		if(IsIdentifier("addl")) return X86AssemblyToken::X86AddL;
		else if(IsIdentifier("retq")) return X86AssemblyToken::X86Return;

		return X86AssemblyToken::X86Identifier;
	}

	static int GetNumber()
	{
		std::string NumStr;

		do
		{
			NumStr += LastChar;
			LastChar = Advance();
		} while (isdigit(LastChar));

		NumValString = NumStr;
		return X86AssemblyToken::X86Number;
	}
};

#endif