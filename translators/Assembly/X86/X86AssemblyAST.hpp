#ifndef ASSEMBLY_AST_HPP
#define ASSEMBLY_AST_HPP

#include <vector>
#include <memory>

#define NEW_X86_TYPE(x) struct x : public Type { std::string codegen() override; }

struct X86AssemblyAST {

	struct Expression {

		std::string name;

		std::string asmType;

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

	struct RAM : public Expression {

		std::string pointerName;
		std::unique_ptr<Type> ty;

		RAM(std::string name_in, std::string pointerName_in, std::unique_ptr<Type> ty_in) {

			name = name_in;
			pointerName = pointerName_in;
			ty = std::move(ty_in);
		}

		std::string codegen() override;
	};

	struct Return : public Expression {

		Return() {}

		std::string codegen() override;
	};

	struct Attributes {

		bool isStackProtected = false;
		bool usesCStdLib = false;

		bool IsEverythingDisabled() {

			return !isStackProtected && !usesCStdLib;
		}

		std::string codegen() {

			std::string res = "";

			if(!IsEverythingDisabled()) {
				res += "[";
			}

			if(isStackProtected) { res += " StackProtected"; }
			if(usesCStdLib) { res += " CStdLib"; }

			if(!IsEverythingDisabled()) {
				res += " ]";
			}

			return res;
		}
	};

	struct Function : public Expression {

		std::vector<std::unique_ptr<Expression>> registers;
		std::vector<std::unique_ptr<RAM>> stack;

		std::vector<std::unique_ptr<Expression>> instructions;

		Attributes attrs;

		Function(std::string name_in, Attributes attrs_in,
			std::vector<std::unique_ptr<Expression>> instructions_in, 
			std::vector<std::unique_ptr<Expression>> registers_in,
			std::vector<std::unique_ptr<RAM>> stack_in) {

			name = name_in;

			attrs = attrs_in;

			registers = std::move(registers_in);
			stack = std::move(stack_in);

			instructions = std::move(instructions_in);
		}

		std::string codegen() override;
	};

	struct Add : public Expression {

		std::unique_ptr<Expression> value;
		std::unique_ptr<Expression> target;

		Add(std::unique_ptr<Expression> value_in, std::unique_ptr<Expression> target_in, std::string type) {

			value = std::move(value_in);
			target = std::move(target_in);
			asmType = type;
		}

		std::string codegen() override;
	};

	struct Sub : public Expression {

		std::unique_ptr<Expression> value;
		std::unique_ptr<Expression> target;

		Sub(std::unique_ptr<Expression> value_in, std::unique_ptr<Expression> target_in, std::string type) {

			value = std::move(value_in);
			target = std::move(target_in);
			asmType = type;
		}

		std::string codegen() override;
	};

	struct Mov : public Expression {

		std::unique_ptr<Expression> value;
		std::unique_ptr<Expression> target;

		bool isMem = false;

		Mov(std::unique_ptr<Expression> value_in, std::unique_ptr<Expression> target_in, std::string type, bool isMem_in = false) {

			value = std::move(value_in);
			target = std::move(target_in);
			isMem = isMem_in;
			asmType = type;
		}

		std::string codegen() override;
	};

	struct Lea : public Expression {

		std::unique_ptr<Expression> value;
		std::unique_ptr<Expression> target;

		Lea(std::unique_ptr<Expression> value_in, std::unique_ptr<Expression> target_in, std::string type) {

			value = std::move(value_in);
			target = std::move(target_in);
			asmType = type;
		}

		std::string codegen() override;
	};

	struct Push : public Expression {

		std::unique_ptr<Expression> target;

		Push(std::unique_ptr<Expression> target_in, std::string type) {

			target = std::move(target_in);
			asmType = type;
		}

		std::string codegen() override;
	};

	struct Pop : public Expression {

		std::unique_ptr<Expression> target;

		Pop(std::unique_ptr<Expression> target_in, std::string type) {

			target = std::move(target_in);
			asmType = type;
		}

		std::string codegen() override;
	};

	struct Call : public Expression {

		std::unique_ptr<Expression> target;

		Call(std::unique_ptr<Expression> target_in, std::string type) {

			target = std::move(target_in);
			asmType = type;
		}

		std::string codegen() override;
	};

	struct Comment : public Expression {

		std::string message;

		Comment(std::string message_in) {

			message = message_in;
		}

		std::string codegen() override;
	};

	struct Def : public Expression {

		int scl;
		int ty;

		Def(std::string name_in, int scl_in, int ty_in) {

			name = name_in;
			scl = scl_in;
			ty = ty_in;
		}

		std::string codegen() override;
	};

	struct Set : public Expression {

		std::unique_ptr<Expression> target;

		Set(std::string name_in, std::unique_ptr<Expression> target_in) {

			name = name_in;
			target = std::move(target_in);
		}

		std::string codegen() override;
	};

	struct File : public Expression {

		File(std::string fileName_in) {

			name = fileName_in;
		}

		std::string codegen() override;
	};

	struct P2Align : public Expression {

		int bytes;
		int limit;

		P2Align(int bytes_in, int limit_in) {

			bytes = bytes_in;
			limit = limit_in;
		}

		std::string codegen() override;
	};

	struct EnableSEH : public Expression {

		EnableSEH() {}

		std::string codegen() override {
			return "";
		}
	};

	struct EnableStdLib : public Expression {

		EnableStdLib() {}

		std::string codegen() override {
			return "";
		}
	};

	static bool UsesCStdLib(Expression* expr) {
		return dynamic_cast<EnableStdLib*>(expr) != nullptr;
	}

	static bool IsSEH(Expression* expr) {
		return dynamic_cast<EnableSEH*>(expr) != nullptr;
	}
};

#endif