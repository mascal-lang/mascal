#include "CodeGen.hpp"

std::unique_ptr<llvm::LLVMContext> 	CodeGen::TheContext;
std::unique_ptr<llvm::IRBuilder<>> 	CodeGen::Builder;
std::unique_ptr<llvm::Module> 		CodeGen::TheModule;

std::unordered_map<std::string, std::unique_ptr<LLVM_Com>> CodeGen::all_coms;
std::unordered_map<std::string, std::unique_ptr<LLVM_Mem>> CodeGen::all_mems;

bool CodeGen::releaseMode = false;

void CodeGen::Initialize()
{
	// Open a new context and module.
 	TheContext = std::make_unique<llvm::LLVMContext>();

 	TheModule = std::make_unique<llvm::Module>("Mascal", *TheContext);
 	//TheModule->setDataLayout(TheJIT->getDataLayout());

 	 // Create a new builder for the module.
 	Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
}