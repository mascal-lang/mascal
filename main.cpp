#include <fstream>
#include "language/Lexer.hpp"
#include "language/Parser.hpp"
#include "language/CodeGen.hpp"

int main(int argc, char const *argv[])
{
	CodeGen::releaseMode = false;

	if(argc > 1) {

		std::string cmd = argv[1];

		if(cmd == "build") {

			//if(argc < 2) {
			//	cmd = argv[2];

			//	if(cmd == "release") {
			//		CodeGen::releaseMode = true;
			//	}
			//}

			std::ifstream t("main.mascal");
			std::string str((std::istreambuf_iterator<char>(t)),
  			             std::istreambuf_iterator<char>());
		
			CodeGen::Initialize();
		
			Lexer::AddContent(str);
		
			Lexer::Start();
		
			Parser::MainLoop();
		}
	}

	return 0;
}