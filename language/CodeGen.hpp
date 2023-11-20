#ifndef CODEGEN_HPP
#define CODEGEN_HPP

// RELEASE THE KRAKEN.

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Transforms/Vectorize.h"

#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/RegionPrinter.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/CodeGen/Passes.h"

#include "llvm/Transforms/Utils.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/IR/CFG.h"
#include <unordered_map>

struct LLVM_Com {

	llvm::Value* origin;
	llvm::Value* current;

	std::unordered_map<std::string, llvm::Value*> states;

	llvm::BasicBlock* originBlock;

	bool isOutOfScope = false;

	std::vector<llvm::BasicBlock*> blockParents;
};

struct LLVM_Mem {

	llvm::Value* origin;
	llvm::Value* current;

	llvm::Type* ty;

	llvm::BasicBlock* originBlock;

	bool isOutOfScope = false;

	std::unordered_map<std::string, llvm::Value*> states;
};

struct CodeGen {

	static bool releaseMode;

	static std::unordered_map<std::string, std::unique_ptr<LLVM_Com>> all_coms;
	static std::unordered_map<std::string, std::unique_ptr<LLVM_Mem>> all_mems;

	static std::vector<std::pair<std::string, llvm::PHINode*>> all_phi_nodes;

	static std::vector<llvm::BasicBlock*> pureBlocks;

	static void AddPHINodeToVec(std::string name, llvm::PHINode* p);
	static void UpdateAllPHIPreds();

	static int GetParentId(std::string name, llvm::BasicBlock* bb);

	static void SetOriginBlock(std::string name);

	static void EndScope(llvm::BasicBlock* bb);

	static std::unique_ptr<llvm::LLVMContext> TheContext;
	static std::unique_ptr<llvm::IRBuilder<>> Builder;
	static std::unique_ptr<llvm::Module> TheModule;

	static void Initialize();

	static llvm::Value* Default(llvm::Value* v);
	static llvm::Constant* DefaultFromType(llvm::Type* t, llvm::Type* arrayElementT = nullptr);
};

#endif