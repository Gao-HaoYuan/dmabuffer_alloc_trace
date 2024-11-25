#pragma once

#include <stdint.h>
#include <fcntl.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

#include <bionic/macros.h>
#include <unwindstack/Unwinder.h>

#include "Config.h"

enum MemType{ HOST, MMAP, DMA };

struct FrameKeyType {
  size_t num_frames;
  uintptr_t* frames;

  bool operator==(const FrameKeyType& comp) const {
    if (num_frames != comp.num_frames) return false;
    for (size_t i = 0; i < num_frames; i++) {
      if (frames[i] != comp.frames[i]) {
        return false;
      }
    }
    return true;
  }
};

// 新增 hash 算法
namespace std {
template <> struct hash<FrameKeyType> {
  std::size_t operator()(const FrameKeyType& key) const {
    std::size_t cur_hash = key.frames[0];
    // Limit the number of frames to speed up hashing.
    size_t max_frames = (key.num_frames > 5) ? 5 : key.num_frames;
    for (size_t i = 1; i < max_frames; i++) {
      cur_hash ^= key.frames[i];
    }
    return cur_hash;
  }
};
};  // namespace std

struct FrameInfoType {
  size_t references = 0;
  std::vector<uintptr_t> frames;
};

// 新增 timeval 比较函数
inline bool operator<(const timeval& lhs, const timeval& rhs) {
    // Convert both times to microseconds and compare directly
    long long l_time = static_cast<long long>(lhs.tv_sec) * 1000000 + lhs.tv_usec;
    long long r_time = static_cast<long long>(rhs.tv_sec) * 1000000 + rhs.tv_usec;
    return l_time < r_time;
}

struct PointerInfoType {
  size_t size;
  size_t hash_index;
  MemType mem_type;
  timeval alloc_time;
  size_t RealSize() const { return size & ~(1U << 31); }
  static size_t MaxSize() { return (1U << 31) - 1; }
};

struct ListInfoType {
  uintptr_t pointer;
  size_t num_allocations;
  size_t size;
  MemType mem_type;
  FrameInfoType* frame_info;
  std::shared_ptr<std::vector<unwindstack::FrameData>> backtrace_info;
  timeval alloc_time;
};
using Pred = std::function<bool(const ListInfoType&, const ListInfoType&)>;

class PointerData {
public:
  PointerData() = default;
  virtual ~PointerData() = default;

  bool Initialize(const Config& config);

  bool ShouldBacktrace() { return backtrace_enabled_ == 1; }
  static void ToggleBacktraceEnabled(int /*signum*/) { backtrace_enabled_.fetch_xor(1); }

  size_t AddBacktrace(size_t num_frames, size_t size_bytes);
  void RemoveBacktrace(size_t hash_index);

  void Add(uintptr_t pointer, size_t size, MemType type);
  void AddHost(const void* ptr, size_t pointer_size, MemType type = HOST);
  void AddDMA(const uint32_t ptr, size_t pointer_size, MemType type = DMA);
  void Remove(uintptr_t pointer, bool is_dma);
  void RemoveHost(const void* ptr);
  void RemoveDMA(const uint32_t ptr);

  void DumpLiveToFile(int fd);
  void DumpPeakInfo();

private:
  inline uintptr_t ManglePointer(uintptr_t pointer) { return pointer ^ UINTPTR_MAX; }
  inline uintptr_t DemanglePointer(uintptr_t pointer) { return pointer ^ UINTPTR_MAX; }

  void GetList(std::vector<ListInfoType>* list, bool only_with_backtrace, Pred pred);
  void GetUniqueList(std::vector<ListInfoType>* list, bool only_with_backtrace);

  static std::atomic_uint8_t backtrace_enabled_;

  std::mutex pointer_mutex_;
  std::unordered_map<uintptr_t, PointerInfoType> pointers_;

  std::mutex frame_mutex_;
  std::unordered_map<FrameKeyType, size_t> key_to_index_;
  std::unordered_map<size_t, FrameInfoType> frames_;
  std::unordered_map<size_t, std::shared_ptr<std::vector<unwindstack::FrameData>>> backtraces_info_;
  size_t cur_hash_index_;

  size_t current_used, current_host, current_dma;
  size_t peak_tot, peak_host, peak_dma;
  std::vector<ListInfoType> peak_list;
  timeval peak_time;

  BIONIC_DISALLOW_COPY_AND_ASSIGN(PointerData);
};