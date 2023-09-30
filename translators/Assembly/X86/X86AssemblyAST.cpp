#include "X86AssemblyAST.hpp"
#include <iostream>

std::vector<std::unique_ptr<X86AssemblyAST::ConditionBlock>> X86AssemblyAST::allConditionBlocks;

int X86AssemblyAST::slash_t_count = 0;

std::string X86AssemblyAST::Variable::codegen() {

	std::string res;

	res += name;

	return res;
}

std::string X86AssemblyAST::IntNumber::codegen() {

	return std::to_string(numb);
}

std::string X86AssemblyAST::I32::codegen() { return "i32"; }

std::string X86AssemblyAST::Return::codegen() {

	return "return eax";
}

std::string X86AssemblyAST::Register::codegen() {

	std::string res = "com ";

	res += name;
	res += ": ";
	res += ty->codegen();
	res += " = 0";

	return res;
}

std::string X86AssemblyAST::RAM::codegen() {

	std::string res = "mem ";

	res += name;
	res += ": ";
	res += ty->codegen();
	res += " = 0";

	return res;
}

std::string X86AssemblyAST::Function::codegen() {

	std::string res;

	if(name == "main") {
		res += "program ";
		res += attrs.codegen();
		res += " begin\n";
	}
	else {
		res += "proc ";
		res += attrs.codegen();
		res += " ";
		res += name;
		res += "() begin\n";
	}

	slash_t_count += 1;

	for(auto const& i : registers) {

		std::string regCG = i->codegen();

		if(regCG != "") {

			res += GetSlashT();
			res += regCG;
			res += ";\n";
		}
	}

	for(auto const& i : stack) {

		std::string staCG = i->codegen();

		if(staCG != "") {
			
			res += GetSlashT();
			res += staCG;
			res += ";\n";
		}
	}

	for(auto const& i : instructions) {

		std::string insCG = i->codegen();

		if(insCG != "") {
			
			res += GetSlashT();
			res += insCG;
			res += ";\n";
		}
	}

	if(name == "main" && dynamic_cast<X86AssemblyAST::Return*>(instructions[instructions.size() - 1].get()) == nullptr) {
		res += GetSlashT() + "return eax;\n";
	}

	slash_t_count -= 1;

	res += "end\n";

	return res;
}

std::string X86AssemblyAST::Add::codegen() {

	std::string res;

	if(X86AssemblyAST::IsStackPointer(target->name)) {
		return std::string("# [Assembly]: Pop '") + value->codegen() + std::string("' bytes from Stack Pointer.");
	}

	res += "add ";
	res += target->codegen();
	res += ", ";
	res += value->codegen();

	return res;
}

std::string X86AssemblyAST::Sub::codegen() {

	std::string res;

	if(X86AssemblyAST::IsStackPointer(target->name)) {
		return std::string("# [Assembly]: Reserve '") + value->codegen() + std::string("' bytes for local variables.");
	}

	res += "sub ";
	res += target->codegen();
	res += ", ";
	res += value->codegen();

	return res;
}

std::string X86AssemblyAST::Xor::codegen() {

	std::string res;

	//if(X86AssemblyAST::IsStackPointer(target->name)) {
	//	return std::string("# [Assembly]: Reserve '") + value->codegen() + std::string("' bytes for local variables.");
	//}

	res += "xor ";
	res += target->codegen();
	res += ", ";
	res += value->codegen();

	return res;
}

std::string X86AssemblyAST::Mov::codegen() {

	std::string res;

	if(!isMem) {
		res += "comstore ";
	}
	else {
		res += "memstore ";
	}

	res += target->codegen();
	res += ", ";
	res += value->codegen();

	return res;
}

std::string X86AssemblyAST::Lea::codegen() {

	return "# 'lea' instructions not supported yet.";
}

std::string X86AssemblyAST::Push::codegen() {

	if(X86AssemblyAST::IsBasePointer(target->name)) {
		return "# [Assembly]: Initialize the Base Pointer.";
	}

	return "# 'push' instructions not supported yet.";
}

std::string X86AssemblyAST::Pop::codegen() {

	if(X86AssemblyAST::IsBasePointer(target->name)) {
		return "# [Assembly]: Pop the Base Pointer to close the program.";
	}

	return "# 'pop' instructions not supported yet.";
}

std::string X86AssemblyAST::Call::codegen() {

	if(target->name == "__main") {
		return "# [GCC/Clang/LLVM] Call to __main.";
	}

	return "# 'call' instructions not supported yet.";
}

std::string X86AssemblyAST::Comment::codegen() {

	std::string comment = "# ";
	comment += message;
	return comment;
}

std::string X86AssemblyAST::Def::codegen() {

	std::string res = "# [Assembly] Added new definition '";

	res += name;
	res += "':\n";

	res += "#	.scl = ";
	res += std::to_string(scl);
	res += "\n";

	res += "#	.type = ";
	res += std::to_string(ty);

	return res;
}

std::string X86AssemblyAST::Set::codegen() {

	std::string res = "# [Assembly] Set '";
	
	res += name;
	res += "' to '";
	res += target->codegen();
	res += "'.";

	return res;
}

std::string X86AssemblyAST::File::codegen() {

	std::string res = "# [Assembly] Filename: \"";
	res += name;
	res += "\".";

	return res;
}

std::string X86AssemblyAST::P2Align::codegen() {

	std::string res = "# [Assembly] P2Align Set (";
	res += std::to_string(bytes);
	res += ", ";
	res += std::to_string(limit);
	res += ").";

	return res;
}

std::string JumpInstructionToCompare(std::string comp, bool& is_second_param_zero) {

	if(comp == "jge") { is_second_param_zero = false; return "IsMoreThanOrEquals"; }
	if(comp == "jle") { is_second_param_zero = false; return "IsLessThanOrEquals"; }

	if(comp == "jg") { is_second_param_zero = false; return "IsMoreThan"; }
	if(comp == "jl") { is_second_param_zero = false; return "IsLessThan"; }

	if(comp == "je") { is_second_param_zero = false; return "IsEquals"; }
	if(comp == "jne") { is_second_param_zero = false; return "IsNotEquals"; }

	if(comp == "jz") { is_second_param_zero = true; return "IsEquals"; }
	if(comp == "jnz") { is_second_param_zero = true; return "IsNotEquals"; }

	if(comp == "js") { is_second_param_zero = true; return "IsMoreThan"; }
	if(comp == "jns") { is_second_param_zero = true; return "IsLessThan"; }

	if(comp == "ja") { is_second_param_zero = true; return "IsMoreThan"; }
	if(comp == "jb") { is_second_param_zero = true; return "IsLessThan"; }

	return "";
}

std::string X86AssemblyAST::If::codegen() {

	std::string res;

	res += "if COMPARE.";

	bool is_second_param_zero = false;

	res += JumpInstructionToCompare(cmpType, is_second_param_zero);
	res += "(";

	res += B->codegen();
	res += ", ";

	if(!is_second_param_zero) {
		res += A->codegen();
	}
	else {
		res += "0";
	}

	res += ") then\n";

	slash_t_count += 1;

	res += X86AssemblyAST::GetConditionBlock(conditionBlockName)->codegen();

	slash_t_count -= 1;

	if(elseConditionBlockName.size() != 0) {

		std::cout << "Generating Else...\n";

		res += GetSlashT() + "else then\n";

		slash_t_count += 1;

		res += X86AssemblyAST::GetConditionBlock(elseConditionBlockName)->codegen();

		slash_t_count -= 1;
	}

	res += GetSlashT() + "end";

	return res;
}

std::string X86AssemblyAST::While::codegen() {

	std::string res;

	res += "while COMPARE.";

	bool is_second_param_zero = false;

	res += JumpInstructionToCompare(cmp->cmpType, is_second_param_zero);
	res += "(";

	res += cmp->B->codegen();
	res += ", ";
	res += cmp->A->codegen();

	res += ") do\n";

	slash_t_count += 1;

	std::cout << "Generating While...\n";

	for(auto const& i : instructions) {

		std::cout << "Generating Instruction... (i->ifId: " << i->ifId << ", cmp->ifId: " << cmp->ifId << ")\n";

		if(i->ifId != cmp->ifId) {

			std::cout << "Instruction Accepted! (i->ifId: " << i->ifId << ", cmp->ifId: " << cmp->ifId << ")\n";

			std::string insCG = i->codegen();

			if(insCG != "") {
				
				res += GetSlashT();
				res += insCG;
				res += ";\n";
			}
		}
	}

	slash_t_count -= 1;

	if(is_haskell_style) {

		res += GetSlashT() + "end;\n";

		res += X86AssemblyAST::GetConditionBlock(cmp->conditionBlockName)->codegen();
	
		if(res[res.size() - 1] == '\n') {
			res.pop_back();
		}
	
		if(res[res.size() - 1] == ';') {
			res.pop_back();
		}
	}
	else {

		res += GetSlashT() + "end";
	}

	return res;
}

std::string X86AssemblyAST::ConditionBlock::codegen() {

	std::string res;

	for(auto const& i : instructions) {

		std::string insCG = i->codegen();

		if(insCG != "") {
			
			res += GetSlashT();
			res += insCG;
			res += ";\n";
		}

	}

	return res;
}