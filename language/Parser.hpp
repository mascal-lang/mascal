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

	static std::unique_ptr<AST::Expression> ParseIdentifier() {

		std::string idName = Lexer::IdentifierStr;

		Lexer::GetNextToken();

		return std::make_unique<AST::Variable>(idName);
	}

	static std::unique_ptr<AST::Expression> ParseNumber() {

		int64_t n = std::stoi(Lexer::NumValString);

		Lexer::GetNextToken();

		return std::make_unique<AST::IntNumber>(n, std::make_unique<AST::Integer32>());
	}

	static std::unique_ptr<AST::Type> IdentStrToType() {

		std::string curr_ident = Lexer::IdentifierStr;

		if(curr_ident == "i32") { return std::make_unique<AST::Integer32>(); }

		ExprError("Unknown type found.");
		return nullptr;
	}

	static std::unique_ptr<AST::Expression> ParseCom() {

		Lexer::GetNextToken();

		std::string idName = Lexer::IdentifierStr;

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != ':') { ExprError("Expected ':'."); }

		Lexer::GetNextToken();

		std::unique_ptr<AST::Type> ty = IdentStrToType();

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '=') { ExprError("Expected '='."); }

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> expr = ParseExpression();

		return std::make_unique<AST::Com>(idName, std::move(ty), std::move(expr));
	}

	static std::unique_ptr<AST::Expression> ParseLLReturn() {

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> expr = ParseExpression();

		return std::make_unique<AST::LLReturn>(std::move(expr));
	}

	static std::unique_ptr<AST::Expression> ParseAdd() {

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> target = ParseExpression();

		if(Lexer::CurrentToken != ',') { ExprError("Expected ','."); }

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> value = ParseExpression();

		return std::make_unique<AST::Add>(std::move(target), std::move(value));
	}

	static std::unique_ptr<AST::Expression> ParsePrimary() {

		if(Lexer::CurrentToken == Token::Identifier) 	{ return ParseIdentifier(); }
		else if(Lexer::CurrentToken == Token::Number) 	{ return ParseNumber(); }
		else if(Lexer::CurrentToken == Token::Com) 		{ return ParseCom(); }
		else if(Lexer::CurrentToken == Token::LLReturn) { return ParseLLReturn(); }

		else if(Lexer::CurrentToken == Token::Add) 		{ return ParseAdd(); }
		//else if(Lexer::CurrentToken == Token::Sub) 		{ return ParseSub(); }

		ExprError("Unknown expression found.");
		return nullptr;
	}

	static std::unique_ptr<AST::Expression> ParseExpression() {

		return ParsePrimary();
	}

	static std::unique_ptr<AST::Program> ParseProgram() {

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != Token::Begin) { ExprError("'begin' keyword not found."); }

		Lexer::GetNextToken();

		std::vector<std::unique_ptr<AST::Expression>> all_instructions;

		while (Lexer::CurrentToken != Token::End) { 

			std::unique_ptr<AST::Expression> e = ParseExpression();

			if(Lexer::CurrentToken != ';') { ExprError("Expected ';' to end instruction."); }

			all_instructions.push_back(std::move(e));

			Lexer::GetNextToken();
		}

		return std::make_unique<AST::Program>(std::move(all_instructions));
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