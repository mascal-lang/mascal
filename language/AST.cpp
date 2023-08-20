#include "AST.hpp"

llvm::Function* AST::Program::codegen() {

	std::vector<llvm::Type*> llvmArgs;

	llvm::FunctionType* FT = llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(*CodeGen::TheContext), llvmArgs, false);

	llvm::Function* F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", CodeGen::TheModule.get());

	llvm::BasicBlock* BB = llvm::BasicBlock::Create(*CodeGen::TheContext, "entry", F);

	CodeGen::Builder->SetInsertPoint(BB);

	return F;
}