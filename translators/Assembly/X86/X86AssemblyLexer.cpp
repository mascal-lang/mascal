#include "X86AssemblyLexer.hpp"

std::string X86AssemblyLexer::Content;
std::string X86AssemblyLexer::IdentifierStr;
std::string X86AssemblyLexer::NumValString;
int X86AssemblyLexer::CurrentToken;
int X86AssemblyLexer::Position;
int X86AssemblyLexer::Line;
int X86AssemblyLexer::Column;
std::string X86AssemblyLexer::line_as_string;
int X86AssemblyLexer::LastChar;
std::vector<std::string> X86AssemblyLexer::all_lines_vector;