#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <fstream>
#include <cstdlib>
#include "Lexer.hpp"
#include "AST.hpp"
#include "../utils/DeleteGCCMainCall.hpp"

struct Parser_Mem {

	AST::Type* ty = nullptr;
	bool is_verified = true;

	int loadCount = 0;
	std::string loadVariableName = "";
};

struct Parser {

	static std::string main_target;
	static bool can_main_target_be_modified;

	static std::string current_procedure_name;

	static std::vector<std::unique_ptr<AST::Procedure>> all_procedures;

	static std::unordered_map<std::string, AST::Type*> all_parser_coms;
	static std::unordered_map<std::string, std::unique_ptr<Parser_Mem>> all_parser_mems;

	static AST::Attributes currentAttributes;

	static std::unique_ptr<AST::Expression> lastCompareOne;
	static std::unique_ptr<AST::Expression> lastCompareTwo;
	static int lastCmpType;

	static void AddParserCom(std::string name, AST::Type* t) {

		all_parser_coms[name] = t;
	}

	static void AddParserMem(std::string name, AST::Type* t) {

		auto pMem = std::make_unique<Parser_Mem>();

		pMem->ty = t;
		pMem->is_verified = true;
		
		pMem->loadCount = 0;
		pMem->loadVariableName = "";

		all_parser_mems[name] = std::move(pMem);
	}

	static AST::Type* FindType(std::string name) {

		if(all_parser_coms.find(name) != all_parser_coms.end()) {
			return all_parser_coms[name];
		}
		else if(all_parser_mems.find(name) != all_parser_mems.end()) {
			return all_parser_mems[name]->ty;
		}
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

		if(dynamic_cast<AST::Integer128*>(t)) { return std::make_unique<AST::Integer128>(); }
		else if(dynamic_cast<AST::Integer64*>(t)) { return std::make_unique<AST::Integer64>(); }
		else if(dynamic_cast<AST::Integer32*>(t)) { return std::make_unique<AST::Integer32>(); }
		else if(dynamic_cast<AST::Integer16*>(t)) { return std::make_unique<AST::Integer16>(); }
		else if(dynamic_cast<AST::Integer8*>(t)) { return std::make_unique<AST::Integer8>(); }
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

		bool isVoid = dynamic_cast<AST::Void*>(proc_copy->proc_type.get()) != nullptr;

		if(!isVoid) {

			for(auto const& i: body_clone) {

				if(i->ContainsName(name + "_return")) {
					i->ReplaceTargetNameTo(name + "_return", name + "_return" + std::to_string(proc_copy->call_count));
				}
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

		std::unique_ptr<AST::Expression> returnObj;

		if(!isVoid) {
			returnObj = std::make_unique<AST::Variable>(name + "_return" + std::to_string(proc_copy->call_count));
		}
		else {
			returnObj = std::make_unique<AST::RetVoid>();
		}

		return std::make_unique<AST::ProcedureCall>(std::move(body_clone), std::move(returnObj));
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

		if(Parser::main_target == "") {
			return std::make_unique<AST::IntNumber>(n, std::make_unique<AST::Integer32>());
		}

		return std::make_unique<AST::IntNumber>(n, CopyType(FindType(Parser::main_target)));
	}

	static std::unique_ptr<AST::Type> IdentStrToType() {

		std::string curr_ident = Lexer::IdentifierStr;

		if(curr_ident == "i128") { return std::make_unique<AST::Integer128>(); }
		else if(curr_ident == "i64") { return std::make_unique<AST::Integer64>(); }
		else if(curr_ident == "i32") { return std::make_unique<AST::Integer32>(); }
		else if(curr_ident == "i16") { return std::make_unique<AST::Integer16>(); }
		else if(curr_ident == "i8") { return std::make_unique<AST::Integer8>(); }
		else if(curr_ident == "i1" || curr_ident == "bool") { return std::make_unique<AST::Integer1>(); }

		else if(curr_ident == "void") { return std::make_unique<AST::Void>(); }

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

	static std::unique_ptr<AST::Expression> ParseMem() {

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

		AddParserMem(idName, ty.get());

		std::unique_ptr<AST::Expression> expr = ParseExpression();

		auto final_mem = std::make_unique<AST::Mem>(idName, std::move(ty), std::move(expr));

		return final_mem;
	}

	static std::unique_ptr<AST::Expression> ParseLLReturn() {

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> expr = ParseExpression();

		return std::make_unique<AST::LLReturn>(MemTreatment(std::move(expr)));
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

		return std::make_unique<AST::Add>(UnverifyMem(std::move(target)), MemTreatment(std::move(value)));
	}

	static std::unique_ptr<AST::Expression> ParseSub() {

		Lexer::GetNextToken();

		ResetMainTarget();

		std::unique_ptr<AST::Expression> target = ParseExpression();

		if(Lexer::CurrentToken != ',') { ExprError("Expected ','."); }

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> value = ParseExpression();

		return std::make_unique<AST::Sub>(UnverifyMem(std::move(target)), MemTreatment(std::move(value)));
	}

	static std::unique_ptr<AST::Expression> ParseXor() {

		Lexer::GetNextToken();

		ResetMainTarget();

		std::unique_ptr<AST::Expression> target = ParseExpression();

		if(Lexer::CurrentToken != ',') { ExprError("Expected ','."); }

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> value = ParseExpression();

		return std::make_unique<AST::Xor>(UnverifyMem(std::move(target)), MemTreatment(std::move(value)));
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

		lastCompareOne = CompareOne->Clone();
		lastCompareTwo = CompareTwo->Clone();
		lastCmpType = finalCompare;

		return std::make_unique<AST::Compare>(MemTreatment(std::move(CompareOne)), MemTreatment(std::move(CompareTwo)), finalCompare);
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

		MemVerifyAll();

		while(Lexer::CurrentToken != Token::End && Lexer::CurrentToken != Token::Else) {

			ResetMainTarget();

			std::unique_ptr<AST::Expression> e = ParseExpression();

			if(Lexer::CurrentToken != ';') { ExprError("Expected ';' to end instruction inside if block."); }

			if_body.push_back(std::move(e));

			Lexer::GetNextToken();
		}

		MemVerifyAll();

		if(Lexer::CurrentToken == Token::Else) {

			Lexer::GetNextToken();

			if(Lexer::CurrentToken == Token::If) {
				std::unique_ptr<AST::Expression> if_b = ParseIf(false);

				else_body.push_back(std::move(if_b));
			}
			else if(Lexer::CurrentToken == Token::Then) {

				Lexer::GetNextToken();

				while(Lexer::CurrentToken != Token::End) {

					ResetMainTarget();

					std::unique_ptr<AST::Expression> e = ParseExpression();

					if(Lexer::CurrentToken != ';') { ExprError("Expected ';' to end instruction inside else block."); }
		
					else_body.push_back(std::move(e));
		
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

		value = MemTreatment(std::move(value));

		std::cout << "ComStore >> Target Name: '" << target->name << "'. Value Name: '" << value->name << "'.\n";

		return std::make_unique<AST::ComStore>(std::move(target), std::move(value));
	}

	static std::unique_ptr<AST::Expression> ParseMemStore() {

		Lexer::GetNextToken();

		ResetMainTarget();

		std::unique_ptr<AST::Expression> target = ParseExpression();

		if(Lexer::CurrentToken != ',') { ExprError("Expected ','."); }

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> value = ParseExpression();

		return std::make_unique<AST::MemStore>(Mem_Verify(std::move(target)), MemTreatment(std::move(value)));
	}

	static std::unique_ptr<AST::Expression> ParseLoadMem() {

		Lexer::GetNextToken();

		std::unique_ptr<AST::Expression> expr = ParseExpression();

		return std::make_unique<AST::LoadMem>(std::move(expr));
	}

	static std::unique_ptr<AST::Expression> ParseIntCast() {

		Lexer::GetNextToken();

		ResetMainTarget();

		auto Expr = ParseExpression();

		if(Lexer::CurrentToken != Token::To) {
			ExprError("Expected 'to'.");
		}

		Lexer::GetNextToken();

		auto ty = IdentStrToType();

		Lexer::GetNextToken();

		return std::make_unique<AST::IntCast>(MemTreatment(std::move(Expr)), std::move(ty));
	}

	static std::unique_ptr<AST::Expression> ParseWhile() {

		Lexer::GetNextToken();

		auto Cond = ParseExpression();

		auto cOneOrigin = std::move(lastCompareOne);
		auto cTwoOrigin = std::move(lastCompareTwo);

		int cmpTypeOrigin = lastCmpType;
		lastCmpType = 0;

		if(Lexer::CurrentToken != Token::Do) {
			ExprError("Expected 'do' keyword.");
		}

		Lexer::GetNextToken();

		std::vector<std::unique_ptr<AST::Expression>> loop_body;

		std::vector<std::string> verifyMemAtEnd;

		MemVerifyAll();

		while(Lexer::CurrentToken != Token::End) {

			std::unique_ptr<AST::Expression> e = ParseExpression();

			if(dynamic_cast<AST::MemStore*>(e.get()) != nullptr) {

				AST::MemStore* ms = dynamic_cast<AST::MemStore*>(e.get());

				verifyMemAtEnd.push_back(ms->target->name);
			}

			if(Lexer::CurrentToken != ';') { ExprError("Expected ';' to end instruction inside while loop."); }

			loop_body.push_back(std::move(e));

			ResetMainTarget();

			Lexer::GetNextToken();
		}

		Lexer::GetNextToken();

		auto RepeatCond = std::make_unique<AST::Compare>(MemTreatment(std::move(cOneOrigin)), MemTreatment(std::move(cTwoOrigin)), cmpTypeOrigin);

		UNORDERED_MAP_FOREACH(std::string, std::unique_ptr<Parser_Mem>, all_parser_mems, it) {

			it->second->loadVariableName.clear();

			it->second->is_verified = true;
		}

		return std::make_unique<AST::While>(std::move(Cond), std::move(RepeatCond), std::move(loop_body));
	}

	static std::unique_ptr<AST::Expression> ParsePrimary() {

		if(Lexer::CurrentToken == Token::Identifier) 	{ return ParseIdentifier(); }
		else if(Lexer::CurrentToken == Token::Number) 	{ return ParseNumber(); }
		else if(Lexer::CurrentToken == Token::Com) 		{ return ParseCom(); }
		else if(Lexer::CurrentToken == Token::LLReturn) { return ParseLLReturn(); }

		else if(Lexer::CurrentToken == Token::Add) 		{ return ParseAdd(); }
		else if(Lexer::CurrentToken == Token::Sub) 		{ return ParseSub(); }
		else if(Lexer::CurrentToken == Token::Xor) 		{ return ParseXor(); }

		else if(Lexer::CurrentToken == Token::Compare) 	{ return ParseCompare(); }

		else if(Lexer::CurrentToken == Token::If) { return ParseIf(); }

		else if(Lexer::CurrentToken == Token::Return) { return ParseReturn(); }

		else if(Lexer::CurrentToken == Token::ComStore) { return ParseComStore(); }

		else if(Lexer::CurrentToken == Token::Mem) { return ParseMem(); }
		else if(Lexer::CurrentToken == Token::LoadMem) { return ParseLoadMem(); }
		else if(Lexer::CurrentToken == Token::MemStore) { return ParseMemStore(); }

		else if(Lexer::CurrentToken == Token::IntCast) { return ParseIntCast(); }

		else if(Lexer::CurrentToken == Token::While) { return ParseWhile(); }

		ExprError("Unknown expression found.");
		return nullptr;
	}

	static std::unique_ptr<AST::Expression> Mem_CreateAutoLoad(std::unique_ptr<AST::Expression> V, std::vector<std::unique_ptr<AST::Expression>> ext_init = {} ) {

		Parser::all_parser_mems[V->name]->loadCount += 1;
		Parser::all_parser_mems[V->name]->loadVariableName = V->name + "_load" + std::to_string(Parser::all_parser_mems[V->name]->loadCount);

		auto newCom = std::make_unique<AST::Com>(
			Parser::all_parser_mems[V->name]->loadVariableName, 
			CopyType(Parser::all_parser_mems[V->name]->ty),
			std::make_unique<AST::LoadMem>(std::move(V))
		);

		std::string getComName = newCom->name;

		std::vector<std::unique_ptr<AST::Expression>> addVec;

		addVec = std::move(ext_init);

		addVec.push_back(std::move(newCom));

		//std::cout << getComName << "'s new auto load created!\n";

		return std::make_unique<AST::Variable>(getComName, std::move(addVec));
	}

	static std::unique_ptr<AST::Expression> Mem_CreateAutoStoreAndVerify(std::unique_ptr<AST::Expression> V) {

		std::string getVName = V->name;
		std::string getLoadName = Parser::all_parser_mems[V->name]->loadVariableName;

		Parser::all_parser_mems[V->name]->loadVariableName.clear();

		Parser::all_parser_mems[V->name]->is_verified = true;

		std::vector<std::unique_ptr<AST::Expression>> initStore;

		initStore.push_back(std::make_unique<AST::MemStore>(std::move(V), std::make_unique<AST::Variable>(getLoadName)));

		return Mem_CreateAutoLoad(std::make_unique<AST::Variable>(getVName), std::move(initStore));
	}

	static void MemVerifyAll() {

		UNORDERED_MAP_FOREACH(std::string, std::unique_ptr<Parser_Mem>, Parser::all_parser_mems, it) {

			it->second->loadVariableName.clear();

			it->second->is_verified = true;
		}
	}

	static std::unique_ptr<AST::Expression> Mem_Verify(std::unique_ptr<AST::Expression> V) {

		if(Parser::all_parser_mems.find(V->name) != Parser::all_parser_mems.end()) {

			Parser::all_parser_mems[V->name]->loadVariableName.clear();

			Parser::all_parser_mems[V->name]->is_verified = true;
		}

		return std::move(V);
	}

	static std::unique_ptr<AST::Expression> MemTreatment(std::unique_ptr<AST::Expression> V, bool is_left_ident = false) {

		if(Parser::all_parser_mems.find(V->name) == Parser::all_parser_mems.end()) {
			return V;
		}

		if(Parser::all_parser_mems[V->name]->loadVariableName == "" && Parser::all_parser_mems[V->name]->is_verified) {
			return Mem_CreateAutoLoad(std::move(V));
		}
		else if(!is_left_ident) {

			if(Parser::all_parser_mems[V->name]->loadVariableName != "" && !Parser::all_parser_mems[V->name]->is_verified) {
				return Mem_CreateAutoStoreAndVerify(std::move(V));
			}
		}

		return std::make_unique<AST::Variable>(Parser::all_parser_mems[V->name]->loadVariableName);
	}

	static std::unique_ptr<AST::Expression> UnverifyMem(std::unique_ptr<AST::Expression> V) {

		if(Parser::all_parser_mems.find(V->name) != Parser::all_parser_mems.end()) {
	
			std::string getMemName = V->name;
	
			auto result = MemTreatment(std::move(V), true);

			Parser::all_parser_mems[getMemName]->is_verified = false;
	
			return result;
		}
	
		return V;
	}

	static std::unique_ptr<AST::Expression> ParseAddOperator(std::unique_ptr<AST::Expression> L) {

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '=') {
			ExprError("Expected '='.");
		}

		Lexer::GetNextToken();

		auto R = ParseExpression();

		return std::make_unique<AST::Add>(UnverifyMem(std::move(L)), MemTreatment(std::move(R)));
	}

	static std::unique_ptr<AST::Expression> ParseSubOperator(std::unique_ptr<AST::Expression> L) {

		Lexer::GetNextToken();

		if(Lexer::CurrentToken != '=') {
			ExprError("Expected '='.");
		}

		Lexer::GetNextToken();

		auto R = ParseExpression();

		return std::make_unique<AST::Sub>(UnverifyMem(std::move(L)), MemTreatment(std::move(R)));
	}

	static std::unique_ptr<AST::Expression> ParseEqualsOperator(std::unique_ptr<AST::Expression> L) {

		Lexer::GetNextToken();

		if(all_parser_coms.find(L->name) != all_parser_coms.end()) {

			auto R = ParseExpression();

			return std::make_unique<AST::ComStore>(std::move(L), MemTreatment(std::move(R)));
		}

		if(all_parser_mems.find(L->name) != all_parser_mems.end()) {

			auto R = ParseExpression();

			return std::make_unique<AST::MemStore>(Mem_Verify(std::move(L)), MemTreatment(std::move(R)));
		}

		ExprError("Unknown var type found.");
		return nullptr;
	}

	static std::unique_ptr<AST::Expression> ParseBinaryOperator(std::unique_ptr<AST::Expression> L) {

		if(Lexer::CurrentToken == '=') {
			return ParseEqualsOperator(std::move(L));
		}
		else if(Lexer::CurrentToken == '+') {
			return ParseAddOperator(std::move(L));
		}
		else if(Lexer::CurrentToken == '-') {
			return ParseSubOperator(std::move(L));
		}

		return L;
	}

	static std::unique_ptr<AST::Expression> ParseExpression() {

		auto P = ParsePrimary();

		return ParseBinaryOperator(std::move(P));
	}

	static AST::Attributes ParseAttributes() {

		Lexer::GetNextToken();

		AST::Attributes attrs;

		while(Lexer::CurrentToken != ']') {

			if(Lexer::IsIdentifier("StackProtected")) {
				attrs.isStackProtected = true;
			}

			if(Lexer::IsIdentifier("CStdLib")) {
				attrs.usesCStdLib = true;
			}

			Lexer::GetNextToken();
		}

		if(Lexer::CurrentToken == ']') {
			Lexer::GetNextToken();
		}

		return attrs;
	}

	static std::unique_ptr<AST::Program> ParseProgram() {

		Lexer::GetNextToken();

		if(Lexer::CurrentToken == '[') {
			Parser::currentAttributes = ParseAttributes();
		}

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

		return std::make_unique<AST::Program>(std::move(all_instructions), Parser::currentAttributes);
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

		std::unique_ptr<AST::Type> procType;

		if(Lexer::CurrentToken == ':') { 

			Lexer::GetNextToken();

			procType = IdentStrToType();

			Lexer::GetNextToken();
		}
		else if(Lexer::CurrentToken != Token::Begin) {
			ExprError("Expected ':' to specify procedure type or 'begin' in procedure.");
		}

		bool isVoid = false;

		if(procType == nullptr) {
			isVoid = true;
			procType = std::make_unique<AST::Void>();
		}

		auto proc = std::make_unique<AST::Procedure>(procName, all_argument_var_types, std::move(all_arguments), std::move(all_argument_types), std::move(procType));

		all_procedures.push_back(std::move(proc));

		auto newProc = all_procedures[all_procedures.size() - 1]->CloneWithoutBody();

		//std::cout << "Clone Success!\n";

		if(Lexer::CurrentToken != Token::Begin) { ExprError("Expected 'begin' in procedure."); }

		Lexer::GetNextToken();

		if(!isVoid) {

			auto return_value = std::make_unique<AST::Com>(procName + "_return", newProc->proc_type->Clone(), std::make_unique<AST::IntNumber>(0, newProc->proc_type->Clone()));

			AddParserCom(procName + "_return", return_value->ty.get());

			body.push_back(std::move(return_value));
		}

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

	static std::unique_ptr<AST::Program> HandleProgram() {

		Lexer::isInside = LexerIsInside::AProgram;

		auto program = ParseProgram();

		std::ofstream myfile;
  		myfile.open("llm_main.mascal");
  		myfile << program->ToLLMascal();
  		myfile.close();

  		return program;
	}

	static void HandleProcedure() {

		auto proc = ParseProcedure();

		all_procedures.push_back(std::move(proc));
	}

	static void MainLoop(bool build = false) {

		StartMainTargetSystem();

		std::unique_ptr<AST::Program> MainProgram = nullptr;

		while (Lexer::CurrentToken != Token::EndOfFile) {

			Lexer::GetNextToken();

			if (Lexer::CurrentToken == Token::EndOfFile) 	break;
			if (Lexer::CurrentToken == Token::Program) 		MainProgram = std::move(HandleProgram());
			if (Lexer::CurrentToken == Token::Procedure) 	HandleProcedure();
		}

		//std::cout << "CodeGen...\n";

		MainProgram->codegen();

		if(build) {

			std::error_code EC;
			llvm::raw_fd_ostream dest("output.ll", EC, llvm::sys::fs::OF_None);
			CodeGen::TheModule->print(dest, nullptr);
	
			system("llc output.ll");

			std::cout << "Post-Procesing Assembly...\n";

			std::string compilerArgs = "";
	
			if(!MainProgram->attrs.usesCStdLib) {
	
				std::string asmResult = Utils_DeleteGCCMainCall("output.s");
	
				std::ofstream asmOutput("output.s");
  				asmOutput << asmResult;
  				asmOutput.close();

  				compilerArgs += "-nostdlib";
			}

			std::cout << "Building...\n";

			std::string clangCmd = "clang output.s ";
			clangCmd += compilerArgs;
			clangCmd += " -static -o result";

			system(clangCmd.c_str());

			std::cout << "Done!\n";

			return;
		}

		CodeGen::TheModule->print(llvm::outs(), nullptr);
	}
};

#endif