#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <fstream>
#include <cstdlib>
#include "Lexer.hpp"
#include "AST.hpp"

struct Parser {

	static std::string main_target;
	static bool can_main_target_be_modified;

	static std::string current_procedure_name;

	static std::vector<std::unique_ptr<AST::Procedure>> all_procedures;

	static std::unordered_map<std::string, AST::Type*> all_parser_coms;

	static void AddParserCom(std::string name, AST::Type* t) {

		all_parser_coms[name] = t;
	}

	static AST::Type* FindType(std::string name) {

		if(all_parser_coms.find(name) != all_parser_coms.end())
			return all_parser_coms[name];
		else {

			for(auto const& i : all_procedures) {

				if(i->procName == Parser::current_procedure_name) {

					//std::cout << "Checking Procedure " << i->procName << " with Current " << Parser::current_procedure_name << "...\n";

					//if(i->all_arguments.size() == 0) {
					//	std::cout << i->procName << " has no arguments!\n";
					//}

					int Idx = 0;
					for(auto const& args : i->all_arguments) {

						//std::cout << "Checking " << args->name << "...\n";
						if(args->name == name) {

							return i->all_argument_types[Idx].get();
						}

						Idx++;
					}
				}
			}
		}

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

	static std::unique_ptr<AST::Procedure> CloneProcedure(std::string name) {

		for(auto const& i: all_procedures) {

			if(i->procName == name) {
				return i->Clone();
			}
		}

		ExprError("Procedure '" + name + "' not found.");
		return nullptr;
	}

	static void AddCallCountToProcedure(std::string name) {

		for(auto const& i: all_procedures) {

			if(i->procName == name) {
				i->call_count += 1;
				return;
			}
		}

		ExprError("Procedure '" + name + "' not found.");
	}

	static std::unique_ptr<AST::Expression> ParseCall(std::string name) {

		Lexer::GetNextToken();

		std::vector<std::unique_ptr<AST::Expression>> call_arguments;

		AddCallCountToProcedure(name);

		std::unique_ptr<AST::Procedure> proc_copy = CloneProcedure(name);
		CLONE_EXPR_VECTOR(proc_copy->body, body_clone);

		while(Lexer::CurrentToken != ')') {

			auto I = ParseIdentifier();

			call_arguments.push_back(std::move(I));

			if(Lexer::CurrentToken != ',') {
				if(Lexer::CurrentToken != ')') {
					ExprError("Expected ',' to split arguments or ')' to end call.");
				}
				else {
					break;
				}
			}

			Lexer::GetNextToken();
		}

		if(Lexer::CurrentToken == ')') {
			Lexer::GetNextToken();
		}

		for(auto const& i: body_clone) {

			if(i->ContainsName(name + "_return")) {
				i->ReplaceTargetNameTo(name + "_return", name + "_return" + std::to_string(proc_copy->call_count));
			}
		}

		for(auto const& i: body_clone) {

			int Idx = 0;
			for(auto const& p: proc_copy->all_arguments) {

				if(i->ContainsName(p->name)) {

					i->ReplaceTargetNameTo(p->name, call_arguments[Idx]->name);
				}

				Idx++;
			}
		}

		return std::make_unique<AST::ProcedureCall>(std::move(body_clone), std::make_unique<AST::Variable>(name + "_return" + std::to_string(proc_copy->call_count)));
	}

	static std::unique_ptr<AST::Expression> ParseIdentifier() {

		std::string idName = Lexer::IdentifierStr;

		Lexer::GetNextToken();

		if(Lexer::CurrentToken == '(') {
			return ParseCall(idName);
		}

		SetMainTarget(idName);

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

	static std::unique_ptr<AST::Expression> ParseReturn() {

		if(Lexer::isInside == LexerIsInside::AProgram) {
			return ParseLLReturn();
		}

		Lexer::GetNextToken();

		ResetMainTarget();

		SetMainTarget(Parser::current_procedure_name + "_return");

		std::unique_ptr<AST::Expression> expr = ParseExpression();

		return std::make_unique<AST::ComStore>(std::make_unique<AST::Variable>(Parser::current_procedure_name + "_return"), std::move(expr));
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

	static std::unique_ptr<AST::Expression> ParseIf(bool check_comma = true) {

		Lexer::GetNextToken();

		auto condition = ParseExpression();

		if(Lexer::CurrentToken != Token::Then) {
			ExprError("Expected 'then' in if block.");
		}

		Lexer::GetNextToken();

		std::vector<std::unique_ptr<AST::Expression>> if_body;
		std::vector<std::unique_ptr<AST::Expression>> else_body;

		while(Lexer::CurrentToken != Token::End && Lexer::CurrentToken != Token::Else) {

			std::unique_ptr<AST::Expression> e = ParseExpression();

			if(Lexer::CurrentToken != ';') { ExprError("Expected ';' to end instruction inside if block."); }

			if_body.push_back(std::move(e));

			ResetMainTarget();

			Lexer::GetNextToken();
		}

		if(Lexer::CurrentToken == Token::Else) {

			Lexer::GetNextToken();

			if(Lexer::CurrentToken == Token::If) {
				std::unique_ptr<AST::Expression> if_b = ParseIf(false);

				else_body.push_back(std::move(if_b));
			}
			else if(Lexer::CurrentToken == Token::Then) {

				Lexer::GetNextToken();

				while(Lexer::CurrentToken != Token::End) {

					std::unique_ptr<AST::Expression> e = ParseExpression();

					if(Lexer::CurrentToken != ';') { ExprError("Expected ';' to end instruction inside else block."); }
		
					else_body.push_back(std::move(e));
		
					ResetMainTarget();
		
					Lexer::GetNextToken();
				}
			}
			else {
				ExprError("Expected 'if' or 'then' in else block.");
			}
		}

		if(check_comma)
			Lexer::GetNextToken();

		return std::make_unique<AST::If>(std::move(condition), std::move(if_body), std::move(else_body));
	}

	static std::unique_ptr<AST::Expression> ParseComStore() {

		Lexer::GetNextToken();

		ResetMainTarget();

		std::unique_ptr<AST::Expression> target = ParseExpression();

		if(Lexer::CurrentToken != ',') { ExprError("Expected ','."); }

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> value = ParseExpression();

		return std::make_unique<AST::ComStore>(std::move(target), std::move(value));
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

		else if(Lexer::CurrentToken == Token::Return) { return ParseReturn(); }

		else if(Lexer::CurrentToken == Token::ComStore) { return ParseComStore(); }

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

	static std::unique_ptr<AST::Procedure> ParseProcedure() {

		Lexer::isInside = LexerIsInside::AProcedure;

		Lexer::GetNextToken();

		std::string procName = Lexer::IdentifierStr;

		Parser::current_procedure_name = procName;

		Lexer::GetNextToken();

		std::vector<std::string> all_argument_var_types;
		std::vector<std::unique_ptr<AST::Expression>> all_arguments;
		std::vector<std::unique_ptr<AST::Type>> all_argument_types;
		std::vector<std::unique_ptr<AST::Expression>> body;

		if(Lexer::CurrentToken != '(') { ExprError("Expected '(' to add arguments."); }

		Lexer::GetNextToken();

		while(Lexer::CurrentToken != ')') {

			if(Lexer::CurrentToken == Token::Com)
				all_argument_var_types.push_back("com");

			Lexer::GetNextToken();

			auto I = ParseIdentifier();

			if(Lexer::CurrentToken != ':') { ExprError("Expected ':' to specify argument type."); }

			Lexer::GetNextToken();

			auto T = IdentStrToType();

			Lexer::GetNextToken();

			all_arguments.push_back(std::move(I));
			all_argument_types.push_back(std::move(T));

			if(Lexer::CurrentToken != ',') {
				if(Lexer::CurrentToken == ')') {
					break;
				}
				else {
					ExprError("Expected ',' to add another argument or ')' to close argument list.");
				}
			}

			Lexer::GetNextToken();
		}

		if(Lexer::CurrentToken != ')') { ExprError("Expected ')' to close argument list."); }

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != ':') { ExprError("Expected ':' to specify procedure type."); }

		Lexer::GetNextToken();

		auto procType = IdentStrToType();

		Lexer::GetNextToken();

		auto proc = std::make_unique<AST::Procedure>(procName, all_argument_var_types, std::move(all_arguments), std::move(all_argument_types), std::move(procType));

		all_procedures.push_back(std::move(proc));

		auto newProc = all_procedures[all_procedures.size() - 1]->CloneWithoutBody();

		//std::cout << "Clone Success!\n";

		if(Lexer::CurrentToken != Token::Begin) { ExprError("Expected 'begin' in procedure."); }

		Lexer::GetNextToken();

		auto return_value = std::make_unique<AST::Com>(procName + "_return", newProc->proc_type->Clone(), std::make_unique<AST::IntNumber>(0, newProc->proc_type->Clone()));

		AddParserCom(procName + "_return", return_value->ty.get());

		body.push_back(std::move(return_value));

		while (Lexer::CurrentToken != Token::End) { 

			std::unique_ptr<AST::Expression> e = ParseExpression();

			if(Lexer::CurrentToken != ';') { ExprError("Expected ';' to end instruction inside procedure."); }

			body.push_back(std::move(e));

			ResetMainTarget();

			Lexer::GetNextToken();
		}

		newProc->body = std::move(body);

		all_procedures.pop_back();

		return newProc;
	}

	static void HandleProgram() {

		Lexer::isInside = LexerIsInside::AProgram;

		auto program = ParseProgram();

		std::ofstream myfile;
  		myfile.open("llm_main.mascal");
  		myfile << program->ToLLMascal();
  		myfile.close();

		program->codegen();

		CodeGen::TheModule->print(llvm::outs(), nullptr);
	}

	static void HandleProcedure() {

		auto proc = ParseProcedure();

		all_procedures.push_back(std::move(proc));
	}

	static void MainLoop() {

		StartMainTargetSystem();

		while (Lexer::CurrentToken != Token::EndOfFile) {

			Lexer::GetNextToken();

			if (Lexer::CurrentToken == Token::EndOfFile) 	break;
			if (Lexer::CurrentToken == Token::Program) 		HandleProgram();
			if (Lexer::CurrentToken == Token::Procedure) 	HandleProcedure();
		}
	}
};

#endif