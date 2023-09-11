#include "AST.hpp"
#include <iostream>

int AST::slash_t_count = 0;

llvm::Function* AST::Program::codegen() {

	std::vector<llvm::Type*> llvmArgs;

	llvm::FunctionType* FT = llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(*CodeGen::TheContext), llvmArgs, false);

	llvm::Function* F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", CodeGen::TheModule.get());

	llvm::BasicBlock* BB = llvm::BasicBlock::Create(*CodeGen::TheContext, "entry", F);

	CodeGen::Builder->SetInsertPoint(BB);

	for(int i = 0; i < all_instructions.size(); i++) {

		all_instructions[i]->codegen();
	}

	// mustprogress nofree norecurse nosync nounwind readnone willreturn

	F->addFnAttr(llvm::Attribute::MustProgress);
	F->addFnAttr(llvm::Attribute::NoFree);
	F->addFnAttr(llvm::Attribute::NoRecurse);
	F->addFnAttr(llvm::Attribute::NoSync);
	F->addFnAttr(llvm::Attribute::NoUnwind);
	F->addFnAttr(llvm::Attribute::ReadNone);
	F->addFnAttr(llvm::Attribute::WillReturn);

	F->setCallingConv(llvm::CallingConv::GHC);

	return F;
}

llvm::Type* AST::Integer128::codegen() { return llvm::IntegerType::getInt128Ty(*CodeGen::TheContext); }
llvm::Type* AST::Integer64::codegen() { return llvm::IntegerType::getInt64Ty(*CodeGen::TheContext); }
llvm::Type* AST::Integer32::codegen() { return llvm::IntegerType::getInt32Ty(*CodeGen::TheContext); }
llvm::Type* AST::Integer16::codegen() { return llvm::IntegerType::getInt16Ty(*CodeGen::TheContext); }
llvm::Type* AST::Integer8::codegen() { return llvm::IntegerType::getInt8Ty(*CodeGen::TheContext); }
llvm::Type* AST::Integer1::codegen() { return llvm::IntegerType::getInt1Ty(*CodeGen::TheContext); }

llvm::Value* AST::ProcedureCall::codegen() {

	for(auto const& i: body) {
		i->codegen();
	}

	return return_obj->codegen();
}

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

	return llvm::ConstantInt::get(*CodeGen::TheContext, llvm::APInt(int_ty->getBitWidth(), num, true));
}

llvm::Value* AST::Variable::codegen() {

	for(auto const& i : initializers) {
		i->codegen();
	}

	llvm::Value* result = AST::GetCurrentInstructionByName(name);

	if(result == nullptr) {

		std::cout << "Unknown variable '" << name << "'\n";
		exit(1);
	}

	return result;
}

llvm::Value* AST::Com::codegen() {

	llvm::Value* tc = AST::GetOrCreateInstruction(target.get());

	std::unique_ptr<LLVM_Com> lcom = std::make_unique<LLVM_Com>();
	lcom->origin = tc;
	lcom->current = tc;

	CodeGen::all_coms[name] = std::move(lcom);

	return CodeGen::all_coms[name]->origin;
}

llvm::Value* AST::Mem::codegen() {

	llvm::Value* tc = AST::GetOrCreateInstruction(target.get());

	std::unique_ptr<LLVM_Mem> lmem = std::make_unique<LLVM_Mem>();

	llvm::Type* get_type = ty->codegen();

	llvm::Value* alloc_origin = CodeGen::Builder->CreateAlloca(get_type, 0, name);
	lmem->origin = alloc_origin;

	llvm::Value* store_target = CodeGen::Builder->CreateStore(tc, lmem->origin);
	lmem->current = alloc_origin;

	lmem->ty = get_type;

	CodeGen::all_mems[name] = std::move(lmem);

	return CodeGen::all_mems[name]->origin;
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

llvm::Value* AST::IntCast::codegen() {

	llvm::Value* targetC = AST::GetOrCreateInstruction(target.get());
	llvm::Type* typeC = intType->codegen();

	return CodeGen::Builder->CreateIntCast(targetC, typeC, true, target->name);
}

llvm::Value* AST::ComStore::codegen() {

	llvm::Value* result = AST::GetOrCreateInstruction(value.get());

	AST::AddInstruction(target.get(), result);

	return result;
}

llvm::Value* AST::GetAllocaFromMem(AST::Expression* e) {

	if(CodeGen::all_mems.find(e->name) != CodeGen::all_mems.end()) {
		return CodeGen::all_mems[e->name]->origin;
	}

	return nullptr;
}

llvm::Value* AST::MemStore::codegen() {

	llvm::Value* result = AST::GetOrCreateInstruction(value.get());

	llvm::Value* mem_alloca = AST::GetAllocaFromMem(target.get());

	if(mem_alloca == nullptr) {
		std::cout << "Error: Mem Origin not found for 'memstore'.\n";
		exit(1);
	}

	return CodeGen::Builder->CreateStore(result, mem_alloca);
}

llvm::Value* AST::LoadMem::codegen() {

	llvm::Value* mem_alloca = AST::GetAllocaFromMem(target.get());

	if(mem_alloca == nullptr) {
		std::cout << "Error: Mem Origin not found for 'loadmem'.\n";
		exit(1);
	}

	return CodeGen::Builder->CreateLoad(CodeGen::all_mems[target->name]->ty, mem_alloca, target->name);
}

llvm::Value* AST::Compare::codegen() {

	llvm::Value* L = AST::GetOrCreateInstruction(compareOne.get());
	llvm::Value* R = AST::GetOrCreateInstruction(compareTwo.get());

	llvm::Value* comp = nullptr;

	if(cmp_type == AST::CompareType::IsLessThan) { comp = CodeGen::Builder->CreateICmpUGT(R, L, "cmptmp"); }
	if(cmp_type == AST::CompareType::IsMoreThan) { comp = CodeGen::Builder->CreateICmpUGT(L, R, "cmptmp"); }
	if(cmp_type == AST::CompareType::IsEquals) { comp = CodeGen::Builder->CreateICmpEQ(L, R, "cmptmp"); }
	if(cmp_type == AST::CompareType::IsNotEquals) { comp = CodeGen::Builder->CreateICmpNE(L, R, "cmptmp"); }
	if(cmp_type == AST::CompareType::IsLessThanOrEquals) { comp = CodeGen::Builder->CreateICmpUGE(R, L, "cmptmp"); }
	if(cmp_type == AST::CompareType::IsMoreThanOrEquals) { comp = CodeGen::Builder->CreateICmpUGE(L, R, "cmptmp"); }

	return comp;
}

llvm::Value* AST::While::codegen() {

	llvm::Value* conditionCodegen = AST::GetOrCreateInstruction(condition.get());

	llvm::Function *TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

	llvm::BasicBlock* EntryBlock = CodeGen::Builder->GetInsertBlock();
	llvm::BasicBlock* LoopBlock = llvm::BasicBlock::Create(*CodeGen::TheContext, "while", TheFunction);
	llvm::BasicBlock* ContinueBlock = llvm::BasicBlock::Create(*CodeGen::TheContext, "continue");

	CodeGen::Builder->CreateCondBr(conditionCodegen, LoopBlock, ContinueBlock);

	CodeGen::Builder->SetInsertPoint(LoopBlock);

	std::unordered_map<std::string, llvm::PHINode*> allPHIs;

	for(auto const& i: loop_body) {

		AST::SaveState(i->name, EntryBlock);

		if(allPHIs.find(i->name) == allPHIs.end()) {

			CodeGen::Builder->SetInsertPoint(LoopBlock, LoopBlock->begin());

			auto entry = AST::FindExistingState(i->name, EntryBlock);

			allPHIs[i->name] = CodeGen::Builder->CreatePHI(entry->getType(), 2, "phi");

			allPHIs[i->name]->addIncoming(entry, LoopBlock);
			allPHIs[i->name]->addIncoming(entry, EntryBlock);

			CodeGen::Builder->SetInsertPoint(LoopBlock);

			AST::AddInstruction(i.get(), allPHIs[i->name]);
		}

		auto t = i->codegen();

		if(allPHIs.find(i->name) != allPHIs.end()) {
			allPHIs[i->name]->setIncomingValue(0, t);
		}
	}

	CodeGen::Builder->CreateCondBr(condition->codegen(), LoopBlock, ContinueBlock);

	TheFunction->getBasicBlockList().push_back(ContinueBlock);
	CodeGen::Builder->SetInsertPoint(ContinueBlock);

	UNORDERED_MAP_FOREACH(std::string, llvm::PHINode*, allPHIs, it) {

		auto firstVal = it->second->getIncomingValue(0);
		auto secondVal = it->second->getIncomingValue(1);

		auto finalPHI = CodeGen::Builder->CreatePHI(firstVal->getType(), 2, "phi");

		finalPHI->addIncoming(firstVal, LoopBlock);
		finalPHI->addIncoming(secondVal, EntryBlock);

		AST::AddInstructionToName(it->first, finalPHI);
	}

	for(auto const& i: loop_body) {
		AST::SaveState(i->name, ContinueBlock);
	}

	return nullptr;
}

llvm::Value* AST::If::codegen() {

	llvm::Value* conditionCodegen = AST::GetOrCreateInstruction(condition.get());

	llvm::Function *TheFunction = CodeGen::Builder->GetInsertBlock()->getParent();

	llvm::BasicBlock* EntryBlock = CodeGen::Builder->GetInsertBlock();

	llvm::BasicBlock* IfBlock = llvm::BasicBlock::Create(*CodeGen::TheContext, "if", TheFunction);
	llvm::BasicBlock* ElseBlock = nullptr;
	llvm::BasicBlock* ContinueBlock = llvm::BasicBlock::Create(*CodeGen::TheContext, "continue");

	if(else_body.size() == 0) {
		CodeGen::Builder->CreateCondBr(conditionCodegen, IfBlock, ContinueBlock);
	}
	else {
		ElseBlock = llvm::BasicBlock::Create(*CodeGen::TheContext, "else");
		CodeGen::Builder->CreateCondBr(conditionCodegen, IfBlock, ElseBlock);
	}

	CodeGen::Builder->SetInsertPoint(IfBlock);

	llvm::BasicBlock* finalEntryBlock = nullptr;

	if(parent_entry_block != nullptr) {
		finalEntryBlock = parent_entry_block;
	}
	else {
		finalEntryBlock = EntryBlock;
	}

	for(auto const& i: if_body) {

		AST::SaveState(i->name, EntryBlock);

		if(parent_entry_block != nullptr) {
			AST::SetExistingState(i->name, finalEntryBlock);
		}

		i->codegen();
	}

	for(auto const& i: if_body) {
		AST::SaveState(i->name, IfBlock);
	}

	CodeGen::Builder->CreateBr(ContinueBlock);

	if(else_body.size() != 0) {

		TheFunction->getBasicBlockList().push_back(ElseBlock);
		CodeGen::Builder->SetInsertPoint(ElseBlock);

		for(auto const& i: else_body) {

			AST::SetExistingState(i->name, finalEntryBlock);

			i->parent_entry_block = finalEntryBlock;

			i->codegen();
		}
	
		for(auto const& i: else_body) {
			AST::SaveState(i->name, ElseBlock);
		}
	
		CodeGen::Builder->CreateBr(ContinueBlock);
	}

	TheFunction->getBasicBlockList().push_back(ContinueBlock);
	CodeGen::Builder->SetInsertPoint(ContinueBlock);

	AST::CreateIfPHIs(ContinueBlock);

	for(auto const& i: if_body) {
		AST::SaveState(i->name, ContinueBlock);
	}

	return nullptr;
}

llvm::Value* AST::GetCurrentInstruction(AST::Expression* e) {

	return AST::GetCurrentInstructionByName(e->name);
}

llvm::Value* AST::GetCurrentInstructionByName(std::string name) {

	if(CodeGen::all_coms.find(name) != CodeGen::all_coms.end()) {
		return CodeGen::all_coms[name]->current;
	}

	if(CodeGen::all_mems.find(name) != CodeGen::all_mems.end()) {
		return CodeGen::all_mems[name]->current;
	}

	return nullptr;
}

llvm::Value* AST::GetOrCreateInstruction(AST::Expression* e) {

	llvm::Value* result = AST::GetCurrentInstruction(e);

	if(result == nullptr) {
		return e->codegen();
	}

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

			//std::cout << "Saved '" << name << "' state in '" << bbName << "'.\n";
		}
	}
}

void AST::SetExistingState(std::string name, llvm::BasicBlock* bb) {

	if(CodeGen::all_coms.find(name) != CodeGen::all_coms.end()) {
	
		std::string bbName = std::string(bb->getName());
		if(CodeGen::all_coms[name]->states.find(bbName) != CodeGen::all_coms[name]->states.end()) {

			CodeGen::all_coms[name]->current = CodeGen::all_coms[name]->states[bbName];
		}
	}
}

llvm::Value* AST::FindExistingState(std::string name, llvm::BasicBlock* bb) {

	if(CodeGen::all_coms.find(name) != CodeGen::all_coms.end()) {
	
		std::string bbName = std::string(bb->getName());
		if(CodeGen::all_coms[name]->states.find(bbName) != CodeGen::all_coms[name]->states.end()) {

			return CodeGen::all_coms[name]->states[bbName];
		}
	}

	return nullptr;
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

void AST::CreateIfElsePHIs(llvm::BasicBlock* continueBlock) {

	std::vector<llvm::BasicBlock*> blockPreds;

	for (auto it = llvm::pred_begin(continueBlock), et = llvm::pred_end(continueBlock); it != et; ++it)
	{
		llvm::BasicBlock* predecessor = *it;
		blockPreds.push_back(predecessor);
	}

	UNORDERED_MAP_FOREACH(std::string, std::unique_ptr<LLVM_Com>, CodeGen::all_coms, it) {

		llvm::Value* elseValue = nullptr;
		llvm::Value* ifValue = nullptr;

		llvm::BasicBlock* elseBlock = nullptr;
		llvm::BasicBlock* ifBlock = nullptr;
		
		std::string bbPreds0 = std::string(blockPreds[1]->getName());
		std::string bbPreds1 = std::string(blockPreds[0]->getName());

		std::string bbPreds2 = std::string(continueBlock->getName());

		//std::cout << "bbPreds0: " << bbPreds0 << "\n";
		//std::cout << "bbPreds1: " << bbPreds1 << "\n";

		if(it->second->states.find(bbPreds0) != it->second->states.end()) {

			ifValue = it->second->states[bbPreds0];
			ifBlock = blockPreds[1];
		}
		else {
			//std::cout << "'" << bbPreds0 << "' (bbPreds0) not found!\n";
		}

		if(it->second->states.find(bbPreds1) != it->second->states.end()) {

			elseValue = it->second->states[bbPreds1];
			elseBlock = blockPreds[0];
	
			auto phi = CodeGen::Builder->CreatePHI(ifValue->getType(), 2, "phi");
	
			phi->addIncoming(elseValue, elseBlock);
			phi->addIncoming(ifValue, ifBlock);

			it->second->current = phi;
		}
		else if(it->second->states.find(bbPreds2) != it->second->states.end()) {
			
			elseValue = it->second->states[bbPreds2];
			elseBlock = continueBlock;

			auto phi = CodeGen::Builder->CreatePHI(ifValue->getType(), 2, "phi");
	
			phi->addIncoming(elseValue, elseBlock);
			phi->addIncoming(ifValue, ifBlock);

			it->second->current = phi;
		}
		else {
			//std::cout << "'" << bbPreds1 << "' (bbPreds1) and '" << bbPreds2 << "' (bbPreds2) not found!\n";
		}
	}
}