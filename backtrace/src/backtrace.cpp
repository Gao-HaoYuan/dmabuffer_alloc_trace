/*
 * Copyright (C) 2012 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <cxxabi.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <unwind.h>
#include <cstdio>

#include "MapData.h"
#include "backtrace.h"

#include "android-base/stringprintf.h"

#if defined(__LP64__)
#define PAD_PTR "016" PRIxPTR
#else
#define PAD_PTR "08" PRIxPTR
#endif

typedef struct _Unwind_Context __unwind_context;

static MapData g_map_data;
static const MapEntry* g_current_code_map = nullptr;

static _Unwind_Reason_Code find_current_map(__unwind_context* context, void*) {
  uintptr_t ip = _Unwind_GetIP(context);

  if (ip == 0) {
    return _URC_END_OF_STACK;
  }
  g_current_code_map = g_map_data.find(ip);
  return _URC_END_OF_STACK;
}

void backtrace_startup() {
  _Unwind_Backtrace(find_current_map, nullptr);
}

void backtrace_shutdown() {}

struct stack_crawl_state_t {
  uintptr_t* frames;
  size_t frame_count;
  size_t cur_frame = 0;

  stack_crawl_state_t(uintptr_t* frames, size_t frame_count)
      : frames(frames), frame_count(frame_count) {}
};

static _Unwind_Reason_Code trace_function(__unwind_context* context, void* arg) {
  stack_crawl_state_t* state = static_cast<stack_crawl_state_t*>(arg);

  uintptr_t ip = _Unwind_GetIP(context);

  // `ip` is the address of the instruction *after* the call site in
  // `context`, so we want to back up by one instruction. This is hard for
  // every architecture except arm64, so we just make sure we're *inside*
  // that instruction, not necessarily at the start of it. (If the value
  // is too low to be valid, we just leave it alone.)
  if (ip >= 4096) {
#if defined(__aarch64__)
    ip -= 4;  // Exactly.
#elif defined(__arm__) || defined(__riscv)
    ip -= 2;  // At least.
#elif defined(__i386__) || defined(__x86_64__)
    ip -= 1;  // At least.
#endif
  }

  // Do not record the frames that fall in our own shared library.
  if (g_current_code_map && (ip >= g_current_code_map->start) && ip < g_current_code_map->end) {
    return _URC_NO_REASON;
  }

  state->frames[state->cur_frame++] = ip;
  return (state->cur_frame >= state->frame_count) ? _URC_END_OF_STACK : _URC_NO_REASON;
}

size_t backtrace_get(uintptr_t* frames, size_t frame_count) {
  stack_crawl_state_t state(frames, frame_count);
  _Unwind_Backtrace(trace_function, &state);
  return state.cur_frame;
}

void dump_backtrace(int fd, const uintptr_t* frames, size_t frame_count) {
  for (size_t frame_num = 0; frame_num < frame_count; frame_num++) {
    uintptr_t offset = 0;
    const char* symbol = nullptr;

    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(frames[frame_num]), &info) != 0) {
      offset = reinterpret_cast<uintptr_t>(info.dli_saddr);
      symbol = info.dli_sname;
    } else {
      info.dli_fname = nullptr;
    }

    uintptr_t rel_pc = offset;
    const MapEntry* entry = g_map_data.find(frames[frame_num], &rel_pc);
    std::string str = android::base::StringPrintf("#%0zd %" PRIx64 " ", frame_num, rel_pc);

    const char* soname = (entry != nullptr) ? entry->name.c_str() : info.dli_fname;
    if (soname == nullptr) {
      soname = "<unknown>";
    }
    str += soname;

    char offset_buf[128];
    if (entry != nullptr && entry->elf_start_offset != 0) {
      snprintf(offset_buf, sizeof(offset_buf), " (offset 0x%" PRIxPTR ")", entry->elf_start_offset);
    } else {
      offset_buf[0] = '\0';
    }
    str += offset_buf;

    if (symbol != nullptr) {
      char* demangled_name = abi::__cxa_demangle(symbol, nullptr, nullptr, nullptr);
      const char* name;
      if (demangled_name != nullptr) {
        name = demangled_name;
      } else {
        name = symbol;
      }
      str += " (";
      str += name;
      if (entry != nullptr && entry->elf_start_offset != 0) {
        str += "+" + std::to_string(frames[frame_num] - offset);
      }
      str += ")";
      free(demangled_name);
    } 

    dprintf(fd, "%s\n", str.c_str());
  }
}