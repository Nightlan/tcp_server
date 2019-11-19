/**
* epoll_module
* Copyright (c) 2017 engwei, yang (437798348@qq.com).
*
* @version 1.0
* @author engwei, yang
*/
/// @file Contains the byte array

#ifndef EPOLL_BYTE_ARRAY_H__
#define EPOLL_BYTE_ARRAY_H__

#include <common.h>

#include <vector>
#include <memory_allocator.h>

class ByteArray {
 public:
  ByteArray()
    : size_(0) {
    // nothing
  }
  ~ByteArray() {
    // nothing
  }

  void Write(const uint8_t* buffer, int size) {
    if (0 == size) return;
    allocator_.Ensure(size_ + size);
    memcpy(allocator_.memory() + size_, buffer, size);
    size_ += size;
  }

  void Write(uint8_t byte) {
    allocator_.Ensure(size_ + 1);
    allocator_.memory()[size_] = byte;
    size_ += 1;
  }

  uint8_t* Increment(int added) {
    allocator_.Ensure(size_ + added);
    int old_size = size_;
    size_ += added;
    return allocator_.memory() + old_size;
  }

  void Shrink(int size) {
    memmove(begin(), begin() + size, size_ - size);
    size_ -= size;
  }

  void Clear() {
    size_ = 0;
  }

  uint32_t CalculateSum() {
    uint32_t result = 0;
    uint8_t* buf = allocator_.memory();
    int length = size_;
    while (length > 8) {
      result += buf[0];
      result += buf[1];
      result += buf[2];
      result += buf[3];
      result += buf[4];
      result += buf[5];
      result += buf[6];
      result += buf[7];
      length -= 8;
      buf += 8;
    }
    for (int i = 0; i < length; ++i) {
      result += buf[i];
    }
    return result;
  }

  const uint8_t* begin() const {
    if (0 == size_) return nullptr;
    return allocator_.memory();
  }

  const uint8_t* end() const {
    if (0 == size_) return nullptr;
    return begin() + size_;
  }

  uint8_t* begin() {
    if (0 == size_) return nullptr;
    return allocator_.memory();
  }

  int size() const {
    return size_;
  }

 private:
  MemoryAllocator allocator_;
  // the data size
  int size_;

  // Disable copying of ByteArray
  DISALLOW_CONSTRUCTORS(ByteArray);
};

#endif // EPOLL_BYTE_ARRAY_H__