//===- llvm/Transforms/Utils/LowerParallelAlloca.h --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Lower parallel_alloca intrinsic to either a regular alloca or a "privatized"
// alloca.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_UTILS_LOWERPARALLELALLOCA_H
#define LLVM_TRANSFORMS_UTILS_LOWERPARALLELALLOCA_H

#include "llvm/Support/Compiler.h"
#include "llvm/Support/TypeSize.h"

namespace llvm {

class Instruction;
class IntrinsicInst;

/// Expand \p ParallelAlloca into a "privatized" alloca, where each vector lane
/// has its own private portion of the alloca. \p ParallelAlloca is not deleted
/// and its uses are not replaced, the caller is expected to perform that. The
/// resulting alloca is hoisted to the beginning of the function containing \p
/// ParallelAlloca. The return value is the vector of addresses to the generated
/// alloca.
LLVM_ABI Instruction* lowerParallelAlloca(
    IntrinsicInst *ParallelAlloca,
    ElementCount VectorWidth);

/// Expand \p ParallelAlloca as a regular alloca. This has to be done when the
/// parallel alloca is not in a parallel loop, i.e. if it is still present after
/// vectorization passes.
LLVM_ABI void lowerParallelAllocaToRegularAlloca(IntrinsicInst *ParallelAlloca);

} // namespace llvm

#endif
