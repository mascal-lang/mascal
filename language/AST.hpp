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

	struct Variable : public Expression {

		Variable(std::string name_in) {

			name = name_in;
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

	struct LLReturn : public Expression {

		std::unique_ptr<Expression> target;

		LLReturn(std::unique_ptr<Expression> target_in) {

			target = std::move(target_in);
		}

		llvm::Value* codegen() override;
	};

	struct Add : public Expression {

		std::unique_ptr<Expression> target;
		std::unique_ptr<Expression> value;

		Add(std::unique_ptr<Expression> target_in, std::unique_ptr<Expression> value_in) {

			target = std::move(target_in);
			value = std::move(value_in);

			name = target->name;
		}

		llvm::Value* codegen() override;
	};

	struct Program {

		std::vector<std::unique_ptr<AST::Expression>> all_instructions;

		Program(std::vector<std::unique_ptr<AST::Expression>> all_instructions) : all_instructions(std::move(all_instructions)) {}

		llvm::Function* codegen();
	};

	static llvm::Value* GetCurrentInstruction(AST::Expression* e);
	static llvm::Value* GetCurrentInstructionByName(std::string name);

	static llvm::Value* GetOrCreateInstruction(AST::Expression* e);

	static void AddInstruction(AST::Expression* e, llvm::Value* l);
	static void AddInstructionToName(std::string name, llvm::Value* l);
};

#endif