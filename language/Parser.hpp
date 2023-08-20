#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <cstdlib>
#include "Lexer.hpp"
#include "AST.hpp"

struct Parser {

	static void ExprError(std::string str) {
		std::cout << "Parser Error: " << str << "\n";
		exit(1);
	}

	static std::unique_ptr<AST::Program> ParseProgram() {

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != Token::Begin) { ExprError("'begin' keyword not found."); }

		Lexer::GetNextToken();

		while (Lexer::CurrentToken != Token::End) { 
			Lexer::GetNextToken(); 
		}

		return std::make_unique<AST::Program>();
	}

	static void HandleProgram() {

		auto program = ParseProgram();

		program->codegen()->print(llvm::outs());
	}

	static void MainLoop() {

		while (Lexer::CurrentToken != Token::EndOfFile) {

			Lexer::GetNextToken();

			if (Lexer::CurrentToken == Token::EndOfFile) 	break;
			if (Lexer::CurrentToken == Token::Program) 		HandleProgram();
		}
	}
};

#endif