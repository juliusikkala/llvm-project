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

/*
Instruction* llvm::lowerParallelAlloca(
    IntrinsicInst *ParallelAlloca,
    ElementCount VectorWidth)
{
  assert(ParallelAlloca->getIntrinsicID() == Intrinsic::parallel_alloca);

  auto *LaneAllocationSize = ParallelAlloca->getArgOperand(0);
  auto Alignment = cast<ConstantInt>(ParallelAlloca->getArgOperand(1))->getAlignValue();

  IRBuilder<> B(ParallelAlloca);
  Function *ParentFunc = ParallelAlloca->getParent()->getParent();
  B.SetInsertPointPastAllocas(ParentFunc);

  Value* TotalAllocationSize = nullptr;
  if ()
  {
  }

  auto *NewAlloca = B.Insert(new AllocaInst(B.getInt8Ty(), 0, AllocationSize, Alignment));
}
*/

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
