#include "tools/random.h"
#include "tools/catch.h"
#include <boost/crc.hpp>
#include <unordered_map>

#include "hippomocks.h"

#define DIAGNOSTIC_LOCATION_SIZE 1024
static uint8_t s_diagnostic_area[DIAGNOSTIC_LOCATION_SIZE] = {0};

#define DIAGNOSTIC_LOCATION_BEGIN (&s_diagnostic_area[0])
#define DIAGNOSTIC_LOCATION_END (&s_diagnostic_area[0] + DIAGNOSTIC_LOCATION_SIZE)

#include "concurrent_hal.h"
#include "core_hal.h"

#define DIAGNOSTIC_LOCATION_EXTERNAL_DEFINES
#include "diagnostic.h"
#include "diagnostic_impl.h"

int HAL_disable_irq()
{
  return 0;
}

void HAL_enable_irq(int is) {
}

void os_thread_scheduling(bool enable, void* reserved) {
}

uint32_t HAL_Core_Compute_CRC32(const uint8_t *pBuffer, uint32_t bufferSize)
{
  boost::crc_32_type result;
  result.process_bytes(pBuffer, bufferSize);
  return result.checksum();
}

int HAL_Core_Generate_Stacktrace(os_thread_dump_info_t* info, HAL_Stacktrace_Callback callback, void* ptr, void* reserved) {
  return 0;
}

os_unique_id_t os_thread_unique_id(os_thread_t th) {
  return (os_unique_id_t)th;
}

namespace {

using namespace particle::diagnostic;

class DiagnosticTestService : public DiagnosticService {
public:
  DiagnosticTestService()
      : DiagnosticService(s_diagnostic_area, sizeof(s_diagnostic_area)) {

  }

  bool pallocate(ThreadEntry* th, uint8_t* ptr, size_t available, size_t needed) {
    return allocate(th, ptr, available, needed);
  }

  ThreadEntry* pfirst() const {
    return first();
  }

  ThreadEntry* plast(bool initialized = false) const {
    return last(initialized);
  }

  ThreadEntry* pnext(ThreadEntry* t) const {
    return next(t);
  }

  ThreadEntry* pfindThread(os_unique_id_t th) {
    return findThread(th);
  }

  bool pvalid() const {
    return valid();
  }

  uint32_t pcomputeCrc() const {
    return computeCrc();
  }

  void pupdateCrc() {
    return updateCrc();
  }

  bool pisThreadEntry(uint8_t* ptr) const {
    return isThreadEntry(ptr);
  }

  void pinitialize() {
    return initialize();
  }

  static size_t psize(diagnostic_checkpoint_t* chkpt) {
    return size(chkpt);
  }

  size_t pfreeSpace() {
    return freeSpace();
  }

  bool psavedAvailable() {
    return savedAvailable_;
  }
};

const char* s_thread_names[] = {
  "thread0",
  "thread1",
  "thread2",
  "thread3",
  "thread4",
  "thread5",
  "thread6",
  "thread7",
  "thread8",
  "thread9",
  "thread10",
  "thread11",
  "thread12",
  "thread13",
  "thread14",
  "thread15",
  "thread16"
};

struct ThreadEntryValidator {
  ThreadEntryValidator(ThreadEntry* th, os_thread_dump_info_t* info, diagnostic_checkpoint_t* chkpt) {
    const size_t checkpointSize = DiagnosticTestService::psize(chkpt);
    const size_t stacktraceSize = th->stacktraceSize();
    value = (
        th != nullptr && chkpt != nullptr &&
        th->initialized() == true &&
        (th->size == sizeof(*th) + strnlen(info->name, maxThreadNameLength) + 1 + checkpointSize + stacktraceSize) &&
        (th->checkpointSize() == checkpointSize) &&
        !strncmp(th->name(), info->name, strlen(th->name())) &&
        (chkpt->type != CHECKPOINT_TYPE_TEXTUAL || !strncmp(th->checkpointText(), chkpt->text, strlen(th->checkpointText()))) &&
        th->checkpointAddress() && *(th->checkpointAddress()) == (uintptr_t)chkpt->instruction
    );
  }

  operator bool() const {
    return value;
  }

  bool value;
};

struct StacktraceGenerator {
  StacktraceGenerator(size_t num)
      : maxItems(num) {
  }

  int HAL_Core_Generate_Stacktrace(os_thread_dump_info_t* info, HAL_Stacktrace_Callback callback, void* ptr, void* reserved) {
    auto it = stacktraces.find(info->thread);
    if (it == stacktraces.end()) {
      auto& gen = test::randomGenerator();
      std::uniform_int_distribution<uintptr_t> sdist;
      std::uniform_int_distribution<size_t> maxdist(1, maxItems);
      std::vector<uintptr_t> strace;
      const size_t items = maxdist(gen);
      for (size_t i = 0; i < items; i++) {
        strace.push_back(sdist(gen));
      }
      stacktraces.insert({info->thread, std::move(strace)});
      it = stacktraces.find(info->thread);
    }

    if (it != stacktraces.end()) {
      for (unsigned i = 0; i < it->second.size(); i++) {
        if (callback(ptr, i, (it->second)[i], info, nullptr) != 0) {
          return i;
        }
      }

      return it->second.size();
    }

    return 0;
  }

  std::unordered_map<os_thread_t, std::vector<uintptr_t> > stacktraces;
  const size_t maxItems;
};

class StacktraceMocks {
public:
    StacktraceMocks(StacktraceGenerator& gen)
        : gen_(gen) {
      mocks_.OnCallFunc(HAL_Core_Generate_Stacktrace).Do([&](os_thread_dump_info_t* info, HAL_Stacktrace_Callback callback, void* ptr, void* reserved) -> int {
        return gen_.HAL_Core_Generate_Stacktrace(info, callback, ptr, reserved);
      });
    }

private:
  StacktraceGenerator& gen_;
  MockRepository mocks_;
};


} // namespace

TEST_CASE("DiagnosticService") {
  DiagnosticTestService diag;

  std::vector<os_thread_dump_info_t> threads;
  for(unsigned i = 0; i < 16; i++) {
    os_thread_dump_info_t th = {
      0,
      reinterpret_cast<os_thread_t>(i),
      s_thread_names[i],
      static_cast<os_unique_id_t>(i),
      nullptr,
      nullptr,
      nullptr
    };
    threads.push_back(th);
  }

  SECTION("constructed") {
    REQUIRE(diag.isValid() == true);
    REQUIRE(diag.pfreeSpace() == (DIAGNOSTIC_LOCATION_SIZE - sizeof(uint32_t)));
    auto f = diag.pfirst();
    REQUIRE((uint8_t*)f == ((uint8_t*)DIAGNOSTIC_LOCATION_BEGIN + sizeof(uint32_t)));
    REQUIRE(f->initialized() == false);
    REQUIRE(diag.plast() == diag.pfirst());
    REQUIRE(diag.plast(true) == nullptr);
    REQUIRE(diag.psavedAvailable() == false);
  }

  SECTION("long thread name and long textual checkpoint get truncated") {
    threads[0].name = "this a very long thread name that should be truncated";
    diagnostic_checkpoint_t chkpt = {0};
    chkpt.type = CHECKPOINT_TYPE_TEXTUAL;
    chkpt.text = "this is a very long checkpoint that should be truncated 12345678901234567890-=1234551235";
    chkpt.instruction = (void*)0xdeadbeef;
    auto result = diag.insertCheckpoint(&threads[0], &chkpt, false);
    diag.updateCrc();
    REQUIRE(result == DIAGNOSTIC_ERROR_NONE);
    auto f = diag.pfirst();
    REQUIRE(ThreadEntryValidator(f, &threads[0], &chkpt) == true);
    REQUIRE(strlen(f->name()) == maxThreadNameLength);
    REQUIRE(strlen(f->checkpointText()) == maxTextualCheckpointLength);
  }

  SECTION("insert thread entry with an instruction checkpoint") {
    diagnostic_checkpoint_t chkpt = {0};
    chkpt.type = CHECKPOINT_TYPE_INSTRUCTION_ADDRESS;
    chkpt.instruction = (void*)0xdeadbeef;
    auto result = diag.insertCheckpoint(&threads[0], &chkpt, false);
    diag.updateCrc();
    REQUIRE(result == DIAGNOSTIC_ERROR_NONE);
    auto f = diag.pfirst();
    REQUIRE(ThreadEntryValidator(f, &threads[0], &chkpt) == true);
    REQUIRE(diag.pfreeSpace() == (DIAGNOSTIC_LOCATION_SIZE - sizeof(uint32_t) - f->size));

    SECTION("update thread entry with a textual checkpoint") {
      diagnostic_checkpoint_t chkpt = {0};
      chkpt.type = CHECKPOINT_TYPE_TEXTUAL;
      chkpt.instruction = (void*)0xdeadcafe;
      chkpt.text = "a test string";
      auto result = diag.insertCheckpoint(&threads[0], &chkpt, false);
      diag.updateCrc();
      REQUIRE(result == DIAGNOSTIC_ERROR_NONE);
      auto f = diag.pfirst();
      REQUIRE(ThreadEntryValidator(f, &threads[0], &chkpt) == true);
      REQUIRE(diag.pfreeSpace() == (DIAGNOSTIC_LOCATION_SIZE - sizeof(uint32_t) - f->size));
    }

    SECTION("insert maximum number of thread entries") {
      diagnostic_checkpoint_t chkpt = {0};
      chkpt.type = CHECKPOINT_TYPE_TEXTUAL;
      chkpt.instruction = (void*)0xdeadbeef;
      chkpt.text = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
      bool result = true;
      int i = 1;
      while (result && i < 16) {
        result = diag.insertCheckpoint(&threads[i], &chkpt, false) == DIAGNOSTIC_ERROR_NONE;
        if (result) {
          i++;
        }
      }
      diag.updateCrc();
      REQUIRE(i > 1);
      REQUIRE(diag.pfreeSpace() < diag.psize(&chkpt) + sizeof(ThreadEntry));
      // Count number of threads
      int count = 0;
      for(ThreadEntry* t = diag.pfirst(); t != nullptr && t->initialized(); t = diag.pnext(t)) {
        count++;
      }
      REQUIRE(count == i);

      SECTION("unmark and remove all entries") {
        diag.unmarkAll();
        diag.removeUnmarked();
        REQUIRE(diag.pfreeSpace() == DIAGNOSTIC_LOCATION_SIZE - sizeof(uint32_t));
      }
    }

    SECTION("unmark single entry and remove it") {
      diag.unmarkAll();
      diag.removeUnmarked();
      REQUIRE(diag.pfreeSpace() == DIAGNOSTIC_LOCATION_SIZE - sizeof(uint32_t));
    }
  }

  SECTION("insert all threads with stacktraces") {
    StacktraceGenerator gen(5);
    StacktraceMocks mocks_(gen);
    for (auto& th : threads) {
      REQUIRE(diag.insertCheckpoint(&th, nullptr, true) == DIAGNOSTIC_ERROR_NONE);
    }
    diag.updateCrc();
    REQUIRE(diag.isValid() == true);

    for (auto& th : threads) {
      auto f = diag.pfindThread(th.id);
      REQUIRE(f != nullptr);
      auto stacktrace = gen.stacktraces[th.thread];
      REQUIRE(f->count == stacktrace.size());
      for (unsigned i = 0; i < f->count; i++) {
        REQUIRE(*((uintptr_t*)f->stacktrace() + i) == stacktrace[i]);
      }
    }
  }

  SECTION("insert all threads with stacktraces plus a checkpoint") {
    StacktraceGenerator gen(5);
    StacktraceMocks mocks_(gen);

    diagnostic_checkpoint_t chkpt = {0};
    chkpt.type = CHECKPOINT_TYPE_TEXTUAL;
    chkpt.instruction = (void*)0xdeadcafe;
    chkpt.text = "a test string";

    const os_unique_id_t id = 10;

    for (auto& th : threads) {
      if (th.id == id) {
        REQUIRE(diag.insertCheckpoint(&th, &chkpt, true) == DIAGNOSTIC_ERROR_NONE);
      } else {
        REQUIRE(diag.insertCheckpoint(&th, nullptr, true) == DIAGNOSTIC_ERROR_NONE);
      }
    }
    diag.updateCrc();
    REQUIRE(diag.isValid() == true);

    for (auto& th : threads) {
      auto f = diag.pfindThread(th.id);
      REQUIRE(f != nullptr);
      auto stacktrace = gen.stacktraces[th.thread];
      REQUIRE(f->count == stacktrace.size());
      for (unsigned i = 0; i < f->count; i++) {
        REQUIRE(*((uintptr_t*)f->stacktrace() + i) == stacktrace[i]);
      }

      if (th.id == id) {
        REQUIRE(ThreadEntryValidator(f, &th, &chkpt) == true);
      }
    }
  }
}

