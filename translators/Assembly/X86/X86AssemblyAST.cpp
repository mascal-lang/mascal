#include "X86AssemblyAST.hpp"

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

std::string X86AssemblyAST::Function::codegen() {

	std::string res;

	if(name == "main") {
		res += "program begin\n";
	}
	else {
		res += "proc ";
		res += name;
		res += "() begin\n";
	}

	for(auto const& i : initializers) {

		res += i->codegen();
		res += ";\n";
	}

	for(auto const& i : instructions) {

		res += i->codegen();
		res += ";\n";
	}

	res += "end\n";

	return res;
}

std::string X86AssemblyAST::Add::codegen() {

	std::string res;

	res += "add ";
	res += target->codegen();
	res += ", ";
	res += value->codegen();

	return res;
}