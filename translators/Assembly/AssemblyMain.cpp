#include "AssemblyMain.hpp"
#include <fstream>

void AssemblyTR::Start(std::string file) {

	std::ifstream t(file.c_str());

	std::string str((std::istreambuf_iterator<char>(t)),
  			         std::istreambuf_iterator<char>());

	X86AssemblyLexer::AddContent(str);

	X86AssemblyLexer::Start();

	std::cout << X86AssemblyParser::MainLoop() << "\n";
}