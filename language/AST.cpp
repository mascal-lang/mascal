#include "AST.hpp"
#include <iostream>

llvm::Function* AST::Program::codegen() {

	std::vector<llvm::Type*> llvmArgs;

	llvm::FunctionType* FT = llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(*CodeGen::TheContext), llvmArgs, false);

	llvm::Function* F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", CodeGen::TheModule.get());

	llvm::BasicBlock* BB = llvm::BasicBlock::Create(*CodeGen::TheContext, "entry", F);

	CodeGen::Builder->SetInsertPoint(BB);

	for(int i = 0; i < all_instructions.size(); i++) {

		all_instructions[i]->codegen();
	}

	return F;
}

llvm::Type* AST::Integer32::codegen() { return llvm::IntegerType::getInt32Ty(*CodeGen::TheContext); }
llvm::Type* AST::Integer1::codegen() { return llvm::IntegerType::getInt1Ty(*CodeGen::TheContext); }

llvm::Value* AST::IntNumber::codegen() {

	llvm::Type* ty_codegen = ty->codegen();
	llvm::IntegerType* int_ty = nullptr;

	if(isa<llvm::IntegerType>(ty_codegen)) {
		int_ty = dyn_cast<llvm::IntegerType>(ty_codegen);
	}
	else {
		std::cout << "Oops!\n";
		exit(1);
	}

	return llvm::ConstantInt::get(*CodeGen::TheContext, llvm::APInt(int_ty->getBitWidth(), num, false));
}

llvm::Value* AST::Variable::codegen() {

	llvm::Value* result = AST::GetCurrentInstructionByName(name);

	if(result == nullptr) {

		std::cout << "Unknown variable '" << name << "'\n";
		exit(1);
	}

	return result;
}

llvm::Value* AST::Com::codegen() {

	llvm::Value* tc = target->codegen();

	std::unique_ptr<LLVM_Com> lcom = std::make_unique<LLVM_Com>();
	lcom->origin = tc;
	lcom->current = tc;

	CodeGen::all_coms[name] = std::move(lcom);

	//std::cout << name << " = ";
	//CodeGen::all_coms[name]->origin->print(llvm::outs());
	//std::cout << "\n";

	return CodeGen::all_coms[name]->origin;
}

llvm::Value* AST::Add::codegen() {

	llvm::Value* L = AST::GetOrCreateInstruction(target.get());
	llvm::Value* R = AST::GetOrCreateInstruction(value.get());

	llvm::Value* result = CodeGen::Builder->CreateAdd(L, R);

	AST::AddInstruction(target.get(), result);

	return result;
}

llvm::Value* AST::Sub::codegen() {

	llvm::Value* L = AST::GetOrCreateInstruction(target.get());
	llvm::Value* R = AST::GetOrCreateInstruction(value.get());

	llvm::Value* result = CodeGen::Builder->CreateSub(L, R);

	AST::AddInstruction(target.get(), result);

	return result;
}

llvm::Value* AST::Compare::codegen() {

	llvm::Value* L = AST::GetOrCreateInstruction(compareOne.get());
	llvm::Value* R = AST::GetOrCreateInstruction(compareTwo.get());

	if(cmp_type == AST::CompareType::IsLessThan) { return CodeGen::Builder->CreateICmpUGT(R, L, "cmptmp"); }
	if(cmp_type == AST::CompareType::IsMoreThan) { return CodeGen::Builder->CreateICmpUGT(L, R, "cmptmp"); }
	if(cmp_type == AST::CompareType::IsEquals) { return CodeGen::Builder->CreateICmpEQ(L, R, "cmptmp"); }
	if(cmp_type == AST::CompareType::IsNotEquals) { return CodeGen::Builder->CreateICmpNE(L, R, "cmptmp"); }
	if(cmp_type == AST::CompareType::IsLessThanOrEquals) { return CodeGen::Builder->CreateICmpUGE(R, L, "cmptmp"); }
	if(cmp_type == AST::CompareType::IsMoreThanOrEquals) { return CodeGen::Builder->CreateICmpUGE(L, R, "cmptmp"); }

	return nullptr;
}

llvm::Value* AST::If::codegen() {

	llvm::Value* conditionCodegen = AST::GetOrCreateInstruction(condition.get());

	llvm::Function *TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

	llvm::BasicBlock* EntryBlock = CodeGen::Builder->GetInsertBlock();

	llvm::BasicBlock* IfBlock = llvm::BasicBlock::Create(*CodeGen::TheContext, "if", TheFunction);
	llvm::BasicBlock* ElseBlock = nullptr;
	llvm::BasicBlock* ContinueBlock = llvm::BasicBlock::Create(*CodeGen::TheContext, "continue");

	// if(ElseBody.size() != 0)
	// {
	// 	ElseBlock = llvm::BasicBlock::Create(*CodeGen::TheContext, "else");
	// 	CodeGen::Builder->CreateCondBr(ConditionV, IfBlock, ElseBlock);
	// }
	// else 

	CodeGen::Builder->CreateCondBr(conditionCodegen, IfBlock, ContinueBlock);

	CodeGen::Builder->SetInsertPoint(IfBlock);

	for(auto const& i: if_body) {

		AST::SaveState(i->name, EntryBlock);
		i->codegen();
	}

	CodeGen::Builder->CreateBr(ContinueBlock);

	TheFunction->getBasicBlockList().push_back(ContinueBlock);
	CodeGen::Builder->SetInsertPoint(ContinueBlock);

	AST::CreateIfPHIs(ContinueBlock);

	return nullptr;
}

llvm::Value* AST::GetCurrentInstruction(AST::Expression* e) {

	return AST::GetCurrentInstructionByName(e->name);
}

llvm::Value* AST::GetCurrentInstructionByName(std::string name) {

	if(CodeGen::all_coms.find(name) != CodeGen::all_coms.end()) {
		return CodeGen::all_coms[name]->current;
	}

	return nullptr;
}

llvm::Value* AST::GetOrCreateInstruction(AST::Expression* e) {

	llvm::Value* result = AST::GetCurrentInstruction(e);

	if(result == nullptr)
		return e->codegen();

	return result;
}

void AST::AddInstruction(AST::Expression* e, llvm::Value* l) {

	AST::AddInstructionToName(e->name, l);
}

void AST::AddInstructionToName(std::string name, llvm::Value* l) {

	if(CodeGen::all_coms.find(name) != CodeGen::all_coms.end()) {
		CodeGen::all_coms[name]->current = l;
	}
}

llvm::Value* AST::LLReturn::codegen() {

	return CodeGen::Builder->CreateRet(AST::GetOrCreateInstruction(target.get()));
}

void AST::SaveState(std::string name, llvm::BasicBlock* bb) {

	if(CodeGen::all_coms.find(name) != CodeGen::all_coms.end()) {
		
		std::string bbName = std::string(bb->getName());
		if(CodeGen::all_coms[name]->states.find(bbName) == CodeGen::all_coms[name]->states.end()) {

			CodeGen::all_coms[name]->states[bbName] = CodeGen::all_coms[name]->current;
		}
	}
}

void AST::CreateIfPHIs(llvm::BasicBlock* continueBlock) {

	std::vector<llvm::BasicBlock*> blockPreds;

	for (auto it = llvm::pred_begin(continueBlock), et = llvm::pred_end(continueBlock); it != et; ++it)
	{
		llvm::BasicBlock* predecessor = *it;

		blockPreds.push_back(predecessor);
	}

	UNORDERED_MAP_FOREACH(std::string, std::unique_ptr<LLVM_Com>, CodeGen::all_coms, it) {

		llvm::Value* entryValue = nullptr;
		llvm::Value* ifValue = nullptr;
		
		std::string bbPreds0 = std::string(blockPreds[1]->getName());

		if(it->second->states.find(bbPreds0) != it->second->states.end()) {

			entryValue = it->second->states[bbPreds0];
	
			ifValue = it->second->current;
	
			auto phi = CodeGen::Builder->CreatePHI(ifValue->getType(), 2, "phi");
	
			phi->addIncoming(ifValue, blockPreds[0]);
			phi->addIncoming(entryValue, blockPreds[1]);

			it->second->current = phi;
		}
	}
}