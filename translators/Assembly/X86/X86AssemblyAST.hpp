#ifndef ASSEMBLY_AST_HPP
#define ASSEMBLY_AST_HPP

#include <vector>
#include <memory>
#include <iostream>

#define NEW_X86_TYPE(x) struct x : public Type { std::string codegen() override; std::unique_ptr<Type> Clone() override { return std::make_unique<x>(); } }

#define CLONE_X86EXPR_VECTOR(x, y) std::vector<std::unique_ptr<X86AssemblyAST::Expression>> y; for(auto const& i: x) { auto z = i->Clone(); y.push_back(std::move(z)); }

#define CLONE_X86RAM_VECTOR(x, y) std::vector<std::unique_ptr<RAM>> y;\
for(auto const& i: x) {\
 	auto z = i->CloneToRAM();\
 	y.push_back(std::move(z));\
}

struct X86AssemblyAST {

	static int slash_t_count;

	static std::string GetSlashT() {

		std::string res;

		for(int i = 0; i < slash_t_count; i++) {
			res += '\t';
		}

		return res;
	}

	struct Expression {

		std::string name;

		std::string asmType;

		int ifId = -1;

		virtual ~Expression() = default;

		virtual std::string codegen() = 0;

		virtual std::unique_ptr<Expression> Clone() = 0;
	};

	struct Type {

		virtual ~Type() = default;

		virtual std::string codegen() = 0;

		virtual std::unique_ptr<Type> Clone() = 0;
	};

	NEW_X86_TYPE(I32);

	struct Variable : public Expression {

		Variable(std::string name_in) {

			name = name_in;
		}

		std::string codegen() override;

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Variable>(name);
		}
	};

	struct IntNumber : public Expression {

		uint64_t numb;

		IntNumber(uint64_t numb_in) {
			numb = numb_in;
		}

		std::string codegen() override;

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<IntNumber>(numb);
		}
	};

	struct Register : public Expression {

		std::unique_ptr<Type> ty;

		Register(std::string name_in, std::unique_ptr<Type> ty_in) {

			name = name_in;
			ty = std::move(ty_in);
		}

		std::string codegen() override;

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Register>(name, ty->Clone());
		}
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

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<RAM>(name, pointerName, ty->Clone());
		}

		std::unique_ptr<RAM> CloneToRAM() {

			return std::make_unique<RAM>(name, pointerName, ty->Clone());
		}
	};

	struct Return : public Expression {

		Return() {}

		std::string codegen() override;

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Return>();
		}
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

		std::unique_ptr<Expression> Clone() override {

			CLONE_X86EXPR_VECTOR(instructions, clone_instructions)
			CLONE_X86EXPR_VECTOR(registers, clone_registers)

			CLONE_X86RAM_VECTOR(stack, clone_stack)

			return std::make_unique<Function>(name, attrs, std::move(clone_instructions), std::move(clone_registers), std::move(clone_stack));
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

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Add>(value->Clone(), target->Clone());
		}
	};

	struct Sub : public Expression {

		std::unique_ptr<Expression> value;
		std::unique_ptr<Expression> target;

		Sub(std::unique_ptr<Expression> value_in, std::unique_ptr<Expression> target_in) {

			value = std::move(value_in);
			target = std::move(target_in);
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Sub>(value->Clone(), target->Clone());
		}

		std::string codegen() override;
	};

	struct Xor : public Expression {

		std::unique_ptr<Expression> value;
		std::unique_ptr<Expression> target;

		Xor(std::unique_ptr<Expression> value_in, std::unique_ptr<Expression> target_in) {

			value = std::move(value_in);
			target = std::move(target_in);
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Xor>(value->Clone(), target->Clone());
		}

		std::string codegen() override;
	};

	struct Mov : public Expression {

		std::unique_ptr<Expression> value;
		std::unique_ptr<Expression> target;

		bool isMem = false;

		Mov(std::unique_ptr<Expression> value_in, std::unique_ptr<Expression> target_in, bool isMem_in = false) {

			value = std::move(value_in);
			target = std::move(target_in);
			isMem = isMem_in;
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Mov>(value->Clone(), target->Clone(), isMem);
		}

		std::string codegen() override;
	};

	struct Lea : public Expression {

		std::unique_ptr<Expression> value;
		std::unique_ptr<Expression> target;

		Lea(std::unique_ptr<Expression> value_in, std::unique_ptr<Expression> target_in) {

			value = std::move(value_in);
			target = std::move(target_in);
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Lea>(value->Clone(), target->Clone());
		}

		std::string codegen() override;
	};

	struct Push : public Expression {

		std::unique_ptr<Expression> target;

		Push(std::unique_ptr<Expression> target_in) {

			target = std::move(target_in);
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Push>(target->Clone());
		}

		std::string codegen() override;
	};

	struct Pop : public Expression {

		std::unique_ptr<Expression> target;

		Pop(std::unique_ptr<Expression> target_in) {

			target = std::move(target_in);
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Pop>(target->Clone());
		}

		std::string codegen() override;
	};

	struct Call : public Expression {

		std::unique_ptr<Expression> target;

		Call(std::unique_ptr<Expression> target_in) {

			target = std::move(target_in);
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Call>(target->Clone());
		}

		std::string codegen() override;
	};

	struct Comment : public Expression {

		std::string message;

		Comment(std::string message_in) {

			message = message_in;
		}

		std::string codegen() override;

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Comment>(message);
		}
	};

	struct Def : public Expression {

		int scl;
		int ty;

		Def(std::string name_in, int scl_in, int ty_in) {

			name = name_in;
			scl = scl_in;
			ty = ty_in;
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Def>(name, scl, ty);
		}

		std::string codegen() override;
	};

	struct Set : public Expression {

		std::unique_ptr<Expression> target;

		Set(std::string name_in, std::unique_ptr<Expression> target_in) {

			name = name_in;
			target = std::move(target_in);
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Set>(name, target->Clone());
		}

		std::string codegen() override;
	};

	struct File : public Expression {

		File(std::string fileName_in) {

			name = fileName_in;
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<File>(name);
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

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<P2Align>(bytes, limit);
		}

		std::string codegen() override;
	};

	struct EnableSEH : public Expression {

		EnableSEH() {}

		std::string codegen() override {
			return "";
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<EnableSEH>();
		}
	};

	struct EnableStdLib : public Expression {

		EnableStdLib() {}

		std::string codegen() override {
			return "";
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<EnableStdLib>();
		}
	};

	struct DoNothing : public Expression {

		DoNothing(std::string name_in = "") {

			name = name_in;
		}

		std::string codegen() override {
			return "";
		}

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<DoNothing>(name);
		}
	};

	struct ConditionBlock : public Expression {

		std::vector<std::unique_ptr<Expression>> instructions;

		ConditionBlock(std::string name_in, std::vector<std::unique_ptr<Expression>> instructions_in) {

			name = name_in;
			instructions = std::move(instructions_in);
		}

		std::string codegen() override;

		std::unique_ptr<Expression> Clone() override {

			CLONE_X86EXPR_VECTOR(instructions, clone_instructions)

			return std::make_unique<ConditionBlock>(name, std::move(clone_instructions));
		}
	};

	static std::vector<std::unique_ptr<X86AssemblyAST::ConditionBlock>> allConditionBlocks;

	static X86AssemblyAST::ConditionBlock* GetConditionBlock(std::string name) {

		for(auto const& i : allConditionBlocks) {
			if(i->name == name) {
				return i.get();
			}
		}

		std::cout << "'" << name << "' Condition Block Not Found!\n";
		exit(1);

		return nullptr;
	};

	/*
	struct TestVar : public Expression {

		std::unique_ptr<Expression> secondTarget;

		TestVar(std::string name_in, std::unique_ptr<Expression> target_in, std::unique_ptr<Expression> secondTarget_in) {

			name = name_in;
			target = std::move(target_in);
			secondTarget = std::move(secondTarget_in);

		}

		std::string codegen() override;

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<TestVar>(name, target->Clone(), secondTarget->Clone());
		}
	}
	*/

	struct If : public Expression {

		std::unique_ptr<Expression> A;
		std::unique_ptr<Expression> B;
		std::string cmpType;
		std::string conditionBlockName;
		std::string elseConditionBlockName;

		If(std::unique_ptr<Expression> A_in, std::unique_ptr<Expression> B_in, std::string cmpType_in, std::string conditionBlockName_in, std::string elseConditionBlockName_in) {

			A = std::move(A_in);
			B = std::move(B_in);
			cmpType = cmpType_in;
			conditionBlockName = conditionBlockName_in;
			elseConditionBlockName = elseConditionBlockName_in;
		}

		std::string codegen() override;

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<If>(A->Clone(), B->Clone(), cmpType, conditionBlockName, elseConditionBlockName);
		}

		std::unique_ptr<If> CloneToIf() {

			return std::make_unique<If>(A->Clone(), B->Clone(), cmpType, conditionBlockName, elseConditionBlockName);
		}
	};

	struct While : public Expression {

		std::unique_ptr<If> cmp;
		std::vector<std::unique_ptr<Expression>> instructions;
		bool is_haskell_style;

		While(std::unique_ptr<If> cmp_in, std::vector<std::unique_ptr<Expression>> instructions_in, bool is_haskell_style_in) {

			cmp = std::move(cmp_in);
			instructions = std::move(instructions_in);
			is_haskell_style = is_haskell_style_in;
		}

		std::string codegen() override;

		std::unique_ptr<Expression> Clone() override {

			CLONE_X86EXPR_VECTOR(instructions, clone_instructions)

			return std::make_unique<While>(cmp->CloneToIf(), std::move(clone_instructions), is_haskell_style);
		}
	};

	struct Jump : public Expression {

		std::string jumpBlockName;

		Jump(std::string to) {
			name = to;
		}

		std::string codegen() override;

		std::unique_ptr<Expression> Clone() override {

			return std::make_unique<Jump>(name);
		}
	};

	static bool IsJumpRecursive(Expression* expr, std::string functionName) {

		if(dynamic_cast<Jump*>(expr) != nullptr) {
			return expr->name == functionName;
		}

		return false;
	}

	static bool UsesCStdLib(Expression* expr) {
		return dynamic_cast<EnableStdLib*>(expr) != nullptr;
	}

	static bool IsSEH(Expression* expr) {
		return dynamic_cast<EnableSEH*>(expr) != nullptr;
	}

	static bool DoesNothing(Expression* expr) {
		return dynamic_cast<DoNothing*>(expr) != nullptr;
	}

	static bool IsStackPointer(std::string s) {
	
		if(s.size() == 3) {
	
			if(s[1] == 's' && s[2] == 'p') {
				return true;
			}
		}
	
		return false;
	}
	
	static bool IsBasePointer(std::string s) {
	
		if(s.size() == 3) {
	
			if(s[1] == 'b' && s[2] == 'p') {
				return true;
			}
		}
	
		return false;
	}
};

#endif