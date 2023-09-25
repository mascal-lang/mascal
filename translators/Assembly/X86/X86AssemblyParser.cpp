#include "X86AssemblyParser.hpp"

std::vector<std::unique_ptr<X86AssemblyAST::Expression>> X86AssemblyParser::astRegisters;
std::vector<std::unique_ptr<X86AssemblyAST::RAM>> X86AssemblyParser::stackMemory;
std::vector<std::string> X86AssemblyParser::conditionJumpList;