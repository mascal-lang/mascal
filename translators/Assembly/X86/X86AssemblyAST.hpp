#ifndef ASSEMBLY_AST_HPP
#define ASSEMBLY_AST_HPP

#include <vector>
#include <memory>

#define NEW_X86_TYPE(x) struct x : public Type { std::string codegen() override; }

struct X86AssemblyAST {

	struct Expression {

		std::string name;

		virtual ~Expression() = default;

		virtual std::string codegen() = 0;
	};

	struct Type {

		virtual ~Type() = default;

		virtual std::string codegen() = 0;
	};

	NEW_X86_TYPE(I32);

	struct Variable : public Expression {

		Variable(std::string name_in) {

			name = name_in;
		}

		std::string codegen() override;
	};

	struct IntNumber : public Expression {

		uint64_t numb;

		IntNumber(uint64_t numb_in) {
			numb = numb_in;
		}

		std::string codegen() override;
	};

	struct Register : public Expression {

		std::unique_ptr<Type> ty;

		Register(std::string name_in, std::unique_ptr<Type> ty_in) {

			name = name_in;
			ty = std::move(ty_in);
		}

		std::string codegen() override;
	};

	struct Return : public Expression {

		Return() {}

		std::string codegen() override;
	};

	struct Function : public Expression {

		std::vector<std::unique_ptr<Expression>> initializers;
		std::vector<std::unique_ptr<Expression>> instructions;

		Function(std::string name_in, std::vector<std::unique_ptr<Expression>> instructions_in, std::vector<std::unique_ptr<Expression>> initializers_in) {

			name = name_in;
			initializers = std::move(initializers_in);
			instructions = std::move(instructions_in);
		}

		std::string codegen() override;
	};

	struct Add : public Expression {

		std::unique_ptr<Expression> value;
		std::unique_ptr<Expression> target;

		Add(std::unique_ptr<Expression> value_in, std::unique_ptr<Expression> target_in) {

			value = std::move(value_in);
			target = std::move(target_in);
		}

		std::string codegen() override;
	};
};

#endif