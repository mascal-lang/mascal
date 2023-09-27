#ifndef AST_HPP
#define AST_HPP

#include "CodeGen.hpp"

#define NEW_TYPE(x, y) struct x : public Type { llvm::Type* codegen() override; std::string ToLLMascal() override { y } std::unique_ptr<Type> Clone() override { return std::make_unique<x>(); } }

#define EXPR_OBJ() std::unique_ptr<Expression>
#define EXPR_OBJ_VECTOR() std::vector<std::unique_ptr<Expression>>

#define TYPE_OBJ() std::unique_ptr<Type>
#define TYPE_OBJ_VECTOR() std::vector<std::unique_ptr<Type>>

#define MAP_FOREACH(x, y, z, w) for(std::map<x, y>::iterator w = z.begin(); w != z.end(); ++w)
#define MAP_FOREACH_PAIR(x, y1, y2, z, w) for(std::map<x, std::pair<y1, y2>>::iterator w = z.begin(); w != z.end(); ++w)

#define UNORDERED_MAP_FOREACH(x, y, z, w) for(std::unordered_map<x, y>::iterator w = z.begin(); w != z.end(); ++w)
#define UNORDERED_MAP_FOREACH_PAIR(x, y1, y2, z, w) for(std::unordered_map<x, std::pair<y1, y2>>::iterator w = z.begin(); w != z.end(); ++w)

#define CLONE_EXPR_VECTOR(x, y) std::vector<std::unique_ptr<AST::Expression>> y; for(auto const& i: x) { auto z = i->Clone(); y.push_back(std::move(z)); }
#define CLONE_TYPE_VECTOR(x, y) std::vector<std::unique_ptr<AST::Type>> y; for(auto const& i: x) { auto z = i->Clone(); y.push_back(std::move(z)); }
#define CLONE_AST_CUSTOM_VECTOR(z, x, y) std::vector<std::unique_ptr<z>> y; for(auto const& i: x) { auto z = i->Clone(); y.push_back(std::move(z)); }

#define EMPTY_TOLLMASCAL() std::string ToLLMascal() override { return ""; }

#define DEFAULT_TOLLMASCALBEFORE() std::string ToLLMascalBefore() override { return ""; }

#define DEFAULT_REPLACE_TARGET_NAME_TO() void ReplaceTargetNameTo(std::string from, std::string to) override { return; }

#define DEFAULT_CONTAINS_NAME() bool ContainsName(std::string str) override { return false; }

struct AST {

	struct Type {

		virtual ~Type() = default;

		virtual llvm::Type* codegen() = 0;

		virtual std::string ToLLMascal() = 0;

		virtual std::unique_ptr<Type> Clone() = 0;
	};

	NEW_TYPE(Integer128, return "i128"; );
	NEW_TYPE(Integer64, return "i64"; );
	NEW_TYPE(Integer32, return "i32"; );
	NEW_TYPE(Integer16, return "i16"; );
	NEW_TYPE(Integer8, return "i8"; );
	NEW_TYPE(Integer1, return "i1"; );

	NEW_TYPE(Void, return "void"; );

	static int slash_t_count;

	static std::string GetSlashT() {

		std::string res;
		for(int i = 0; i < slash_t_count; i++) {
			res += "\t";
		}

		return res;
	}

	struct Expression {

		virtual ~Expression() = default;

		std::string name;

		std::unique_ptr<Type> ty;

		llvm::BasicBlock* parent_entry_block = nullptr;

		virtual llvm::Value* codegen() = 0;

		virtual std::string ToLLMascal() = 0;

		virtual std::string ToLLMascalBefore() = 0;

		virtual void ReplaceTargetNameTo(std::string from, std::string to) = 0;

		virtual bool ContainsName(std::string s) = 0;

		virtual std::unique_ptr<Expression> Clone() = 0;
	};

	struct IntNumber : public Expression {

		int64_t num = 0;

		IntNumber(int64_t num_in, std::unique_ptr<Type> ty_in) {

			num = num_in;
			ty = std::move(ty_in);
		}

		llvm::Value* codegen() override;

		DEFAULT_TOLLMASCALBEFORE()
		DEFAULT_REPLACE_TARGET_NAME_TO()
		DEFAULT_CONTAINS_NAME()

		std::string ToLLMascal() override {
			return std::to_string(num);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<IntNumber>(num, ty->Clone());
		}
	};

	struct Variable : public Expression {

		EXPR_OBJ_VECTOR() initializers;

		Variable(std::string name_in, EXPR_OBJ_VECTOR() initializers_in = {} ) {

			initializers = std::move(initializers_in);
			name = name_in;
		}

		llvm::Value* codegen() override;

		std::string ToLLMascal() override {
			return name;
		}

		std::string ToLLMascalBefore() override {

			std::string res;

			for(auto const& i : initializers) {

				if(i->ToLLMascalBefore() != "") {

					res += i->ToLLMascalBefore();
					res += "\n";
					res += GetSlashT();
				}
			}

			return res;
		}

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			if(name == from) {
				name = to;
			}

			for(auto const& i : initializers) {
				i->ReplaceTargetNameTo(from, to);
			}
		}

		bool ContainsName(std::string str) override {

			return name == str;
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<Variable>(name);
		}
	};

	struct Com : public Expression {

		EXPR_OBJ() target;

		Com(std::string name_in, std::unique_ptr<Type> ty_in, EXPR_OBJ() target_in) {

			name = name_in;
			ty = std::move(ty_in);
			target = std::move(target_in);
		}

		llvm::Value* codegen() override;

		std::string ToLLMascal() override {

			std::string res;

			if(target->ToLLMascalBefore() != "") {

				res += target->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			res += "com ";
			res += name;
			res += ": ";
			res += ty->ToLLMascal();
			res += " = ";
			res += target->ToLLMascal();
			res += ";";

			return res;
		}

		DEFAULT_TOLLMASCALBEFORE()

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			if(name == from) {
				name = to;
			}

			target->ReplaceTargetNameTo(from, to);
		}

		bool ContainsName(std::string str) override {

			return name == str || target->ContainsName(str);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<Com>(name, ty->Clone(), target->Clone());
		}
	};

	struct Mem : public Expression {

		EXPR_OBJ() target;

		Mem(std::string name_in, std::unique_ptr<Type> ty_in, EXPR_OBJ() target_in) {

			name = name_in;
			ty = std::move(ty_in);
			target = std::move(target_in);
		}

		llvm::Value* codegen() override;

		std::string ToLLMascal() override {

			std::string res;

			if(target->ToLLMascalBefore() != "") {

				res += target->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			res += "mem ";
			res += name;
			res += ": ";
			res += ty->ToLLMascal();
			res += " = ";
			res += target->ToLLMascal();
			res += ";";

			return res;
		}

		DEFAULT_TOLLMASCALBEFORE()

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			if(name == from) {
				name = to;
			}

			target->ReplaceTargetNameTo(from, to);
		}

		bool ContainsName(std::string str) override {

			return name == str || target->ContainsName(str);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<Mem>(name, ty->Clone(), target->Clone());
		}
	};

	struct RetVoid : public Expression {

		RetVoid() {}

		llvm::Value* codegen() override;

		EMPTY_TOLLMASCAL()
		DEFAULT_TOLLMASCALBEFORE()
		DEFAULT_REPLACE_TARGET_NAME_TO()
		DEFAULT_CONTAINS_NAME()

		EXPR_OBJ() Clone() override {

			return std::make_unique<RetVoid>();
		}
	};

	struct LLReturn : public Expression {

		EXPR_OBJ() target;

		LLReturn(EXPR_OBJ() target_in) {

			target = std::move(target_in);
		}

		llvm::Value* codegen() override;

		std::string ToLLMascal() override {

			std::string res;

			if(target->ToLLMascalBefore() != "") {

				res += target->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			res += "llreturn ";
			res += target->ToLLMascal();
			res += ";";

			return res;
		}

		DEFAULT_TOLLMASCALBEFORE()

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			target->ReplaceTargetNameTo(from, to);
		}

		bool ContainsName(std::string str) override {

			return name == str || target->ContainsName(str);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<LLReturn>(target->Clone());
		}
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

		std::string ToLLMascal() override {

			std::string res;

			if(value->ToLLMascalBefore() != "") {

				res += value->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			res += "add ";
			res += target->ToLLMascal();
			res += ", ";
			res += value->ToLLMascal();
			res += ";";

			return res;
		}

		DEFAULT_TOLLMASCALBEFORE()

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			if(name == from) {
				name = to;
			}

			target->ReplaceTargetNameTo(from, to);
			value->ReplaceTargetNameTo(from, to);
		}

		bool ContainsName(std::string str) override {

			return name == str || target->ContainsName(str) || value->ContainsName(str);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<Add>(target->Clone(), value->Clone());
		}
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

		std::string ToLLMascal() override {

			std::string res;

			if(value->ToLLMascalBefore() != "") {

				res += value->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			res += "sub ";
			res += target->ToLLMascal();
			res += ", ";
			res += value->ToLLMascal();
			res += ";";

			return res;
		}

		DEFAULT_TOLLMASCALBEFORE()

		bool ContainsName(std::string str) override {

			return name == str || target->ContainsName(str) || value->ContainsName(str);
		}

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			if(name == from) {
				name = to;
			}

			target->ReplaceTargetNameTo(from, to);
			value->ReplaceTargetNameTo(from, to);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<Sub>(target->Clone(), value->Clone());
		}
	};

	struct IntCast : public Expression {

		EXPR_OBJ() target;
		TYPE_OBJ() intType;

		IntCast(EXPR_OBJ() target_in, TYPE_OBJ() intType_in) {

			target = std::move(target_in);
			intType = std::move(intType_in);
		}

		llvm::Value* codegen() override;

		std::string ToLLMascal() override {

			std::string res;

			res += "intcast ";
			res += target->ToLLMascal();
			res += " to ";
			res += intType->ToLLMascal();

			return res;
		}

		std::string ToLLMascalBefore() override {

			std::string res;

			if(target->ToLLMascalBefore() != "") {

				res += target->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			return res;
		}

		bool ContainsName(std::string str) override {

			return target->ContainsName(str);
		}

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			target->ReplaceTargetNameTo(from, to);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<IntCast>(target->Clone(), intType->Clone());
		}
	};

	struct ComStore : public Expression {

		EXPR_OBJ() target;
		EXPR_OBJ() value;

		ComStore(EXPR_OBJ() target_in, EXPR_OBJ() value_in) {

			target = std::move(target_in);
			value = std::move(value_in);

			name = target->name;
		}

		llvm::Value* codegen() override;

		std::string ToLLMascal() override {

			std::string res;

			if(value->ToLLMascalBefore() != "") {

				res += value->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			res += "comstore ";
			res += target->ToLLMascal();
			res += ", ";
			res += value->ToLLMascal();
			res += ";";

			return res;
		}

		DEFAULT_TOLLMASCALBEFORE()

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			if(name == from) {
				name = to;
			}

			target->ReplaceTargetNameTo(from, to);
			value->ReplaceTargetNameTo(from, to);
		}

		bool ContainsName(std::string str) override {

			return name == str || target->ContainsName(str) || value->ContainsName(str);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<ComStore>(target->Clone(), value->Clone());
		}
	};

	struct LoadMem : public Expression {

		EXPR_OBJ() target;

		LoadMem(EXPR_OBJ() target_in) {

			target = std::move(target_in);
		}

		llvm::Value* codegen() override;

		std::string ToLLMascalBefore() override {

			std::string res;

			if(target->ToLLMascalBefore() != "") {

				res += target->ToLLMascalBefore();
				res += "\n";
			}

			return res;
		}

		std::string ToLLMascal() override {

			std::string res;

			res += "loadmem ";
			res += target->ToLLMascal();

			return res;
		}

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			target->ReplaceTargetNameTo(from, to);
		}

		bool ContainsName(std::string str) override {

			return name == str || target->ContainsName(str);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<LoadMem>(target->Clone());
		}
	};

	struct MemStore : public Expression {

		EXPR_OBJ() target;
		EXPR_OBJ() value;

		MemStore(EXPR_OBJ() target_in, EXPR_OBJ() value_in) {

			target = std::move(target_in);
			value = std::move(value_in);

			name = target->name;
		}

		llvm::Value* codegen() override;

		std::string ToLLMascal() override {

			std::string res;

			if(value->ToLLMascalBefore() != "") {

				res += value->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			res += "memstore ";
			res += target->ToLLMascal();
			res += ", ";
			res += value->ToLLMascal();
			res += ";";

			return res;
		}

		DEFAULT_TOLLMASCALBEFORE()

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			if(name == from) {
				name = to;
			}

			target->ReplaceTargetNameTo(from, to);
			value->ReplaceTargetNameTo(from, to);
		}

		bool ContainsName(std::string str) override {

			return name == str || target->ContainsName(str) || value->ContainsName(str);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<MemStore>(target->Clone(), value->Clone());
		}
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

		std::string ToLLMascal() override {

			std::string res;

			if(compareOne->ToLLMascalBefore() != "") {

				res += compareOne->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			if(compareTwo->ToLLMascalBefore() != "") {

				res += compareTwo->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			res += "COMPARE.";
			
			if(cmp_type == CompareType::IsLessThan) { res += "IsLessThan"; }
			else if(cmp_type == CompareType::IsMoreThan) { res += "IsMoreThan"; }
			else if(cmp_type == CompareType::IsEquals) { res += "IsEquals"; }
			else if(cmp_type == CompareType::IsNotEquals) { res += "IsNotEquals"; }
			else if(cmp_type == CompareType::IsLessThanOrEquals) { res += "IsLessThanOrEquals"; }
			else if(cmp_type == CompareType::IsMoreThanOrEquals) { res += "IsMoreThanOrEquals"; }

			res += "(";
			res += compareOne->ToLLMascal();
			res += ", ";
			res += compareTwo->ToLLMascal();
			res += ")";

			return res;
		}

		DEFAULT_TOLLMASCALBEFORE()

		bool ContainsName(std::string str) override {

			return name == str || compareOne->ContainsName(str) || compareTwo->ContainsName(str);
		}

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			if(name == from) {
				name = to;
			}

			compareOne->ReplaceTargetNameTo(from, to);
			compareTwo->ReplaceTargetNameTo(from, to);
		}

		EXPR_OBJ() Clone() override {

			return std::make_unique<Compare>(compareOne->Clone(), compareTwo->Clone(), cmp_type);
		}
	};

	struct While : public Expression {

		EXPR_OBJ() condition;
		EXPR_OBJ() repeat_condition;
		EXPR_OBJ_VECTOR() loop_body;

		While(EXPR_OBJ() condition_in, EXPR_OBJ() repeat_condition_in, EXPR_OBJ_VECTOR() loop_body_in) {

			condition = std::move(condition_in);
			repeat_condition = std::move(repeat_condition_in);
			loop_body = std::move(loop_body_in);
		}

		llvm::Value* codegen() override;

		std::string ToLLMascal() override {

			std::string res;

			if(condition->ToLLMascalBefore() != "") {

				res += condition->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			res += "while ";
			
			res += condition->ToLLMascal();
			res += " do\n";

			slash_t_count += 1;

			for(auto const& i: loop_body) {
				res += GetSlashT();
				res += i->ToLLMascalBefore();
				res += "\n";

				res += GetSlashT();
				res += i->ToLLMascal();
				res += "\n";
			}

			slash_t_count -= 1;

			res += GetSlashT() + "end;\n";
			
			return res;
		}

		DEFAULT_TOLLMASCALBEFORE()

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			condition->ReplaceTargetNameTo(from, to);
			repeat_condition->ReplaceTargetNameTo(from, to);

			for(auto const& i : loop_body) {

				i->ReplaceTargetNameTo(from, to);
			}
		}

		bool ContainsName(std::string str) override {

			if(condition->ContainsName(str)) {
				return true;
			}

			if(repeat_condition->ContainsName(str)) {
				return true;
			}

			for(auto const& i : loop_body) {
				if(i->ContainsName(str)) {
					return true;
				}
			}

			return false;
		}

		EXPR_OBJ() Clone() override {

			CLONE_EXPR_VECTOR(loop_body, clone_loop_body);

			return std::make_unique<While>(condition->Clone(), repeat_condition->Clone(), std::move(clone_loop_body));
		}
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

		std::string ToLLMascal() override {

			std::string res;

			if(condition->ToLLMascalBefore() != "") {

				res += condition->ToLLMascalBefore();
				res += "\n";
				res += GetSlashT();
			}

			res += "if ";
			
			res += condition->ToLLMascal();
			res += " then\n";

			slash_t_count += 1;

			for(auto const& i: if_body) {
				res += GetSlashT();
				res += i->ToLLMascalBefore();
				res += "\n";

				res += GetSlashT();
				res += i->ToLLMascal();
				res += "\n";
			}

			slash_t_count -= 1;

			if(else_body.size() != 0) {
				
				res += GetSlashT() + "else then\n";
	
				slash_t_count += 1;
	
				for(auto const& i: else_body) {
					res += GetSlashT();
					res += i->ToLLMascalBefore();
					res += "\n";

					res += GetSlashT();
					res += i->ToLLMascal();
					res += "\n";
				}
	
				slash_t_count -= 1;
			}

			res += GetSlashT() + "end;\n";
			
			return res;
		}

		DEFAULT_TOLLMASCALBEFORE()

		void ReplaceTargetNameTo(std::string from, std::string to) override {

			condition->ReplaceTargetNameTo(from, to);

			for(auto const& i : if_body) {

				i->ReplaceTargetNameTo(from, to);
			}

			for(auto const& i : else_body) {

				i->ReplaceTargetNameTo(from, to);
			}
		}

		bool ContainsName(std::string str) override {

			if(condition->ContainsName(str)) {
				return true;
			}

			for(auto const& i : if_body) {
				if(i->ContainsName(str)) {
					return true;
				}
			}

			for(auto const& i : else_body) {
				if(i->ContainsName(str)) {
					return true;
				}
			}

			return false;
		}

		EXPR_OBJ() Clone() override {

			CLONE_EXPR_VECTOR(if_body, clone_if_body);
			CLONE_EXPR_VECTOR(else_body, clone_else_body);

			return std::make_unique<If>(condition->Clone(), std::move(clone_if_body), std::move(clone_else_body));
		}
	};

	struct ProcedureCall : public Expression {

		EXPR_OBJ_VECTOR() body;
		EXPR_OBJ() return_obj;

		ProcedureCall(EXPR_OBJ_VECTOR() body_in, EXPR_OBJ() return_obj_in) {

			body = std::move(body_in);
			return_obj = std::move(return_obj_in);
		}

		llvm::Value* codegen() override;

		std::string ToLLMascal() override {
			return return_obj->ToLLMascal();
		}

		std::string ToLLMascalBefore() override {

			std::string res;

			for(auto const& i: body) {

				if(i->ToLLMascalBefore() != "") {

					res += i->ToLLMascalBefore();
					res += "\n";
					res += GetSlashT();
				}

				res += GetSlashT();
				res += i->ToLLMascal();
				res += "\n";
			}

			return res;
		}

		DEFAULT_REPLACE_TARGET_NAME_TO()

		DEFAULT_CONTAINS_NAME()

		EXPR_OBJ() Clone() override {

			CLONE_EXPR_VECTOR(body, body_clone);

			return std::make_unique<AST::ProcedureCall>(std::move(body_clone), return_obj->Clone());
		}

	};

	struct Procedure {

		std::string procName;

		std::vector<std::string> all_argument_var_types;
		EXPR_OBJ_VECTOR() all_arguments;
		TYPE_OBJ_VECTOR() all_argument_types;

		TYPE_OBJ() proc_type;

		EXPR_OBJ_VECTOR() body;

		int call_count = 0;

		Procedure(std::string procName_in, std::vector<std::string> all_argument_var_types_in, EXPR_OBJ_VECTOR() all_arguments_in, TYPE_OBJ_VECTOR() all_argument_types_in, TYPE_OBJ() proc_type_in) {

			procName = procName_in;

			all_argument_var_types = all_argument_var_types_in;
			all_arguments = std::move(all_arguments_in);
			all_argument_types = std::move(all_argument_types_in);

			proc_type = std::move(proc_type_in);
		}

		Procedure(std::string procName_in, std::vector<std::string> all_argument_var_types_in, EXPR_OBJ_VECTOR() all_arguments_in, TYPE_OBJ_VECTOR() all_argument_types_in, TYPE_OBJ() proc_type_in, EXPR_OBJ_VECTOR() body_in) {

			procName = procName_in;

			all_argument_var_types = all_argument_var_types_in;
			all_arguments = std::move(all_arguments_in);
			all_argument_types = std::move(all_argument_types_in);

			proc_type = std::move(proc_type_in);

			body = std::move(body_in);
		}

		std::unique_ptr<AST::Procedure> Clone() {

			CLONE_EXPR_VECTOR(all_arguments, all_arguments_clone);
			CLONE_TYPE_VECTOR(all_argument_types, all_argument_types_clone);

			CLONE_EXPR_VECTOR(body, body_clone);

			auto proc = std::make_unique<AST::Procedure>(procName, all_argument_var_types, std::move(all_arguments_clone), std::move(all_argument_types_clone), proc_type->Clone(), std::move(body_clone));

			proc->call_count = call_count;

			return proc;
		}

		std::unique_ptr<AST::Procedure> CloneWithoutBody() {

			CLONE_EXPR_VECTOR(all_arguments, all_arguments_clone);
			CLONE_TYPE_VECTOR(all_argument_types, all_argument_types_clone);

			return std::make_unique<AST::Procedure>(procName, all_argument_var_types, std::move(all_arguments_clone), std::move(all_argument_types_clone), proc_type->Clone());
		}

		llvm::Value* codegen();
	};

	struct Attributes {

		bool isStackProtected = false;
		bool usesCStdLib = false;
	};

	struct Program {

		EXPR_OBJ_VECTOR() all_instructions;

		Attributes attrs;

		Program(EXPR_OBJ_VECTOR() all_instructions, Attributes attrs) : all_instructions(std::move(all_instructions)), attrs(attrs) {}

		llvm::Function* codegen();

		std::string ToLLMascal() {

			slash_t_count = 0;

			std::string res = "program begin\n";

			slash_t_count += 1;

			for(auto const& i: all_instructions) {

				if(i->ToLLMascalBefore() != "") {

					res += GetSlashT();
					res += i->ToLLMascalBefore();
					res += "\n";
				}

				res += GetSlashT();
				res += i->ToLLMascal();
				res += "\n";
			}

			slash_t_count -= 1;

			res += "end\n";

			return res;
		}
	};

	static llvm::Value* GetCurrentInstruction(AST::Expression* e);
	static llvm::Value* GetCurrentInstructionByName(std::string name);

	static llvm::Value* GetOrCreateInstruction(AST::Expression* e);

	static llvm::Value* GetAllocaFromMem(AST::Expression* e);

	static void AddInstruction(AST::Expression* e, llvm::Value* l);
	static void AddInstructionToName(std::string name, llvm::Value* l);

	static void GlobalSaveState(llvm::BasicBlock* bb);
	
	static void SaveState(std::string name, llvm::BasicBlock* bb);
	static void SetExistingState(std::string name, llvm::BasicBlock* bb);
	static llvm::Value* FindExistingState(std::string name, llvm::BasicBlock* bb);

	static void CreateIfPHIs(llvm::BasicBlock* continueBlock);
	static void CreateIfElsePHIs(llvm::BasicBlock* continueBlock);

	static bool IsInitializer(AST::Expression* t);
	static bool IsAlgorithm(AST::Expression* t);
};

#endif