; RUN: opt -S -passes=inline < %s | FileCheck %s
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: norecurse nounwind uwtable
define void @Body(ptr nocapture %A, i32 %i) #0 {
entry:
  %A.addr = alloca ptr, align 8
  %i.addr = alloca i32, align 4
  %t = alloca [2 x i32], align 4
  store ptr %A, ptr %A.addr, align 8
  store i32 %i, ptr %i.addr, align 4
  %0 = load i32, ptr %i.addr, align 4
  %sub = sub nsw i32 %0, 1
  %arrayidx = getelementptr inbounds [2 x i32], ptr %t, i64 0, i64 0
  store i32 %sub, ptr %arrayidx, align 4
  %1 = load i32, ptr %i.addr, align 4
  %add = add nsw i32 %1, 1
  %arrayidx1 = getelementptr inbounds [2 x i32], ptr %t, i64 0, i64 1
  store i32 %add, ptr %arrayidx1, align 4
  %2 = load i32, ptr %i.addr, align 4
  %and = and i32 %2, 1
  %idxprom = sext i32 %and to i64
  %arrayidx2 = getelementptr inbounds [2 x i32], ptr %t, i64 0, i64 %idxprom
  %3 = load i32, ptr %arrayidx2, align 4
  %4 = load ptr, ptr %A.addr, align 8
  %5 = load i32, ptr %i.addr, align 4
  %idxprom3 = sext i32 %5 to i64
  %arrayidx4 = getelementptr inbounds i32, ptr %4, i64 %idxprom3
  store i32 %3, ptr %arrayidx4, align 4
  ret void
}

; Function Attrs: norecurse nounwind uwtable
define void @Test(ptr %res, i32 %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %cmp = icmp slt i32 %i.0, 16
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  call void @Body(ptr %res, i32 %i.0), !llvm.access.group !0
  %inc = add nsw i32 %i.0, 1
  br label %for.cond, !llvm.loop !1

for.end:                                          ; preds = %for.cond
  ret void
}

; CHECK-LABEL: @Test
; CHECK: for.body:
; CHECK: call ptr @llvm.parallel_alloca
; CHECK: call ptr @llvm.parallel_alloca
; CHECK: call ptr @llvm.parallel_alloca

attributes #0 = { norecurse nounwind uwtable }

!0 = distinct !{}
!1 = distinct !{!0, !{!"llvm.loop.parallel_accesses", !0}}
