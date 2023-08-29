#ifndef AST_HPP
#define AST_HPP

#include "CodeGen.hpp"

#define NEW_TYPE(x) struct x : public Type { llvm::Type* codegen() override; }

#define EXPR_OBJ() std::unique_ptr<Expression>
#define EXPR_OBJ_VECTOR() std::vector<std::unique_ptr<Expression>>

#define MAP_FOREACH(x, y, z, w) for(std::map<x, y>::iterator w = z.begin(); w != z.end(); ++w)
#define MAP_FOREACH_PAIR(x, y1, y2, z, w) for(std::map<x, std::pair<y1, y2>>::iterator w = z.begin(); w != z.end(); ++w)

#define UNORDERED_MAP_FOREACH(x, y, z, w) for(std::unordered_map<x, y>::iterator w = z.begin(); w != z.end(); ++w)
#define UNORDERED_MAP_FOREACH_PAIR(x, y1, y2, z, w) for(std::unordered_map<x, std::pair<y1, y2>>::iterator w = z.begin(); w != z.end(); ++w)

struct AST {

	struct Type {

		virtual ~Type() = default;

		virtual llvm::Type* codegen() = 0;
	};

	NEW_TYPE(Integer32);
	NEW_TYPE(Integer1);

	struct Expression {

		virtual ~Expression() = default;

		std::string name;

		std::unique_ptr<Type> ty;

		llvm::BasicBlock* parent_entry_block = nullptr;

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

		EXPR_OBJ() target;

		Com(std::string name_in, std::unique_ptr<Type> ty_in, EXPR_OBJ() target_in) {

			name = name_in;
			ty = std::move(ty_in);
			target = std::move(target_in);
		}

		llvm::Value* codegen() override;
	};

	struct LLReturn : public Expression {

		EXPR_OBJ() target;

		LLReturn(EXPR_OBJ() target_in) {

			target = std::move(target_in);
		}

		llvm::Value* codegen() override;
	};

	struct Add : public Expression {

		EXPR_OBJ() target;
		EXPR_OBJ() value;

		Add(EXPR_OBJ() target_in, EXPR_OBJ() value_in) {

			target = std::move(target_in);
			value = std::move(value_in);

			name = target->name;
		}

		llvm::Value* codegen() override;
	};

	struct Sub : public Expression {

		EXPR_OBJ() target;
		EXPR_OBJ() value;

		Sub(EXPR_OBJ() target_in, EXPR_OBJ() value_in) {

			target = std::move(target_in);
			value = std::move(value_in);

			name = target->name;
		}

		llvm::Value* codegen() override;
	};

	enum CompareType {
		IsLessThan,
		IsMoreThan,
		IsEquals,
		IsNotEquals,
		IsLessThanOrEquals,
		IsMoreThanOrEquals
	};

	struct Compare : public Expression {

		EXPR_OBJ() compareOne;
		EXPR_OBJ() compareTwo;

		int cmp_type;

		Compare(EXPR_OBJ() compareOne_in, EXPR_OBJ() compareTwo_in, int cmp_type_in) {

			compareOne = std::move(compareOne_in);
			compareTwo = std::move(compareTwo_in);

			cmp_type = cmp_type_in;
		}

		llvm::Value* codegen() override;
	};

	struct If : public Expression {

		EXPR_OBJ() condition;
		EXPR_OBJ_VECTOR() if_body;
		EXPR_OBJ_VECTOR() else_body;

		If(EXPR_OBJ() condition_in, EXPR_OBJ_VECTOR() if_body_in, EXPR_OBJ_VECTOR() else_body_in) {

			condition = std::move(condition_in);
			if_body = std::move(if_body_in);
			else_body = std::move(else_body_in);
		}

		llvm::Value* codegen() override;
	};

	struct Program {

		EXPR_OBJ_VECTOR() all_instructions;

		Program(EXPR_OBJ_VECTOR() all_instructions) : all_instructions(std::move(all_instructions)) {}

		llvm::Function* codegen();
	};

	static llvm::Value* GetCurrentInstruction(AST::Expression* e);
	static llvm::Value* GetCurrentInstructionByName(std::string name);

	static llvm::Value* GetOrCreateInstruction(AST::Expression* e);

	static void AddInstruction(AST::Expression* e, llvm::Value* l);
	static void AddInstructionToName(std::string name, llvm::Value* l);

	static void SaveState(std::string name, llvm::BasicBlock* bb);
	static void SetExistingState(std::string name, llvm::BasicBlock* bb);

	static void CreateIfPHIs(llvm::BasicBlock* continueBlock);

	static void CreateIfElsePHIs(llvm::BasicBlock* continueBlock);
};

#endif