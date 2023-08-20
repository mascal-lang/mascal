#include "CodeGen.hpp"

std::unique_ptr<llvm::LLVMContext> 	CodeGen::TheContext;
std::unique_ptr<llvm::IRBuilder<>> 	CodeGen::Builder;
std::unique_ptr<llvm::Module> 		CodeGen::TheModule;

void CodeGen::Initialize()
{
	// Open a new context and module.
 	TheContext = std::make_unique<llvm::LLVMContext>();

 	TheModule = std::make_unique<llvm::Module>("Nucleus", *TheContext);
 	//TheModule->setDataLayout(TheJIT->getDataLayout());

 	 // Create a new builder for the module.
 	Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
}