#include "Parser.hpp"

std::string Parser::main_target;
bool Parser::can_main_target_be_modified;

std::unordered_map<std::string, AST::Type*> Parser::all_parser_coms;