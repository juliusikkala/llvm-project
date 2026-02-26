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

class Value;
class IntrinsicInst;

/// Expand \p ParallelAlloca into a "privatized" alloca, where each vector lane
/// has its own private portion of the alloca. The return type of
/// `ParallelAlloca` defines the vectorization width.
LLVM_ABI Value* lowerParallelAlloca(IntrinsicInst *ParallelAlloca);

} // namespace llvm

#endif
