#ifndef ASSEMBLY_PARSER_HPP
#define ASSEMBLY_PARSER_HPP

#include "X86AssemblyAST.hpp"
#include "X86AssemblyLexer.hpp"
#include <cstdlib>
#include <iostream>

struct X86AssemblyParser {

	static std::vector<std::unique_ptr<X86AssemblyAST::Expression>> astRegisters;

	static void AddRegister(std::string name) {

		for(auto const& i : astRegisters) {

			if(i->name == name) {
				return;
			}
		}

		auto Reg = std::make_unique<X86AssemblyAST::Register>(name, std::make_unique<X86AssemblyAST::I32>());

		astRegisters.push_back(std::move(Reg));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseNumber() {

		X86AssemblyLexer::GetNextToken();

		X86AssemblyLexer::NumValString.erase(0, 1);

		return std::make_unique<X86AssemblyAST::IntNumber>(std::stoi(X86AssemblyLexer::NumValString));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseFunction(std::string name) {

		X86AssemblyLexer::GetNextToken();

		std::vector<std::unique_ptr<X86AssemblyAST::Expression>> allInstructions;

		while(X86AssemblyLexer::CurrentToken != X86AssemblyToken::X86EndOfFile) {

			auto Expr = ParseExpression();

			allInstructions.push_back(std::move(Expr));

			if(dynamic_cast<X86AssemblyAST::Return*>(Expr.get())) {
				break;
			}
		}

		return std::make_unique<X86AssemblyAST::Function>(name, std::move(allInstructions), std::move(astRegisters));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseIdentifier() {

		std::string idName = X86AssemblyLexer::IdentifierStr;

		if(idName[0] == '%') {
			idName.erase(0, 1);
		}

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken == ':') {
			return ParseFunction(idName);
		}

		AddRegister(idName);

		return std::make_unique<X86AssemblyAST::Variable>(idName);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseReturn() {

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::Return>();
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseAdd() {

		X86AssemblyLexer::GetNextToken();

		auto One = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in Assembly file.\n";
		}

		X86AssemblyLexer::GetNextToken();

		auto Two = ParseExpression();

		return std::make_unique<X86AssemblyAST::Add>(std::move(One), std::move(Two));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseMov() {

		X86AssemblyLexer::GetNextToken();

		auto One = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in Assembly file.\n";
		}

		X86AssemblyLexer::GetNextToken();

		auto Two = ParseExpression();

		return std::make_unique<X86AssemblyAST::Mov>(std::move(One), std::move(Two));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseExpression() {

		if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Identifier) { return ParseIdentifier(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Return) { return ParseReturn(); }

		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86AddL) { return ParseAdd(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86MovL) { return ParseMov(); }

		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Number) { return ParseNumber(); }

		std::cout << "Unknown token or identifier found.\n";
		exit(1);

		return nullptr;
	}

	static std::string HandleExpression() {

		auto Expr = ParseExpression();

		if(Expr != nullptr) {
			return Expr->codegen();
		}

		return "";
	}

	static std::string MainLoop() {

		std::string result;

		while(X86AssemblyLexer::CurrentToken != X86AssemblyToken::X86EndOfFile) {

			X86AssemblyLexer::GetNextToken();

			if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86EndOfFile) { break; }

			result += HandleExpression();
		}

		return result;
	}
};

#endif