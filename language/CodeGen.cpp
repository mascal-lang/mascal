#include "CodeGen.hpp"
#include "AST.hpp"
#include <iostream>

std::unique_ptr<llvm::LLVMContext> 	CodeGen::TheContext;
std::unique_ptr<llvm::IRBuilder<>> 	CodeGen::Builder;
std::unique_ptr<llvm::Module> 		CodeGen::TheModule;

std::unordered_map<std::string, std::unique_ptr<LLVM_Com>> CodeGen::all_coms;
std::unordered_map<std::string, std::unique_ptr<LLVM_Mem>> CodeGen::all_mems;

bool CodeGen::releaseMode = false;

std::vector<std::pair<std::string, llvm::PHINode*>> CodeGen::all_phi_nodes;

std::vector<llvm::BasicBlock*> CodeGen::pureBlocks;

void CodeGen::Initialize()
{
	// Open a new context and module.
 	TheContext = std::make_unique<llvm::LLVMContext>();

 	TheModule = std::make_unique<llvm::Module>("Mascal", *TheContext);
 	//TheModule->setDataLayout(TheJIT->getDataLayout());

 	 // Create a new builder for the module.
 	Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
}

void CodeGen::AddPHINodeToVec(std::string name, llvm::PHINode* p) {

	CodeGen::all_phi_nodes.push_back(std::make_pair(name, p));
}

llvm::Value* CodeGen::Default(llvm::Value* v) {

	return CodeGen::DefaultFromType(v->getType());
}

llvm::Constant* CodeGen::DefaultFromType(llvm::Type* t, llvm::Type* arrayElementT) {

	if(dyn_cast<llvm::IntegerType>(t) != nullptr) {

		auto intType = dyn_cast<llvm::IntegerType>(t);

		return llvm::ConstantInt::get(*CodeGen::TheContext, llvm::APInt(intType->getBitWidth(), 0, true));
	}

	else if(dyn_cast<llvm::ArrayType>(t) != nullptr) {

		if(arrayElementT == nullptr) {

			std::cout << "Error: Array Element Type not found.\n";
			exit(1);
		}

		auto arrType = dyn_cast<llvm::ArrayType>(t);

		std::vector<llvm::Constant*> a;

		for(int i = 0; i < arrType->getNumElements(); i++) {
			a.push_back(CodeGen::DefaultFromType(arrayElementT));
		}

		return llvm::ConstantArray::get(arrType, llvm::ArrayRef(a));
	}

	std::cout << "Error: Type not found.\n";
	exit(1);

	return nullptr;
}

void CodeGen::UpdateAllPHIPreds() {

	for(auto i : CodeGen::all_phi_nodes) {

		llvm::BasicBlock* parent = i.second->getParent();

		std::vector<llvm::BasicBlock*> blockPreds;

		for (auto it = llvm::pred_begin(parent), et = llvm::pred_end(parent); it != et; ++it)
		{
			llvm::BasicBlock* predecessor = *it;
			blockPreds.push_back(predecessor);
		}

		for(auto b : blockPreds) {
			auto V = AST::FindExistingState(i.first, b);

			if(!AST::IsInstructionInsideOfBlock(b, V)) {
				auto newV = AST::GetOrigin(i.first);

				if(!AST::IsInstructionInsideOfBlock(b, newV)) {
					V = Default(newV);
				}
				else {
					V = newV;
				}
			}

			if(V == nullptr) {
				std::cout << i.first << "'s state doesn't exist in block '" << std::string(b->getName()) << "'.\n";
				exit(1);
			}

			i.second->addIncoming(V, b);
		}
	}
}

int CodeGen::GetParentId(std::string name, llvm::BasicBlock* bb) {

	if(CodeGen::all_coms.find(name) != CodeGen::all_coms.end()) {

		int count = 0;

		for(auto i : CodeGen::all_coms[name]->blockParents) {

			if(i == bb) {
				return count;
			}

			count += 1;
		}
	}

	return -1;
}

void CodeGen::SetOriginBlock(std::string name) {

	if(CodeGen::all_coms.find(name) != CodeGen::all_coms.end()) {
		CodeGen::all_coms[name]->originBlock = CodeGen::Builder->GetInsertBlock();
	}
	else if(CodeGen::all_mems.find(name) != CodeGen::all_mems.end()) {
		CodeGen::all_mems[name]->originBlock = CodeGen::Builder->GetInsertBlock();
	}
}

void CodeGen::EndScope(llvm::BasicBlock* bb) {

	UNORDERED_MAP_FOREACH(std::string, std::unique_ptr<LLVM_Com>, CodeGen::all_coms, it) {

		if(it->second->originBlock == bb) {
			it->second->isOutOfScope = true;
		}
	}

	UNORDERED_MAP_FOREACH(std::string, std::unique_ptr<LLVM_Mem>, CodeGen::all_mems, it) {

		if(it->second->originBlock == bb) {
			it->second->isOutOfScope = true;
		}
	}
}