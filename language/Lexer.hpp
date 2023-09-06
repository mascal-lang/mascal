#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <iostream>
#include <vector>
//#include "ErrorHandler.hpp"

enum Token
{
	EndOfFile = -1,
	Program = -2,
	Begin = -3,
	End = -4,

	String = -5,
	Number = -6,

	Identifier = -7,

	Com = -8,

	LLReturn = -9,

	Add = -10,
	Sub = -11,

	Compare = -12,

	If = -13,
	Then = -14,
	Else = -15,

	Return = -16,

	Procedure = -17,

	ComStore = -18,

	Mem = -19,
};

enum LexerIsInside {
	AProgram,
	AProcedure
};

struct Lexer
{
	static std::string Content;

	static std::string IdentifierStr;
	static std::string NumValString;
	static std::string StringString;

	static void AddContent(std::string c)
	{
		Content += c;
	}

	static int CurrentToken;
	static int Position;

	static int Line;
	static int Column;

	static std::string line_as_string;

	static std::vector<std::string> all_lines_vector;

	static LexerIsInside isInside;

	static void Start()
	{
		Position = -1;
		Line = 1;
		Column = 1;
		LastChar = ' ';
	}

	static int Advance()
	{
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

	static void GetNextToken()
	{
		CurrentToken = GetToken();
	}

	static int LastChar;

	static int GetToken()
	{
		while (isspace(LastChar)) LastChar = Advance();

		if (isalpha(LastChar) || LastChar == '@') return GetIdentifier();

		if (isdigit(LastChar)) return GetNumber();

		if(LastChar == '\'') return GetChar();

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

		if (LastChar == EOF) return Token::EndOfFile;

		int ThisChar = LastChar;
		LastChar = Advance();

		// This is a fail-safe in case memory corruption appears.
		// Since at this point, we're looking for normal characters,
		// it makes no sense to find characters that are below space in
		// the ASCII table. Meaning that if we find one like that at this point,
		// its undefined behavior.
		if (ThisChar < 32) ThisChar = Token::EndOfFile;

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

	//static void throw_identifier_syntax_warning(std::string message, std::string recommendation)
	//{
	//	ErrorHandler::print(message, Lexer::Line, Lexer::Column, Lexer::line_as_string, 1, recommendation);
	//}

	static int GetChar()
	{
		LastChar = Advance();

		NumValString = "";

		if(LastChar == '\\')
			StringSlash();

		NumValString += std::to_string(LastChar);

		LastChar = Advance();

		if(LastChar == '\'') { LastChar = Advance(); }

		return Token::Number;
	}

	static int GetString()
	{
		StringString = "";
		LastChar = Advance();

		do
		{
			if(LastChar == '\\')
				StringSlash();

			StringString += LastChar;
			LastChar = Advance();
		} while(LastChar != '\"' && LastChar != Token::EndOfFile && LastChar >= 32);

		if(LastChar == '\"') { LastChar = Advance(); }

		return Token::String;
	}

	static void StringSlash()
	{
		LastChar = Advance();

		if(LastChar == 'n') LastChar = '\n';
		else if(LastChar == 'r') LastChar = '\r';
		else if(LastChar == 't') LastChar = '\t';
		else if(LastChar == '0') LastChar = '\0';
		else if(LastChar == '\"') LastChar = '\"';
		else if(LastChar == '\\') LastChar = '\\';
	}

	static int GetIdentifier()
	{
		IdentifierStr = LastChar;

		while (is_still_identifier((LastChar = Advance())))
		{
			IdentifierStr += LastChar;
		}

		if (IsIdentifier("program")) return Token::Program;
		else if(IsIdentifier("begin")) return Token::Begin;
		else if(IsIdentifier("end")) return Token::End;

		else if(IsIdentifier("com")) return Token::Com;

		else if(IsIdentifier("llreturn")) return Token::LLReturn;

		else if(IsIdentifier("add")) return Token::Add;
		else if(IsIdentifier("sub")) return Token::Sub;

		else if(IsIdentifier("COMPARE")) return Token::Compare;

		else if(IsIdentifier("if")) return Token::If;
		else if(IsIdentifier("then")) return Token::Then;
		else if(IsIdentifier("else")) return Token::Else;

		else if(IsIdentifier("return")) return Token::Return;

		else if(IsIdentifier("proc")) return Token::Procedure;

		else if(IsIdentifier("comstore")) return Token::ComStore;

		else if(IsIdentifier("mem")) return Token::Mem;

		return Token::Identifier;
	}

	static int GetNumber()
	{
		std::string NumStr;

		do
		{
			if(LastChar != '_') NumStr += LastChar;
			LastChar = Advance();
		} while (isdigit(LastChar) || LastChar == '.' || LastChar == 'f' || LastChar == '_');

		NumValString = NumStr;
		return Token::Number;
	}
};

#endif