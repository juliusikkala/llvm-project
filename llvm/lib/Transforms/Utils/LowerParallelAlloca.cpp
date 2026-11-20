//===- LowerParallelAlloca.cpp ----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/LowerParallelAlloca.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

Value* llvm::lowerParallelAlloca(IntrinsicInst *ParallelAlloca)
{
  assert(ParallelAlloca->getIntrinsicID() == Intrinsic::parallel_alloca);

  auto *AnchorToken = ParallelAlloca->getArgOperand(0);
  auto *Anchor = dyn_cast<IntrinsicInst>(AnchorToken);
  assert(Anchor && Anchor->getIntrinsicID() == Intrinsic::parallel_alloca_anchor);

  auto *SizeParam = Anchor->getArgOperand(0);
  auto Alignment = cast<ConstantInt>(Anchor->getArgOperand(1))->getAlignValue();
  auto *ReturnType = ParallelAlloca->getType();
  auto *VecReturnType = dyn_cast<VectorType>(ReturnType);
  unsigned AddrSpace = ReturnType->getPointerAddressSpace();

  auto AlignmentValue = Alignment.value();

  IRBuilder<> B(ParallelAlloca);
  B.SetInsertPoint(Anchor);

  Value* ReturnValue = nullptr;
  if (VecReturnType) {
    // Vectorized, lowers to a privatized alloca
    ElementCount VectorWidth = VecReturnType->getElementCount();

    Value *AllocWidth = B.CreateElementCount(SizeParam->getType(), VectorWidth);
    auto IntWidth = cast<IntegerType>(SizeParam->getType())->getBitWidth();
    auto AlignmentMask = B.getIntN(IntWidth, AlignmentValue-1);
    auto *LaneStride = B.CreateBinOp(
      Instruction::BinaryOps::And,
      B.CreateBinOp(
        Instruction::BinaryOps::Add,
        SizeParam,
        AlignmentMask),
      B.CreateNot(AlignmentMask));

    Value *TotalAllocationSize = B.CreateBinOp(Instruction::BinaryOps::Mul, AllocWidth, LaneStride);
    auto *NewAlloca = B.Insert(new AllocaInst(B.getInt8Ty(), AddrSpace, TotalAllocationSize, Alignment));

    // Dig out per-lane addresses.
    auto *BaseAddressVector = B.CreateVectorSplat(VectorWidth, NewAlloca);

    Value *LaneOffsets = B.CreateBinOp(
      Instruction::BinaryOps::Mul,
      B.CreateStepVector(VectorType::get(B.getIntNTy(IntWidth), VectorWidth)),
      B.CreateVectorSplat(VectorWidth, LaneStride));

    ReturnValue = B.CreateInBoundsGEP(B.getInt8Ty(), BaseAddressVector, LaneOffsets);
  }
  else {
    // Non-vectorized, lowers to a normal alloca
    ReturnValue = B.Insert(new AllocaInst(B.getInt8Ty(), AddrSpace, SizeParam, Alignment));
  }

  ParallelAlloca->replaceAllUsesWith(ReturnValue);
  ParallelAlloca->eraseFromParent();
  
  // If this was the last use of the anchor, remove it.
  if (Anchor->getNumUses() == 0)
    Anchor->eraseFromParent();

  return ReturnValue;
}

void llvm::lowerParallelAllocaAnchor(IntrinsicInst *ParallelAllocaAnchor)
{
  assert(ParallelAllocaAnchor->getIntrinsicID() == Intrinsic::parallel_alloca_anchor);

  auto *SizeParam = ParallelAllocaAnchor->getArgOperand(0);
  auto Alignment = cast<ConstantInt>(ParallelAllocaAnchor->getArgOperand(1))->getAlignValue();

  // Find widest vector length for the allocation
  std::optional<ElementCount> MaxVectorWidth;
  std::optional<unsigned> AllocAddrSpace;
  for (Use &U : make_early_inc_range(ParallelAllocaAnchor->uses())) {
    // The only legal uses of the anchor are in parallel_alloca instructions.
    auto *II = dyn_cast<IntrinsicInst>(U.getUser());
    assert(II && II->getIntrinsicID() == Intrinsic::parallel_alloca);
    auto *ReturnType = II->getType();

    unsigned AddrSpace = ReturnType->getPointerAddressSpace();
    if (!AllocAddrSpace.has_value())
      AllocAddrSpace = AddrSpace;
    else 
    {
      // Can't refer to different address spaces in same alloca, so lower this
      // one separately.
      llvm::lowerParallelAlloca(II);
      continue;
    }

    auto *VecReturnType = dyn_cast<VectorType>(ReturnType);

    ElementCount count = ElementCount::getFixed(1);
    if (VecReturnType)
      count = VecReturnType->getElementCount();

    if (!MaxVectorWidth.has_value() || ElementCount::isKnownGT(count, MaxVectorWidth.value()))
      MaxVectorWidth = count;
  }

  if (!MaxVectorWidth.has_value() || !AllocAddrSpace.has_value()) {
    ParallelAllocaAnchor->eraseFromParent();
    return;
  }

  ElementCount VectorWidth = MaxVectorWidth.value();

  IRBuilder<> B(ParallelAllocaAnchor);

  auto IntWidth = cast<IntegerType>(SizeParam->getType())->getBitWidth();
  auto AlignmentMask = B.getIntN(IntWidth, Alignment.value()-1);
  auto *LaneStride = B.CreateBinOp(
    Instruction::BinaryOps::And,
    B.CreateBinOp(
      Instruction::BinaryOps::Add,
      SizeParam,
      AlignmentMask),
    B.CreateNot(AlignmentMask));

  AllocaInst* NewAlloca;
  if (VectorWidth.isFixed() && VectorWidth.getFixedValue() == 1) {
    NewAlloca = B.Insert(new AllocaInst(B.getInt8Ty(), AllocAddrSpace.value(), SizeParam, Alignment));
  }
  else {
    Value *AllocWidth = B.CreateElementCount(SizeParam->getType(), VectorWidth);
    Value *TotalAllocationSize = B.CreateBinOp(Instruction::BinaryOps::Mul, AllocWidth, LaneStride);
    NewAlloca = B.Insert(new AllocaInst(B.getInt8Ty(), AllocAddrSpace.value(), TotalAllocationSize, Alignment));
  }

  for (Use &U : make_early_inc_range(ParallelAllocaAnchor->uses())) {
    auto *II = dyn_cast<IntrinsicInst>(U.getUser());
    auto *ReturnType = II->getType();
    auto *VecReturnType = dyn_cast<VectorType>(ReturnType);

    if (VecReturnType) {
      B.SetInsertPoint(II);
      auto *BaseAddressVector = B.CreateVectorSplat(VectorWidth, NewAlloca);

      Value *LaneOffsets = B.CreateBinOp(
        Instruction::BinaryOps::Mul,
        B.CreateStepVector(VectorType::get(B.getIntNTy(IntWidth), VectorWidth)),
        B.CreateVectorSplat(VectorWidth, LaneStride));

      auto *ReturnValue = B.CreateInBoundsGEP(B.getInt8Ty(), BaseAddressVector, LaneOffsets);
      II->replaceAllUsesWith(ReturnValue);
    }
    else {
      II->replaceAllUsesWith(NewAlloca);
    }
    II->eraseFromParent();
  }

  ParallelAllocaAnchor->eraseFromParent();
}
