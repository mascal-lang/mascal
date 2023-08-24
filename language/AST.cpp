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

llvm::Value* AST::Com::codegen() {

	llvm::Value* tc = target->codegen();

	std::unique_ptr<LLVM_Com> lcom = std::make_unique<LLVM_Com>();
	lcom->origin = tc;
	lcom->current = tc;

	CodeGen::all_coms[name] = std::move(lcom);

	std::cout << name << " = ";
	CodeGen::all_coms[name]->origin->print(llvm::outs());
	std::cout << "\n";

	return CodeGen::all_coms[name]->origin;
}