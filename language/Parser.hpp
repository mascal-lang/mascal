#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <cstdlib>
#include "Lexer.hpp"
#include "AST.hpp"

struct Parser {

	static std::string main_target;
	static bool can_main_target_be_modified;

	static std::unordered_map<std::string, AST::Type*> all_parser_coms;

	static void AddParserCom(std::string name, AST::Type* t) {

		all_parser_coms[name] = t;
	}

	static AST::Type* FindType(std::string name) {

		if(all_parser_coms.find(name) != all_parser_coms.end())
			return all_parser_coms[name];

		ExprError("Variable type of '" + name + "' not found.");
		return nullptr;
	}

	static std::unique_ptr<AST::Type> CopyType(AST::Type* t) {

		if(dynamic_cast<AST::Integer32*>(t)) { return std::make_unique<AST::Integer32>(); }
		else if(dynamic_cast<AST::Integer1*>(t)) { return std::make_unique<AST::Integer1>(); }

		ExprError("Variable type to Copy not found.");
		return nullptr;
	}

	static void StartMainTargetSystem() {

		Parser::main_target.clear();
		Parser::can_main_target_be_modified = true;
	}

	static void SetMainTarget(std::string n) {

		if(can_main_target_be_modified) {
			Parser::main_target = n;
			Parser::can_main_target_be_modified = false;
		}
	}

	static void ResetMainTarget() {

		Parser::main_target.clear();
		Parser::can_main_target_be_modified = true;
	}

	static void ExprError(std::string str) {
		std::cout << "Parser Error: " << str << "\n";
		exit(1);
	}

	static std::unique_ptr<AST::Expression> ParseIdentifier() {

		std::string idName = Lexer::IdentifierStr;

		SetMainTarget(idName);

		Lexer::GetNextToken();

		return std::make_unique<AST::Variable>(idName);
	}

	static std::unique_ptr<AST::Expression> ParseNumber() {

		int64_t n = std::stoi(Lexer::NumValString);

		Lexer::GetNextToken();

		return std::make_unique<AST::IntNumber>(n, CopyType(FindType(Parser::main_target)));
	}

	static std::unique_ptr<AST::Type> IdentStrToType() {

		std::string curr_ident = Lexer::IdentifierStr;

		if(curr_ident == "i32") { return std::make_unique<AST::Integer32>(); }
		else if(curr_ident == "i1" || curr_ident == "bool") { return std::make_unique<AST::Integer1>(); }

		ExprError("Unknown type found.");
		return nullptr;
	}

	static std::unique_ptr<AST::Expression> ParseCom() {

		Lexer::GetNextToken();

		std::string idName = Lexer::IdentifierStr;

		SetMainTarget(idName);

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != ':') { ExprError("Expected ':'."); }

		Lexer::GetNextToken();

		std::unique_ptr<AST::Type> ty = IdentStrToType();

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '=') { ExprError("Expected '='."); }

		Lexer::GetNextToken();

		AddParserCom(idName, ty.get());

		std::unique_ptr<AST::Expression> expr = ParseExpression();

		auto final_com = std::make_unique<AST::Com>(idName, std::move(ty), std::move(expr));

		return final_com;
	}

	static std::unique_ptr<AST::Expression> ParseLLReturn() {

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> expr = ParseExpression();

		return std::make_unique<AST::LLReturn>(std::move(expr));
	}

	static std::unique_ptr<AST::Expression> ParseAdd() {

		Lexer::GetNextToken();

		ResetMainTarget();

		std::unique_ptr<AST::Expression> target = ParseExpression();

		if(Lexer::CurrentToken != ',') { ExprError("Expected ','."); }

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> value = ParseExpression();

		return std::make_unique<AST::Add>(std::move(target), std::move(value));
	}

	static std::unique_ptr<AST::Expression> ParseSub() {

		Lexer::GetNextToken();

		ResetMainTarget();

		std::unique_ptr<AST::Expression> target = ParseExpression();

		if(Lexer::CurrentToken != ',') { ExprError("Expected ','."); }

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> value = ParseExpression();

		return std::make_unique<AST::Sub>(std::move(target), std::move(value));
	}

	static int TextToCompareType(std::string t) {

		/*
		IsLessThan,
		IsMoreThan,
		IsEquals,
		IsNotEquals,
		IsLessThanOrEquals,
		IsMoreThanOrEquals
		*/

		if(t == "IsLessThan") { return AST::CompareType::IsLessThan; }
		else if(t == "IsMoreThan") { return AST::CompareType::IsMoreThan; }
		else if(t == "IsEquals") { return AST::CompareType::IsEquals; }
		else if(t == "IsNotEquals") { return AST::CompareType::IsNotEquals; }
		else if(t == "IsLessThanOrEquals") { return AST::CompareType::IsLessThanOrEquals; }
		else if(t == "IsMoreThanOrEquals") { return AST::CompareType::IsMoreThanOrEquals; }

		ExprError("Uknown compare type '" + t + "'");
		return 0;
	}

	static std::unique_ptr<AST::Expression> ParseCompare() {

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '.') {
			ExprError("Expected '.'");
		}

		Lexer::GetNextToken();

		std::string compareType = Lexer::IdentifierStr;

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '(') {
			ExprError("Expected '(' to add arguments.");
		}

		Lexer::GetNextToken();

		auto CompareOne = ParseExpression();

		if(Lexer::CurrentToken != ',') {
			ExprError("Expected ',' to split arguments.");
		}

		Lexer::GetNextToken();

		auto CompareTwo = ParseExpression();

		if(Lexer::CurrentToken != ')') {
			ExprError("Expected ')' to close arguments.");
		}

		Lexer::GetNextToken();

		int finalCompare = TextToCompareType(compareType);

		return std::make_unique<AST::Compare>(std::move(CompareOne), std::move(CompareTwo), finalCompare);
	}

	static std::unique_ptr<AST::Expression> ParseIf() {

		Lexer::GetNextToken();

		auto condition = ParseExpression();

		if(Lexer::CurrentToken != Token::Then) {
			ExprError("Expected 'then' in if block.");
		}

		Lexer::GetNextToken();

		std::vector<std::unique_ptr<AST::Expression>> if_body;

		while(Lexer::CurrentToken != Token::End) {

			std::unique_ptr<AST::Expression> e = ParseExpression();

			if(Lexer::CurrentToken != ';') { ExprError("Expected ';' to end instruction inside if block."); }

			if_body.push_back(std::move(e));

			ResetMainTarget();

			Lexer::GetNextToken();
		}

		Lexer::GetNextToken();

		return std::make_unique<AST::If>(std::move(condition), std::move(if_body));
	}

	static std::unique_ptr<AST::Expression> ParsePrimary() {

		if(Lexer::CurrentToken == Token::Identifier) 	{ return ParseIdentifier(); }
		else if(Lexer::CurrentToken == Token::Number) 	{ return ParseNumber(); }
		else if(Lexer::CurrentToken == Token::Com) 		{ return ParseCom(); }
		else if(Lexer::CurrentToken == Token::LLReturn) { return ParseLLReturn(); }

		else if(Lexer::CurrentToken == Token::Add) 		{ return ParseAdd(); }
		else if(Lexer::CurrentToken == Token::Sub) 		{ return ParseSub(); }

		else if(Lexer::CurrentToken == Token::Compare) 	{ return ParseCompare(); }

		else if(Lexer::CurrentToken == Token::If) { return ParseIf(); }

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

			if(Lexer::CurrentToken != ';') { ExprError("Expected ';' to end instruction inside program."); }

			all_instructions.push_back(std::move(e));

			ResetMainTarget();

			Lexer::GetNextToken();
		}

		return std::make_unique<AST::Program>(std::move(all_instructions));
	}

	static void HandleProgram() {

		auto program = ParseProgram();

		program->codegen()->print(llvm::outs());
	}

	static void MainLoop() {

		StartMainTargetSystem();

		while (Lexer::CurrentToken != Token::EndOfFile) {

			Lexer::GetNextToken();

			if (Lexer::CurrentToken == Token::EndOfFile) 	break;
			if (Lexer::CurrentToken == Token::Program) 		HandleProgram();
		}
	}
};

#endif