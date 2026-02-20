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

Value* llvm::lowerParallelAlloca(
    IntrinsicInst *ParallelAlloca,
    ElementCount VectorWidth)
{
  assert(ParallelAlloca->getIntrinsicID() == Intrinsic::parallel_alloca);

  auto *SizeParam = ParallelAlloca->getArgOperand(0);
  auto Alignment = cast<ConstantInt>(ParallelAlloca->getArgOperand(1))->getAlignValue();
  unsigned AddrSpace = 0; // TODO

  auto AlignmentValue = Alignment.value();

  IRBuilder<> B(ParallelAlloca);
  Function *ParentFunc = ParallelAlloca->getParent()->getParent();
  B.SetInsertPointPastAllocas(ParentFunc);

  Value *AllocWidth = B.CreateElementCount(SizeParam->getType(), VectorWidth);
  auto IntWidth = cast<IntegerType>(SizeParam->getType())->getBitWidth();
  auto *LaneStride = B.CreateBinOp(
    Instruction::BinaryOps::And,
    B.CreateBinOp(
      Instruction::BinaryOps::Add,
      SizeParam,
      B.getIntN(IntWidth, AlignmentValue-1)),
    B.getIntN(IntWidth, ~(AlignmentValue-1)));

  Value *TotalAllocationSize = B.CreateBinOp(Instruction::BinaryOps::Mul, AllocWidth, LaneStride);
  auto *NewAlloca = B.Insert(new AllocaInst(B.getInt8Ty(), AddrSpace, TotalAllocationSize, Alignment));

  // Dig out per-lane addresses.
  auto *BaseAddressVector = B.CreateVectorSplat(VectorWidth, NewAlloca);

  Value *LaneOffsets = B.CreateBinOp(
    Instruction::BinaryOps::Mul,
    B.CreateStepVector(B.getIntNTy(IntWidth)),
    B.CreateVectorSplat(VectorWidth, LaneStride));

  return B.CreateBinOp(Instruction::BinaryOps::Add, BaseAddressVector, LaneOffsets);
}

void llvm::lowerParallelAllocaToRegularAlloca(IntrinsicInst *ParallelAlloca) {
  assert(ParallelAlloca->getIntrinsicID() == Intrinsic::parallel_alloca);

  auto *AllocationSize = ParallelAlloca->getArgOperand(0);
  auto Alignment = cast<ConstantInt>(ParallelAlloca->getArgOperand(1))->getAlignValue();

  IRBuilder<> B(ParallelAlloca);
  Function *ParentFunc = ParallelAlloca->getParent()->getParent();
  B.SetInsertPointPastAllocas(ParentFunc);

  auto *NewAlloca = B.Insert(new AllocaInst(B.getInt8Ty(), 0, AllocationSize, Alignment));
  ParallelAlloca->replaceAllUsesWith(NewAlloca);
  ParallelAlloca->eraseFromParent();
}
