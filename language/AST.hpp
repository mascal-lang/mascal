#ifndef AST_HPP
#define AST_HPP

#include "CodeGen.hpp"

#define NEW_TYPE(x) struct x : public Type { llvm::Type* codegen() override; }

struct AST {

	struct Type {

		virtual ~Type() = default;

		virtual llvm::Type* codegen() = 0;
	};

	NEW_TYPE(Integer32);

	struct Expression {

		virtual ~Expression() = default;

		std::string name;

		std::unique_ptr<Type> ty;

		virtual llvm::Value* codegen() = 0;
	};

	struct IntNumber : public Expression {

		int64_t num = 0;

		IntNumber(int64_t num_in, std::unique_ptr<Type> ty_in) {

			num = num_in;
			ty = std::move(ty_in);
		}

		llvm::Value* codegen() override;
	};

	struct Com : public Expression {

		std::unique_ptr<Expression> target;

		Com(std::string name_in, std::unique_ptr<Type> ty_in, std::unique_ptr<Expression> target_in) {
			
			name = name_in;
			ty = std::move(ty_in);
			target = std::move(target_in);
		}

		llvm::Value* codegen() override;
	};

	struct Program {

		std::vector<std::unique_ptr<AST::Expression>> all_instructions;

		Program(std::vector<std::unique_ptr<AST::Expression>> all_instructions) : all_instructions(std::move(all_instructions)) {}

		llvm::Function* codegen();
	};
};

#endif