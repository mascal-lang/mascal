#ifndef AST_HPP
#define AST_HPP

#include "CodeGen.hpp"

struct AST {

	struct Expression {

		virtual ~Expression() = default;

		bool is_unsigned = true;

		bool uncontinue = false;

		bool dont_share_history = false;

		bool is_initialized_by_call = false;

		bool is_explicit = false;

		bool is_data = false;

		std::unique_ptr<Expression> internal_length;

		virtual llvm::Value* codegen() = 0;
	};

	struct Program {

		Program() {}

		llvm::Function* codegen();
	};
};

#endif