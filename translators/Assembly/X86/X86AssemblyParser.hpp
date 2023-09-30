#ifndef ASSEMBLY_PARSER_HPP
#define ASSEMBLY_PARSER_HPP

#include "X86AssemblyAST.hpp"
#include "X86AssemblyLexer.hpp"
#include <cstdlib>
#include <iostream>

struct X86AssemblyParser {

	static std::vector<std::unique_ptr<X86AssemblyAST::Expression>> astRegisters;
	static std::vector<std::unique_ptr<X86AssemblyAST::RAM>> stackMemory;
	static std::vector<std::string> conditionJumpList;

	static bool IsInConditionJumpList(std::string name) {

		for(auto i : conditionJumpList) {
			if(i == name) {
				return true;
			}
		}

		return false;
	}

	static void AddRegister(std::string name) {

		for(auto const& i : astRegisters) {

			if(i->name == name) {
				return;
			}
		}

		auto Reg = std::make_unique<X86AssemblyAST::Register>(name, std::make_unique<X86AssemblyAST::I32>());

		astRegisters.push_back(std::move(Reg));
	}

	static void AddStackMemory(std::string pointerName) {

		for(auto const& i : stackMemory) {

			if(i->pointerName == pointerName) {
				return;
			}
		}

		std::string stackName = std::string("stackMemory") + std::to_string(stackMemory.size());
		auto sMem = std::make_unique<X86AssemblyAST::RAM>(stackName, pointerName, std::make_unique<X86AssemblyAST::I32>());

		stackMemory.push_back(std::move(sMem));
	}

	static std::string GetStackMemoryName(std::string pointerName) {

		for(auto const& i : stackMemory) {

			if(i->pointerName == pointerName) {
				return i->name;
			}
		}

		return "";
	}

	static bool StackMemoryExists(std::string name) {

		for(auto const& i : stackMemory) {

			if(i->name == name) {
				return true;
			}
		}

		return false;
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseNumber() {

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::NumValString[0] == '$') {
			X86AssemblyLexer::NumValString.erase(0, 1);
		}

		if(X86AssemblyLexer::CurrentToken == '(') {

			X86AssemblyLexer::GetNextToken();

			std::string basePointer = X86AssemblyLexer::IdentifierStr;

			std::string pointerName = X86AssemblyLexer::NumValString + "(" + basePointer + ")";

			if(X86AssemblyLexer::IdentifierStr[X86AssemblyLexer::IdentifierStr.size() - 2] == 'b' &&
			 X86AssemblyLexer::IdentifierStr[X86AssemblyLexer::IdentifierStr.size() - 1] == 'p') {

				AddStackMemory(pointerName);
			}

			X86AssemblyLexer::GetNextToken();

			if(X86AssemblyLexer::CurrentToken != ')') {

				std::cout << "Expected ')'.\n";
				exit(1);
				return nullptr;
			}

			X86AssemblyLexer::GetNextToken();

			return std::make_unique<X86AssemblyAST::Variable>(GetStackMemoryName(pointerName));
		}

		return std::make_unique<X86AssemblyAST::IntNumber>(std::stoi(X86AssemblyLexer::NumValString));
	}

	static std::string GetOppositeComparator(std::string comp) {

		if(comp == "jge") { return "jl"; }
		if(comp == "jl") { return "jge"; }

		if(comp == "jle") { return "jg"; }
		if(comp == "jg") { return "jle"; }

		if(comp == "jne") { return "je"; }
		if(comp == "je") { return "jne"; }

		return comp;
	}

	static std::vector<std::unique_ptr<X86AssemblyAST::Expression>> CreateContentVector(std::string name) {

		auto CF = ParseFunction(name, true);

		auto cfcast = dynamic_cast<X86AssemblyAST::Function*>(CF.get());

		if(cfcast == nullptr) {
			std::cout << "Error: 'CF' is not a function.\n";
			exit(1);
		}

		astRegisters = std::move(cfcast->registers);
		stackMemory = std::move(cfcast->stack);

		std::cout << "Content Vector Created! Size: " << cfcast->instructions.size() << ".\n";

		return std::move(cfcast->instructions);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseFunction(std::string name, bool is_vector = false) {

		if(!is_vector) {
			X86AssemblyLexer::GetNextToken();
		}

		std::vector<std::unique_ptr<X86AssemblyAST::Expression>> allInstructions;

		X86AssemblyAST::Attributes attrs;

		bool isInJmpList = IsInConditionJumpList(name);
		bool isWhileLoop = false;

		std::unique_ptr<X86AssemblyAST::If> lastIf = nullptr;

		bool freezeLastIfSearch = false;

		int ifIdCount = 0;

		while(X86AssemblyLexer::CurrentToken != X86AssemblyToken::X86EndOfFile) {

			auto Expr = ParseExpression();

			bool sendInstruction = true;

			if(X86AssemblyAST::IsSEH(Expr.get())) {
				attrs.isStackProtected = true;
				sendInstruction = false;
			}

			if(X86AssemblyAST::UsesCStdLib(Expr.get())) {
				attrs.usesCStdLib = true;
				sendInstruction = false;
			}

			if(X86AssemblyAST::DoesNothing(Expr.get())) {
				sendInstruction = false;
			}

			if(X86AssemblyAST::IsJumpRecursive(Expr.get(), name)) {
				isWhileLoop = true;
				sendInstruction = false;
				break;
			}

			if(dynamic_cast<X86AssemblyAST::If*>(Expr.get()) != nullptr) {

				X86AssemblyAST::If* getIf = dynamic_cast<X86AssemblyAST::If*>(Expr.get());

				if(!freezeLastIfSearch) {

					lastIf = getIf->CloneToIf();

					lastIf->ifId = ifIdCount;
					Expr->ifId = ifIdCount;

					if(lastIf->conditionBlockName == name) {
						isWhileLoop = true;
						break;
					}
				}

				ifIdCount += 1;
			}
			
			if(sendInstruction) {
				allInstructions.push_back(std::move(Expr));
			}

			if(dynamic_cast<X86AssemblyAST::Return*>(Expr.get())) {
				break;
			}
		}

		if(isWhileLoop) {

			if(lastIf == nullptr) {
				std::cout << "Internal Error: 'lastIf' is nullptr.\n";
				exit(1);
				return nullptr;
			}

			if(!freezeLastIfSearch) {
				lastIf->cmpType = GetOppositeComparator(lastIf->cmpType);
			}

			return std::make_unique<X86AssemblyAST::While>(std::move(lastIf), std::move(allInstructions), freezeLastIfSearch);
		}

		if(isInJmpList || is_vector) {

			auto CBlock = std::make_unique<X86AssemblyAST::ConditionBlock>(name, std::move(allInstructions));

			X86AssemblyAST::allConditionBlocks.push_back(std::move(CBlock));

			return std::make_unique<X86AssemblyAST::DoNothing>(name);
		}

		return std::make_unique<X86AssemblyAST::Function>(name, attrs, std::move(allInstructions), std::move(astRegisters), std::move(stackMemory));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseIdentifier() {

		std::string idName = X86AssemblyLexer::IdentifierStr;

		if(idName[0] == '%') {
			idName.erase(0, 1);
			AddRegister(idName);
		}

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken == ':') {
			return ParseFunction(idName);
		}

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

	static std::unique_ptr<X86AssemblyAST::Expression> ParseSub() {

		X86AssemblyLexer::GetNextToken();

		auto One = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in Assembly file.\n";
		}

		X86AssemblyLexer::GetNextToken();

		auto Two = ParseExpression();

		return std::make_unique<X86AssemblyAST::Sub>(std::move(One), std::move(Two));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseXor() {

		X86AssemblyLexer::GetNextToken();

		auto One = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in Assembly file.\n";
		}

		X86AssemblyLexer::GetNextToken();

		auto Two = ParseExpression();

		return std::make_unique<X86AssemblyAST::Xor>(std::move(One), std::move(Two));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseIncrement() {

		X86AssemblyLexer::GetNextToken();

		auto Expr = ParseExpression();

		return std::make_unique<X86AssemblyAST::Add>(std::make_unique<X86AssemblyAST::IntNumber>(1), std::move(Expr));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseMov() {

		X86AssemblyLexer::GetNextToken();

		auto One = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in Assembly file.\n";
		}

		X86AssemblyLexer::GetNextToken();

		auto Two = ParseExpression();

		bool isMem = StackMemoryExists(Two->name);

		return std::make_unique<X86AssemblyAST::Mov>(std::move(One), std::move(Two), isMem);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseLea() {

		X86AssemblyLexer::GetNextToken();

		auto One = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in Assembly file.\n";
		}

		X86AssemblyLexer::GetNextToken();

		auto Two = ParseExpression();

		return std::make_unique<X86AssemblyAST::Lea>(std::move(One), std::move(Two));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParsePush() {

		X86AssemblyLexer::GetNextToken();

		auto Expr = ParseExpression();

		return std::make_unique<X86AssemblyAST::Push>(std::move(Expr));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParsePop() {

		X86AssemblyLexer::GetNextToken();

		auto Expr = ParseExpression();

		return std::make_unique<X86AssemblyAST::Pop>(std::move(Expr));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseCall() {

		X86AssemblyLexer::GetNextToken();

		auto Expr = ParseExpression();

		if(Expr->name == "__main") {
			return std::make_unique<X86AssemblyAST::EnableStdLib>();
		}

		return std::make_unique<X86AssemblyAST::Call>(std::move(Expr));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseText() {

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::Comment>("[Assembly] Begin of .text section.");
	}

	static int GetDefScl() {

		if(X86AssemblyLexer::IdentifierStr != ".scl") {
			std::cout << "Expected '.scl'.\n";
			exit(1);
			return 0;
		}

		X86AssemblyLexer::GetNextToken();

		int sclNum = std::stoi(X86AssemblyLexer::NumValString);

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken != ';') {
			std::cout << "Expected ';' to end '.scl'.\n";
			exit(1);
			return 0;
		}

		X86AssemblyLexer::GetNextToken();

		return sclNum;
	}

	static int GetDefType() {

		if(X86AssemblyLexer::IdentifierStr != ".type") {
			std::cout << "Expected '.type'.\n";
			exit(1);
			return 0;
		}

		X86AssemblyLexer::GetNextToken();

		int typeNum = std::stoi(X86AssemblyLexer::NumValString);

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken != ';') {
			std::cout << "Expected ';' to end '.type'.\n";
			exit(1);
			return 0;
		}

		X86AssemblyLexer::GetNextToken();

		return typeNum;
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseDef() {

		X86AssemblyLexer::GetNextToken();

		std::string defineName = X86AssemblyLexer::IdentifierStr;

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken != ';') {
			std::cout << "Expected ';'.\n";
			exit(1);
			return nullptr;
		}

		X86AssemblyLexer::GetNextToken();

		int scl = GetDefScl();
		int ty = GetDefType();

		if(X86AssemblyLexer::IdentifierStr != ".endef") {
			std::cout << "Expected '.endef' to close Assembly definition.\n";
			exit(1);
			return nullptr;
		}

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::Def>(defineName, scl, ty);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseGlobl() {

		X86AssemblyLexer::GetNextToken();

		std::string defName = X86AssemblyLexer::IdentifierStr;

		X86AssemblyLexer::GetNextToken();

		std::string msg = "[Assembly] Set definition '";
		msg += defName;
		msg += "' as Global for the Linker.";

		return std::make_unique<X86AssemblyAST::Comment>(msg);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseSet() {

		X86AssemblyLexer::GetNextToken();

		std::string defName = X86AssemblyLexer::IdentifierStr;

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in '.set'\n";
			exit(1);
			return nullptr;
		}

		X86AssemblyLexer::GetNextToken();

		auto Expr = ParseExpression();

		return std::make_unique<X86AssemblyAST::Set>(defName, std::move(Expr));
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseFile() {

		X86AssemblyLexer::GetNextToken();

		std::string fileName = X86AssemblyLexer::StringString;

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::File>(fileName);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseP2Align() {

		X86AssemblyLexer::GetNextToken();

		int bytes = std::stoi(X86AssemblyLexer::NumValString);

		X86AssemblyLexer::GetNextToken();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' to split p2align/align arguments.\n";
			exit(1);
			return nullptr;
		}

		X86AssemblyLexer::GetNextToken();

		int limit = std::stoi(X86AssemblyLexer::NumValString);

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::P2Align>(bytes, limit);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseSEH() {

		X86AssemblyLexer::GetNextToken();

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::EnableSEH>();
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseSEHEnd() {

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::EnableSEH>();
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseSEHSetFrame() {

		// Skip '.seh_setframe'
		X86AssemblyLexer::GetNextToken();

		auto Reg = ParseExpression();

		// Check for the comma just in case.
		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in '.seh_setframe'.\n";
			exit(1);
			return nullptr;
		}

		// Skip the comma if it exists.
		X86AssemblyLexer::GetNextToken();

		auto Bytes = ParseExpression();

		return std::make_unique<X86AssemblyAST::EnableSEH>();
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseCompare() {

		X86AssemblyLexer::GetNextToken();

		auto A = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in 'cmp'.\n";
			exit(1);
			return nullptr;
		}

		X86AssemblyLexer::GetNextToken();

		auto B = ParseExpression();

		if(X86AssemblyLexer::IdentifierStr[0] == 'j') {

			std::string jmp = X86AssemblyLexer::IdentifierStr;

			X86AssemblyLexer::GetNextToken();

			std::string jumpName = X86AssemblyLexer::IdentifierStr;
			conditionJumpList.push_back(jumpName);

			X86AssemblyLexer::GetNextToken();

			std::string elseJumpName = "";
			if(X86AssemblyLexer::CurrentToken != X86AssemblyToken::X86Identifier && !X86AssemblyLexer::IsIdentifierMetadata()) {

				auto elseBlock = ParseFunction(std::string("else") + std::to_string(conditionJumpList.size()), true);
				elseJumpName = elseBlock->name;
				conditionJumpList.push_back(elseJumpName);
			}

			return std::make_unique<X86AssemblyAST::If>(std::move(A), std::move(B), jmp, jumpName, elseJumpName);
		}

		return std::make_unique<X86AssemblyAST::Comment>("Not all jump conditions are currently supported yet.");
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseTest() {

		X86AssemblyLexer::GetNextToken();

		auto A = ParseExpression();

		if(X86AssemblyLexer::CurrentToken != ',') {
			std::cout << "Expected ',' in 'test'.\n";
			exit(1);
			return nullptr;
		}

		X86AssemblyLexer::GetNextToken();

		auto B = ParseExpression();

		if(X86AssemblyLexer::IdentifierStr[0] == 'j') {

			std::string jmp = X86AssemblyLexer::IdentifierStr;

			X86AssemblyLexer::GetNextToken();

			std::string jumpName = X86AssemblyLexer::IdentifierStr;
			conditionJumpList.push_back(jumpName);

			X86AssemblyLexer::GetNextToken();

			std::string elseJumpName = "";
			if(X86AssemblyLexer::CurrentToken != X86AssemblyToken::X86Identifier && !X86AssemblyLexer::IsIdentifierMetadata()) {

				auto elseBlock = ParseFunction(std::string("else") + std::to_string(conditionJumpList.size()), true);
				elseJumpName = elseBlock->name;
				conditionJumpList.push_back(elseJumpName);
			}

			return std::make_unique<X86AssemblyAST::If>(std::make_unique<X86AssemblyAST::IntNumber>(0), std::move(B), jmp, jumpName, elseJumpName);
		}

		return std::make_unique<X86AssemblyAST::Comment>("Not all test conditions are currently supported yet.");
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseJump() {

		X86AssemblyLexer::GetNextToken();

		std::string callName = X86AssemblyLexer::IdentifierStr;

		X86AssemblyLexer::GetNextToken();

		return std::make_unique<X86AssemblyAST::Jump>(callName);
	}

	static std::unique_ptr<X86AssemblyAST::Expression> ParseExpression() {

		if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Identifier) { return ParseIdentifier(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Return) { return ParseReturn(); }

		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Add) { return ParseAdd(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Sub) { return ParseSub(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Xor) { return ParseXor(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Inc) { return ParseIncrement(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Lea) { return ParseLea(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Push) { return ParsePush(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Pop) { return ParsePop(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Call) { return ParseCall(); }

		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Mov) { return ParseMov(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Cmp) { return ParseCompare(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Test) { return ParseTest(); }
		
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Jmp) { return ParseJump(); }

		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Text) { return ParseText(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Def) { return ParseDef(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Globl) { return ParseGlobl(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Set) { return ParseSet(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86File) { return ParseFile(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86P2Align) { return ParseP2Align(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86Number) { return ParseNumber(); }

		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86SEH) { return ParseSEH(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86SEHEnd) { return ParseSEHEnd(); }
		else if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86SEHSetFrame) { return ParseSEHSetFrame(); }

		std::cout << "Unknown token or identifier found.\n";
		exit(1);

		return nullptr;
	}

	static std::string HandleExpression() {

		auto Expr = ParseExpression();

		std::string res = "";

		if(Expr != nullptr) {

			res += Expr->codegen();
			res += "\n";
		}

		return res;
	}

	static std::string MainLoop() {

		std::string result;

		X86AssemblyLexer::GetNextToken();

		while(X86AssemblyLexer::CurrentToken != X86AssemblyToken::X86EndOfFile) {

			if(X86AssemblyLexer::CurrentToken == X86AssemblyToken::X86EndOfFile) { break; }

			result += HandleExpression();
		}

		return result;
	}
};

#endif