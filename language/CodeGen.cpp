#include "CodeGen.hpp"

std::unique_ptr<llvm::LLVMContext> 	CodeGen::TheContext;
std::unique_ptr<llvm::IRBuilder<>> 	CodeGen::Builder;
std::unique_ptr<llvm::Module> 		CodeGen::TheModule;

std::unordered_map<std::string, std::unique_ptr<LLVM_Com>> CodeGen::all_coms;
std::unordered_map<std::string, std::unique_ptr<LLVM_Mem>> CodeGen::all_mems;

bool CodeGen::releaseMode = false;

std::vector<llvm::PHINode*> CodeGen::all_phi_nodes;

void CodeGen::Initialize()
{
	// Open a new context and module.
 	TheContext = std::make_unique<llvm::LLVMContext>();

 	TheModule = std::make_unique<llvm::Module>("Mascal", *TheContext);
 	//TheModule->setDataLayout(TheJIT->getDataLayout());

 	 // Create a new builder for the module.
 	Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
}

void CodeGen::AddPHINodeToVec(llvm::PHINode* p) {

	CodeGen::all_phi_nodes.push_back(p);
}

void CodeGen::UpdateAllPHIPreds() {

	for(auto i : CodeGen::all_phi_nodes) {

		llvm::BasicBlock* parent = i->getParent();

		std::vector<llvm::BasicBlock*> blockPreds;

		for (auto it = llvm::pred_begin(parent), et = llvm::pred_end(parent); it != et; ++it)
		{
			llvm::BasicBlock* predecessor = *it;
			blockPreds.push_back(predecessor);
		}

		int bCount = 0;
		for(auto b : blockPreds) {

			i->setIncomingBlock(bCount, blockPreds[bCount]);
			bCount += 1;
		}
	}
}