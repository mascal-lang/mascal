#include "Parser.hpp"

std::string Parser::main_target;
bool Parser::can_main_target_be_modified;

std::unordered_map<std::string, AST::Type*> Parser::all_parser_coms;
std::unordered_map<std::string, std::unique_ptr<Parser_Mem>> Parser::all_parser_mems;

std::vector<std::unique_ptr<AST::Procedure>> Parser::all_procedures;

std::string Parser::current_procedure_name;

AST::Attributes Parser::currentAttributes;

std::unique_ptr<AST::Expression> Parser::lastCompareOne;
std::unique_ptr<AST::Expression> Parser::lastCompareTwo;
int Parser::lastCmpType;

std::vector<std::string> Parser::allBlockNames;