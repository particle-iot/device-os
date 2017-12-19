/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SYSTEM_TRACER_SERVICE_IMPL_H
#define SYSTEM_TRACER_SERVICE_IMPL_H

#ifndef TRACER_LOCATION_EXTERNAL_DEFINES
#include "platform_tracer.h"
#endif // TRACER_LOCATION_EXTERNAL_DEFINES

#include <inttypes.h>
#include <type_traits>
#include <cstdlib>
#include "service_debug.h"
#include "spark_wiring_json.h"

namespace particle { namespace tracer {

/**
 * Default constraints on maximum size
 */
constexpr size_t maxThreadNameLength = 10;
constexpr size_t maxStacktraceSize = 32;
constexpr size_t maxTextualCheckpointLength = 63;

struct __attribute__((packed)) ThreadEntry {
  /**
   * Total size of the entry including the structure size
   */
  uint16_t size : 15;

  /**
   * A marker
   */
  uint16_t mark : 1;

  /**
   * Unique thread id
   */
  uintptr_t id;

  /**
   * Textual type checkpoint present flag
   */
  uint16_t textual_checkpoint : 1;

  /**
   * Address type checkpoint present flag
   */
  uint16_t address_checkpoint : 1;

  /**
   * A number of uintptr_t entries in the stacktrace
   */
  uint16_t count : 7;

  /**
   * Thread name length
   */
  uint16_t name_length : 7;


  bool initialized() const {
    return (size != 0);
  }

  char* name() const {
    if (initialized() && name_length > 0) {
      return ((char*)this + sizeof(*this));
    }

    return nullptr;
  }

  void setName(const char* name) {
    uint8_t* ptr = (uint8_t*)this + sizeof(*this);
    const size_t len = strnlen(name, maxThreadNameLength);
    memcpy(ptr, name, len);
    ptr += len;
    *ptr = '\0';
    name_length = len + 1;
    updateSize();
  }

  uint8_t* checkpoint() const {
    if (initialized()) {
      return ((uint8_t*)this + sizeof(*this) + name_length);
    }
    return nullptr;
  }

  const char* checkpointText() const {
    uint8_t* ptr = checkpoint();
    if (ptr && textual_checkpoint) {
      if (address_checkpoint) {
        ptr += sizeof(uintptr_t);
      }
      return (const char*)ptr;
    }
    return nullptr;
  }

  uintptr_t* checkpointAddress() const {
    uint8_t* ptr = checkpoint();
    if (ptr && address_checkpoint) {
      return (uintptr_t*)ptr;
    }
    return nullptr;
  }

  size_t checkpointSize() const {
    size_t sz = 0;
    uint8_t* ptr = checkpoint();
    if (initialized() && ptr != nullptr) {
      if (address_checkpoint) {
        sz += sizeof(uintptr_t);
      }
      if (textual_checkpoint) {
        sz += strlen((const char*)ptr + sizeof(uintptr_t)) + 1;
      }
    }

    return sz;
  }

  uint8_t* stacktrace() const {
    size_t chksz = checkpointSize();
    uint8_t* ptr = checkpoint();
    if (ptr != nullptr) {
      return (ptr + chksz);
    }

    return nullptr;
  }

  size_t stacktraceSize() const {
    size_t sz = 0;
    if (initialized()) {
      sz = count * sizeof(uintptr_t);
    }

    return sz;
  }

  size_t calculateSize() const {
    return sizeof(*this) + name_length + checkpointSize() + stacktraceSize();
  }

  void updateSize() {
    size = calculateSize();
  }

  void updateCheckpoint(tracer_checkpoint_t* chkpt) {
    if (chkpt != nullptr) {
      address_checkpoint = 1;
      textual_checkpoint = chkpt->type == CHECKPOINT_TYPE_TEXTUAL;
      uint8_t* ptr = checkpoint();
      *((uintptr_t*)ptr) = (uintptr_t)chkpt->instruction;
      ptr += sizeof(uintptr_t);
      if (textual_checkpoint) {
        const size_t tLen = strnlen(chkpt->text, maxTextualCheckpointLength);
        memcpy(ptr, chkpt->text, tLen);
        ptr += tLen;
        *ptr = '\0';
      }
    } else {
      address_checkpoint = textual_checkpoint = 0;
    }
    updateSize();
  }

  void reset() {
    memset(this, 0, sizeof(*this));
  }

};

struct ThreadEntryIterator {
  ThreadEntryIterator(ThreadEntry* th, const uint8_t* location, size_t sz)
      : loc{location},
        locsz{sz},
        val{th} {
  }

  ThreadEntryIterator(const ThreadEntryIterator& other) = default;

  ThreadEntry* value() {
    if (valid(val)) {
      return val;
    }

    return nullptr;
  }

  bool valid(ThreadEntry* th) const {
    if (th && (uint8_t*)th >= ((uint8_t*)loc + sizeof(uint32_t)) &&
        (uint8_t*)th < ((uint8_t*)(loc + locsz) - sizeof(ThreadEntry)) &&
        th->initialized()) {
      return true;
    }

    return false;
  }

  ThreadEntryIterator next() const {
    if (!valid(val)) {
      return ThreadEntryIterator(nullptr, loc, locsz);
    }

    uint8_t* ptr = ((uint8_t*)val) + val->size;
    if (valid(reinterpret_cast<ThreadEntry*>(ptr))) {
      return ThreadEntryIterator(reinterpret_cast<ThreadEntry*>(ptr), loc, locsz);
    }

    return ThreadEntryIterator(nullptr, loc, locsz);
  }

  const uint8_t* loc;
  size_t locsz;
  ThreadEntry* val;
};

static_assert(std::is_pod<ThreadEntry>::value, "ThreadEntry should be a POD");

class TracerService {
public:
  static TracerService* instance();

  bool isValid() const;
  tracer_error_t insertCheckpoint(os_thread_t thread, tracer_checkpoint_t* chkpt);
  tracer_error_t insertCheckpoint(os_thread_dump_info_t* info, tracer_checkpoint_t* chkpt, bool stacktrace, bool keep = true);
  tracer_error_t insertCheckpoint(os_unique_id_t thread, tracer_checkpoint_t* chkpt);
  bool unmarkAll();
  bool removeUnmarked();
  bool cleanStacktraces();

  static uintptr_t lock(bool irq = false);
  static void unlock(bool irq = false, uintptr_t st = 0);

  size_t dumpCurrent(char* out, size_t sz) const;
  size_t dumpSaved(char* out, size_t sz) const;
  void updateCrc();

  void freeze(bool val);
  bool frozen() const;

protected:
  TracerService();
  TracerService(uint8_t* location, size_t sz);

  static size_t size(tracer_checkpoint_t* chkpt);

  void initialize();
  bool valid() const;
  uint32_t computeCrc() const;

  size_t dump(uint8_t* location, size_t dsize, char* out, size_t sz) const;
  bool allocate(ThreadEntry* th, uint8_t* ptr, size_t available, size_t needed);

  bool setThreadCheckpoint(ThreadEntry* h, tracer_checkpoint_t* chkpt);

  ThreadEntry* first() const;
  ThreadEntry* last(bool initialized = false) const;
  ThreadEntry* next(ThreadEntry* t) const;
  ThreadEntry* findThread(os_unique_id_t th) const;

  bool isThreadEntry(uint8_t* ptr) const;

  size_t freeSpace() const;

  uint8_t* location_ = nullptr;
  const size_t size_ = 0;
  uint8_t* savedData_;
  bool savedAvailable_ = false;
  bool frozen_ = false;
};

inline TracerService::TracerService() : TracerService((uint8_t*)TRACER_LOCATION_BEGIN, (TRACER_LOCATION_END - TRACER_LOCATION_BEGIN)) {
}

inline TracerService::TracerService(uint8_t* location, size_t sz)
    : location_{location},
      size_{sz} {
  uintptr_t st = lock(true);
  if (valid()) {
    savedAvailable_ = true;
    savedData_ = (uint8_t*)malloc(size_);
    SPARK_ASSERT(savedData_ != nullptr);
    memcpy(savedData_, location_, size_);
  }
  initialize();
  unlock(true, st);
}

inline TracerService* TracerService::instance() {
  static TracerService service;
  return &service;
}

inline bool TracerService::isValid() const {
  uintptr_t st = lock(true);
  bool v = valid();
  unlock(true, st);

  return v;
}

inline tracer_error_t TracerService::insertCheckpoint(os_thread_t thread, tracer_checkpoint_t* chkpt) {
  if (frozen_) {
    return TRACER_ERROR_FROZEN;
  }
  os_unique_id_t id = os_thread_unique_id(thread);
  return insertCheckpoint(id, chkpt);
}

inline tracer_error_t TracerService::insertCheckpoint(os_unique_id_t thread, tracer_checkpoint_t* chkpt) {
  if (frozen_) {
    return TRACER_ERROR_FROZEN;
  }
  ThreadEntry* h = findThread(thread);
  if (h == nullptr) {
      return TRACER_ERROR_NO_THREAD_ENTRY;
  }

  bool result = setThreadCheckpoint(h, chkpt);

  return result ? TRACER_ERROR_NONE : TRACER_ERROR;
}

inline tracer_error_t TracerService::insertCheckpoint(os_thread_dump_info_t* info, tracer_checkpoint_t* chkpt, bool stacktrace, bool keep) {
  if (frozen_) {
    return TRACER_ERROR_FROZEN;
  }
  ThreadEntry* th = findThread(info->id);
  bool res = false;

  if (th != nullptr) {
    // Update existing
    if (chkpt != nullptr || !keep) {
      res = setThreadCheckpoint(th, chkpt);
    } else {
      res = true;
    }
  } else {
    // Insert new at the end
    if (freeSpace() < (sizeof(ThreadEntry) + size(chkpt) + strnlen(info->name, maxThreadNameLength) + 1)) {
      return TRACER_ERROR_NO_SPACE;
    }

    th = last();

    if (th == nullptr) {
      return TRACER_ERROR;
    }

    th->id = info->id;
    th->setName(info->name);
    res = setThreadCheckpoint(th, chkpt);
  }

  if (stacktrace) {
    // Dry run to get a number of entries
    int count = HAL_Core_Generate_Stacktrace(info,
      [](void* ptr, int idx, uintptr_t addr, os_thread_dump_info_t* info, void* reserved) -> int {
        return 0;
      },
      (void*)th, nullptr);

    th->mark = 1;

    size_t available = th->stacktraceSize();
    if (allocate(th, th->stacktrace(), available, sizeof(uintptr_t) * count)) {
      th->count = std::min(count, (int)maxStacktraceSize);
      HAL_Core_Generate_Stacktrace(info,
        [](void* ptr, int idx, uintptr_t addr, os_thread_dump_info_t* info, void* reserved) -> int {
          ThreadEntry* th = (ThreadEntry*)ptr;
          if (idx < th->count) {
            ((uintptr_t*)th->stacktrace())[idx] = addr;
            return 0;
          }
          return 1;
        },
        (void*)th, nullptr);

      res = true;
    } else {
      allocate(th, th->stacktrace(), available, 0);
      th->count = 0;
    }
  }

  th->updateSize();

  return res ? TRACER_ERROR_NONE : TRACER_ERROR;
}

inline bool TracerService::unmarkAll() {
  if (frozen_) {
    return false;
  }
  for(ThreadEntry* h = first(); h != nullptr && h->initialized(); h = next(h)) {
      h->mark = 0;
  }

  return true;
}

inline bool TracerService::removeUnmarked() {
  if (frozen_) {
    return false;
  }
  bool modified = false;
  for(ThreadEntry* h = first(); h != nullptr && h->initialized();) {
    if (h->mark == 0) {
      const size_t sz = h->size;
      memset(h, 0, sz);
      allocate(h, (uint8_t*)h, sz, 0);
      modified = true;
    } else {
      h = next(h);
    }
  }

  if (modified) {
    updateCrc();
  }

  return true;
}

inline bool TracerService::cleanStacktraces() {
  if (frozen_) {
    return false;
  }
  bool modified = false;
  for(ThreadEntry* h = first(); h != nullptr && h->initialized(); h = next(h)) {
    if (h->stacktraceSize() > 0) {
      allocate(h, h->stacktrace(), h->stacktraceSize(), 0);
      h->count = 0;
      h->updateSize();
      modified = true;
    }
  }

  if (modified) {
    updateCrc();
  }

  return true;
}

inline uintptr_t TracerService::lock(bool irq) {
  if (!HAL_IsISR() && !irq) {
    os_thread_scheduling(false, nullptr);
  } else {
    return static_cast<uintptr_t>(HAL_disable_irq());
  }

  return 0;
}

inline void TracerService::unlock(bool irq, uintptr_t st) {
  if (!HAL_IsISR() && !irq) {
    os_thread_scheduling(true, nullptr);
  } else {
    HAL_enable_irq(st);
  }
}

inline size_t TracerService::dumpCurrent(char* out, size_t sz) const {
  return dump(location_, size_, out, sz);
}

inline size_t TracerService::dumpSaved(char* out, size_t sz) const {
  if (savedAvailable_ == true) {
    return dump((uint8_t*)savedData_, size_, out, sz);
  }
  return 0;
}

inline void TracerService::updateCrc() {
  if (frozen_) {
    return;
  }
  uint32_t* crc = reinterpret_cast<uint32_t*>(location_);
  *crc = computeCrc();
}

inline size_t TracerService::size(tracer_checkpoint_t* chkpt) {
  size_t sz = 0;

  if (chkpt) {
    sz = chkpt->type == CHECKPOINT_TYPE_INSTRUCTION_ADDRESS ?
         sizeof(uintptr_t) :
         sizeof(uintptr_t) + strnlen(chkpt->text, maxTextualCheckpointLength) + 1;
  }

  return sz;
}

inline void TracerService::freeze(bool val) {
  frozen_ = val;
}

inline bool TracerService::frozen() const {
  return frozen_;
}

inline void TracerService::initialize() {
  memset(location_, 0, size_);
  updateCrc();
}

inline bool TracerService::valid() const {
  uint32_t crc = *reinterpret_cast<uint32_t*>(location_);
  return (crc == computeCrc());
}

inline uint32_t TracerService::computeCrc() const {
  return HAL_Core_Compute_CRC32((const uint8_t*)(location_ + sizeof(uint32_t)),
                                size_ - sizeof(uint32_t));
}

inline size_t TracerService::dump(uint8_t* location, size_t dsize, char* out, size_t sz) const {
  char tmp[sizeof(uintptr_t) * 2 + 4] = {0};
  spark::JSONBufferWriter writer(out, sz - 1);

  writer.beginArray();
  for (auto it = ThreadEntryIterator(reinterpret_cast<ThreadEntry*>(location + sizeof(uint32_t)),
                                     location, dsize);
       it.value() != nullptr;
       it = it.next()) {
    auto th = it.value();
    writer.beginObject();
    writer.name("t");
    writer.value(th->name());
    writer.name("i");
    writer.value(th->id);
    if (th->textual_checkpoint || th->address_checkpoint) {
      writer.name("c");
      writer.beginObject();
      if (th->address_checkpoint) {
        writer.name("a");
        snprintf(tmp, sizeof(tmp), "0x%" PRIxPTR, *th->checkpointAddress());
        writer.value(tmp);
      }
      if (th->textual_checkpoint) {
        writer.name("x");
        writer.value(th->checkpointText());
      }
      writer.endObject();
    }
    if (th->count > 0) {
      writer.name("s");
      writer.beginArray();
      for (unsigned i = 0; i < th->count; i++) {
        uintptr_t val = *(uintptr_t*)(th->stacktrace() + (i * sizeof(uintptr_t)));
        snprintf(tmp, sizeof(tmp), "0x%" PRIxPTR, val);
        writer.value(tmp);
      }
      writer.endArray();
    }
    writer.endObject();
  }
  writer.endArray();

  out[std::min(sz - 1, writer.dataSize())] = '\0';
  return writer.dataSize();
}

inline bool TracerService::allocate(ThreadEntry* th, uint8_t* ptr, size_t available, size_t needed) {
  if (available >= needed) {
    memmove(ptr + needed, ptr + available,
            (uint8_t*)(location_ + size_) - (ptr + available));

    return true;
  } else {
    const size_t moveRight = (needed - available);
    if (freeSpace() >= moveRight) {
      ThreadEntry* lt = last(true);
      if (lt == nullptr) {
        lt = first();
      }
      uint8_t* end = (uint8_t*)lt;
      end += lt->size;
      memmove(ptr + moveRight, ptr, end - ptr);
      return true;
    } else {
      return false;
    }
  }

  return false;
}

inline bool TracerService::setThreadCheckpoint(ThreadEntry* h, tracer_checkpoint_t* chkpt) {
  if (h == nullptr) {
    return false;
  }

  const size_t currentSize = h->checkpointSize();
  const size_t newSize = size(chkpt);

  if (allocate(h, h->checkpoint(), currentSize, newSize)) {
    h->updateCheckpoint(chkpt);
  } else {
    // Remove it
    allocate(h, h->checkpoint(), currentSize, 0);
    h->updateCheckpoint(nullptr);
    return false;
  }


  return true;
}

inline ThreadEntry* TracerService::first() const {
  // Never returns null
  ThreadEntry* h = (ThreadEntry*)((uint8_t*)(location_) + sizeof(uint32_t));
  return h;
}

inline ThreadEntry* TracerService::last(bool initialized) const {
  ThreadEntry* l = first();
  ThreadEntry* prev = l;
  for (ThreadEntry* h = l; h != nullptr; h = next(h)) {
    prev = l;
    l = h;
    if (!h->initialized()) {
      break;
    }
  }

  if (initialized && !l->initialized()) {
    return (prev != nullptr && prev->initialized()) ? prev : nullptr;
  }

  if (!initialized && l->initialized()) {
    return nullptr;
  }

  return l;
}

inline ThreadEntry* TracerService::next(ThreadEntry* t) const {
  if (t == nullptr || !t->initialized()) {
    return nullptr;
  }

  uint8_t* ptr = ((uint8_t*)t) + t->size;
  if (isThreadEntry(ptr)) {
    return (ThreadEntry*)(ptr);
  }

  return nullptr;
}

inline ThreadEntry* TracerService::findThread(os_unique_id_t th) const {
  for(ThreadEntry* h = first(); h != nullptr && h->initialized(); h = next(h)) {
    if (h->id == th) {
      return h;
    }
  }

  return nullptr;
}

inline bool TracerService::isThreadEntry(uint8_t* ptr) const {
  if (ptr >= ((uint8_t*)location_ + sizeof(uint32_t)) &&
      ptr < ((uint8_t*)(location_ + size_) - sizeof(ThreadEntry))) {

    return true;
  }

  return false;
}

inline size_t TracerService::freeSpace() const {
  ThreadEntry* t = last(true);
  if (t == nullptr) {
    t = first();
  }
  uint8_t* end = (uint8_t*)t;
  end += t->size;

  if (end <= (location_ + size_)) {
    return ((uint8_t*)(location_ + size_) - end);
  }

  return 0;
}

} } // namespace particle::tracer

#endif // SYSTEM_TRACER_SERVICE_IMPL_H
