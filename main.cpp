#include <fstream>
#include "language/Lexer.hpp"
#include "language/Parser.hpp"
#include "language/CodeGen.hpp"

#include "translators/Assembly/AssemblyMain.hpp"

int main(int argc, char const *argv[])
{
	CodeGen::releaseMode = false;

	if(argc > 1) {

		std::string cmd = argv[1];

		if(cmd == "build" || cmd == "emit") {

			std::ifstream t("main.mascal");
			std::string str((std::istreambuf_iterator<char>(t)),
  			             std::istreambuf_iterator<char>());
		
			CodeGen::Initialize();
		
			Lexer::AddContent(str);
		
			Lexer::Start();
			
			bool canBuild = cmd == "build";

			Parser::MainLoop(canBuild);
		}

		if(cmd == "translate") {

			AssemblyTR::Start("main.s");
		}
	}

	return 0;
}