#include "X86AssemblyAST.hpp"

std::vector<std::unique_ptr<X86AssemblyAST::ConditionBlock>> X86AssemblyAST::allConditionBlocks;

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

	for(auto const& i : registers) {

		res += "\t";
		res += i->codegen();
		res += ";\n";
	}

	for(auto const& i : stack) {

		res += "\t";
		res += i->codegen();
		res += ";\n";
	}

	for(auto const& i : instructions) {

		res += "\t";
		res += i->codegen();
		res += ";\n";
	}

	if(dynamic_cast<X86AssemblyAST::Return*>(instructions[instructions.size() - 1].get()) == nullptr) {
		res += "return eax;\n";
	}

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

std::string JumpInstructionToCompare(std::string comp) {

	if(comp == "jge") { return "IsMoreThanOrEquals"; }
	if(comp == "jle") { return "IsLessThanOrEquals"; }

	if(comp == "jg") { return "IsMoreThan"; }
	if(comp == "jl") { return "IsLessThan"; }

	if(comp == "je") { return "IsEquals"; }
	if(comp == "jne") { return "IsNotEquals"; }

	return "";
}

std::string X86AssemblyAST::If::codegen() {

	std::string res;

	res += "if COMPARE.";

	res += JumpInstructionToCompare(cmpType);
	res += "(";

	res += B->codegen();
	res += ", ";
	res += A->codegen();

	res += ") then\n";

	res += X86AssemblyAST::GetConditionBlock(conditionBlockName)->codegen();

	res += "\tend";

	return res;
}

std::string X86AssemblyAST::While::codegen() {

	std::string res;

	res += "while COMPARE.";

	res += JumpInstructionToCompare(cmp->cmpType);
	res += "(";

	res += cmp->B->codegen();
	res += ", ";
	res += cmp->A->codegen();

	res += ") do\n";

	for(auto const& i : instructions) {

		if(i->ifId != cmp->ifId) {
			
			res += "\t";
			res += i->codegen();
			res += ";\n";
		}
	}

	res += "end;\n";

	res += X86AssemblyAST::GetConditionBlock(cmp->conditionBlockName)->codegen();

	return res;
}

std::string X86AssemblyAST::ConditionBlock::codegen() {

	std::string res;

	for(auto const& i : instructions) {

		res += "\t";
		res += i->codegen();
		res += ";\n";

	}

	return res;
}