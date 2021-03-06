/* -*- Mode: C++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#include "ReturnAddressList.h"

#include "Task.h"

namespace rr {

template <typename Arch>
static void return_addresses_x86ish(ReturnAddressList* result, Task* t) {
  // Immediately after a function call the return address is on the stack at
  // SP. After BP is pushed, but before it's initialized for the new stack
  // frame, the return address is on the stack at SP+wordsize. Just
  // capture those words now. We could inspect the code for known prologs/
  // epilogs but that misses cases such as calling into optimized code
  // or PLT stubs (which start with 'jmp'). Since it doesn't matter if we
  // capture addresses that aren't real return addresses, just capture those
  // words unconditionally.
  typename Arch::size_t frame[2];
  int next_address = 0;
  if (t->read_bytes_fallible(t->regs().sp(), sizeof(frame), frame) ==
      sizeof(frame)) {
    result->addresses[0] = frame[0];
    result->addresses[1] = frame[1];
    next_address = 2;
  }

  remote_ptr<void> bp = t->regs().bp();
  for (int i = next_address; i < ReturnAddressList::COUNT; ++i) {
    if (t->read_bytes_fallible(bp, sizeof(frame), frame) != sizeof(frame)) {
      break;
    }
    result->addresses[i] = frame[1];
    bp = frame[0];
  }
}

static void compute_return_addresses(ReturnAddressList* result, Task* t) {
  RR_ARCH_FUNCTION(return_addresses_x86ish, t->arch(), result, t);
}

ReturnAddressList::ReturnAddressList(Task* t) {
  compute_return_addresses(this, t);
}

} // namespace rr
