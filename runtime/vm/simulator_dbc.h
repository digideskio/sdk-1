// Copyright (c) 2016, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef RUNTIME_VM_SIMULATOR_DBC_H_
#define RUNTIME_VM_SIMULATOR_DBC_H_

#ifndef RUNTIME_VM_SIMULATOR_H_
#error Do not include simulator_dbc.h directly; use simulator.h.
#endif

#include "vm/compiler/method_recognizer.h"
#include "vm/constants_dbc.h"

namespace dart {

class Isolate;
class RawObject;
class SimulatorSetjmpBuffer;
class Thread;
class Code;
class Array;
class RawICData;
class RawImmutableArray;
class RawArray;
class RawObjectPool;
class RawFunction;
class ObjectPointerVisitor;

// Simulator intrinsic handler. It is invoked on entry to the intrinsified
// function via Intrinsic bytecode before the frame is setup.
// If the handler returns true then Intrinsic bytecode works as a return
// instruction returning the value in result. Otherwise interpreter proceeds to
// execute the body of the function.
typedef bool (*IntrinsicHandler)(Thread* thread,
                                 RawObject** FP,
                                 RawObject** result);

class Simulator {
 public:
  static const uword kSimulatorStackUnderflowSize = 0x80;

  Simulator();
  ~Simulator();

  // The currently executing Simulator instance, which is associated to the
  // current isolate
  static Simulator* Current();

  // Low address (DBC stack grows up).
  uword stack_base() const { return stack_base_; }
  // Limit for StackOverflowError.
  uword overflow_stack_limit() const { return overflow_stack_limit_; }
  // High address (DBC stack grows up).
  uword stack_limit() const { return stack_limit_; }

  // Call on program start.
  static void Init();

  RawObject* Call(const Code& code,
                  const Array& arguments_descriptor,
                  const Array& arguments,
                  Thread* thread);

  void JumpToFrame(uword pc, uword sp, uword fp, Thread* thread);

  uword get_sp() const { return reinterpret_cast<uword>(fp_); }
  uword get_fp() const { return reinterpret_cast<uword>(fp_); }
  uword get_pc() const { return reinterpret_cast<uword>(pc_); }

  enum IntrinsicId {
#define V(test_class_name, test_function_name, enum_name, fp)                  \
  k##enum_name##Intrinsic,
    ALL_INTRINSICS_LIST(V) GRAPH_INTRINSICS_LIST(V)
#undef V
        kIntrinsicCount,
  };

  static bool IsSupportedIntrinsic(IntrinsicId id) {
    return intrinsics_[id] != NULL;
  }

  enum SpecialIndex {
    kExceptionSpecialIndex,
    kStackTraceSpecialIndex,
    kSpecialIndexCount
  };

  void VisitObjectPointers(ObjectPointerVisitor* visitor);

 private:
  uintptr_t* stack_;
  uword stack_base_;
  uword overflow_stack_limit_;
  uword stack_limit_;

  RawObject** fp_;
  uword pc_;
  DEBUG_ONLY(uint64_t icount_;)

  SimulatorSetjmpBuffer* last_setjmp_buffer_;

  RawObjectPool* pp_;  // Pool Pointer.
  RawArray* argdesc_;  // Arguments Descriptor: used to pass information between
                       // call instruction and the function entry.
  RawObject* special_[kSpecialIndexCount];

  static IntrinsicHandler intrinsics_[kIntrinsicCount];

  void Exit(Thread* thread,
            RawObject** base,
            RawObject** exit_frame,
            uint32_t* pc);

  void CallRuntime(Thread* thread,
                   RawObject** base,
                   RawObject** exit_frame,
                   uint32_t* pc,
                   intptr_t argc_tag,
                   RawObject** args,
                   RawObject** result,
                   uword target);

  void Invoke(Thread* thread,
              RawObject** call_base,
              RawObject** call_top,
              uint32_t** pc,
              RawObject*** FP,
              RawObject*** SP);

  bool Deoptimize(Thread* thread,
                  uint32_t** pc,
                  RawObject*** FP,
                  RawObject*** SP,
                  bool is_lazy);

  void InlineCacheMiss(int checked_args,
                       Thread* thread,
                       RawICData* icdata,
                       RawObject** call_base,
                       RawObject** top,
                       uint32_t* pc,
                       RawObject** FP,
                       RawObject** SP);

  void InstanceCall1(Thread* thread,
                     RawICData* icdata,
                     RawObject** call_base,
                     RawObject** call_top,
                     uint32_t** pc,
                     RawObject*** FP,
                     RawObject*** SP,
                     bool optimized);

  void InstanceCall2(Thread* thread,
                     RawICData* icdata,
                     RawObject** call_base,
                     RawObject** call_top,
                     uint32_t** pc,
                     RawObject*** FP,
                     RawObject*** SP,
                     bool optimized);

  void PrepareForTailCall(RawCode* code,
                          RawImmutableArray* args_desc,
                          RawObject** FP,
                          RawObject*** SP,
                          uint32_t** pc);

#if !defined(PRODUCT)
  // Returns true if tracing of executed instructions is enabled.
  bool IsTracingExecution() const;

  // Prints bytecode instruction at given pc for instruction tracing.
  void TraceInstruction(uint32_t* pc) const;
#endif  // !defined(PRODUCT)

  // Longjmp support for exceptions.
  SimulatorSetjmpBuffer* last_setjmp_buffer() { return last_setjmp_buffer_; }
  void set_last_setjmp_buffer(SimulatorSetjmpBuffer* buffer) {
    last_setjmp_buffer_ = buffer;
  }

  friend class SimulatorSetjmpBuffer;
  DISALLOW_COPY_AND_ASSIGN(Simulator);
};

}  // namespace dart

#endif  // RUNTIME_VM_SIMULATOR_DBC_H_
