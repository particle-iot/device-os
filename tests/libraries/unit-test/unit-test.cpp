#include "application.h"
#include "unit-test.h"

const uint8_t Test::UNSETUP = 0;
const uint8_t Test::LOOPING = 1;
const uint8_t Test::DONE_SKIP = 2;
const uint8_t Test::DONE_PASS = 3;
const uint8_t Test::DONE_FAIL = 4;

Test::TestString::TestString(const char *_data) : data((uint32_t)_data) {}

void Test::TestString::read(void *destination, uint16_t offset, uint8_t length) const
{
    memcpy(destination,(char*)(data+offset),length);
}

uint16_t Test::TestString::length() const {
    return strlen((char*)(data));
}

int8_t Test::TestString::compare(const Test::TestString &to) const
{
  uint8_t a_buf[4],b_buf[4];
  uint16_t i=0;

  for (;;) {
    uint8_t j=(i%4);
    if (j == 0) {
      this->read(a_buf,i,4);
      to.read(b_buf,i,4);
    }
    if (a_buf[j] < b_buf[j]) return -1;
    if (a_buf[j] > b_buf[j]) return  1;
    if (a_buf[j] == 0) return 0;
    ++i;
  }
}

size_t Test::TestString::printTo(Print &p) const {
    return p.print((char*)data);
}

bool Test::TestString::matches(const char *pattern) const {
  uint8_t np = strlen(pattern);
  uint8_t ns = length();
  uint8_t nb = (np+2)/8+((np+2)%8 != 0);
  uint8_t k;
  uint8_t buffer[8];

  uint8_t buffer0[nb];
  uint8_t buffer1[nb];

  uint8_t *state0=buffer0;
  uint8_t *state1=buffer1;

  memset(state0,0,nb);
  state0[0]=1;
  for (k=1; pattern[k-1] == '*'; ++k) state0[k/8] |= (1 << (k%8));

  for (int i=0; i<=ns; ++i) {
    if ((i%sizeof(buffer)) == 0) {
      uint8_t n=sizeof(buffer);
      if (ns+1-i < n) n=ns+1-i;
      read(buffer,i,n);
    }
    uint8_t c=buffer[i%sizeof(buffer)];

    memset(state1,0,nb);
    for (int j=0; j<=np; ++j) {
      if (state0[j/8] & (1 << (j%8))) {
        if (pattern[j] == '*') {
          k=j;
          state1[k/8] |= (1 << (k%8));
          ++k;
          state1[k/8] |= (1 << (k%8));
        } else if (pattern[j] == c) {
          k=j+1;
          state1[k/8] |= (1 << (k%8));
          while (pattern[k] == '*') {
            ++k;
            state1[k/8] |= (1 << (k%8));
          }
        }
      }
    }

    uint8_t *tmp = state0;
    state0 = state1;
    state1 = tmp;
  }

  k=np+1;
  return (state0[k/8]&(1<<(k%8))) != 0;
}

Test* Test::root = 0;
Test* Test::current = 0;

uint32_t Test::count = 0;
uint32_t Test::passed = 0;
uint32_t Test::failed = 0;
uint32_t Test::skipped = 0;
uint8_t Test::max_verbosity = TEST_VERBOSITY_ALL;
uint8_t Test::min_verbosity = TEST_VERBOSITY_TESTS_SUMMARY;

Print* Test::out = &Serial;

void Test::resolve()
{
  bool pass = current->state==DONE_PASS;
  bool fail = current->state==DONE_FAIL;
  bool skip = current->state==DONE_SKIP;
  bool done = (pass || fail || skip);

  if (done) {
    if (pass) ++Test::passed;
    if (fail) ++Test::failed;
    if (skip) ++Test::skipped;
    
    _runner.testDone();

#if TEST_VERBOSITY_EXISTS(TESTS_SKIPPED) || TEST_VERBOSITY_EXISTS(TESTS_PASSED) || TEST_VERBOSITY_EXISTS(TESTS_FAILED)

    bool output = false;

    output = output || (skip && TEST_VERBOSITY(TESTS_SKIPPED));
    output = output || (pass && TEST_VERBOSITY(TESTS_PASSED));
    output = output || (fail && TEST_VERBOSITY(TESTS_FAILED));

    if (output) {
      out->print(F("Test "));
      out->print(name);
#if TEST_VERBOSITY_EXISTS(TESTS_SKIPPED)
      if (skip) out->println(F(" skipped."));
#endif

#if TEST_VERBOSITY_EXISTS(TESTS_PASSED)
      if (pass) out->println(F(" passed."));
#endif

#if TEST_VERBOSITY_EXISTS(TESTS_FAILED)
      if (fail) out->println(F(" failed."));
#endif
    }
#endif
  }
#if TEST_VERBOSITY_EXISTS(TESTS_SUMMARY)
  if (root == 0 && TEST_VERBOSITY(TESTS_SUMMARY)) {
    out->print(F("Test summary: "));
    out->print(passed);
    out->print(F(" passed, "));
    out->print(failed);
    out->print(F(" failed, and "));
    out->print(skipped);
    out->print(F(" skipped, out of "));
    out->print(count);
    out->println(F(" test(s)."));
  }  
#endif
}

void Test::remove()
{
  for (Test **p = &root; *p != 0; p=&((*p)->next)) {
    if (*p == this) {
      *p = (*p)->next;
      break;
    }
  }
}

Test::Test(const char *_name, uint8_t _verbosity)
  : name(_name), verbosity(_verbosity)
{
  insert();
}

void Test::insert()
{
  state = UNSETUP;
  { // keep list sorted
    Test **p = &Test::root;
    while ((*p) != 0) {
      if (name.compare((*p)->name) <= 0) break;
      p=&((*p)->next);
    }
    next=(*p);
    (*p)=this;
  }
  ++Test::count;
}

void Test::pass() { state = DONE_PASS; }
void Test::fail() { state = DONE_FAIL; }
void Test::skip() { state = DONE_SKIP; }

void Test::setup() {};

void Test::run()
{
  _runner.setState(root ? RUNNING : COMPLETE);
  
  for (Test **p = &root; (*p) != 0; ) {
    current = *p;

    if (current->state == LOOPING) {
      current->loop();
    } else if (current->state == UNSETUP) {
      current->setup();
      if (current->state == UNSETUP) {
        current->state = LOOPING;
      }
    }

    if (current->state != LOOPING) {
      (*p)=((*p)->next);
      current->resolve();
    } else {
      p=&((*p)->next);
    }
    break;  // allow main loop to execute
  }
}

Test::~Test()
{
  remove();
}

void Test::include(const char *pattern)
{
  for (Test *p = root; p != 0; p=p->next) {
    if (p->state == DONE_SKIP && p->name.matches(pattern)) {
      p->state = UNSETUP;
    }
  }
}

void Test::exclude(const char *pattern)
{
  for (Test *p = root; p != 0; p=p->next) {
    if (p->state == UNSETUP && p->name.matches(pattern)) {
      p->state = DONE_SKIP;
    }
  }
}

TestOnce::TestOnce(const char *name) : Test(name) {}

void TestOnce::loop()
{
  once();
  if (state == LOOPING) state = DONE_PASS;
}


template <>
bool isLess<const char*>(const char* const &a, const char* const &b)
{
  return (strcmp(a,b) < 0);
}

template <>
bool isLessOrEqual<const char*>(const char* const &a, const char* const &b)
{
  return (strcmp(a,b) <= 0);
}

template <>
bool isEqual<const char*>(const char* const &a, const char* const &b)
{
  return (strcmp(a,b) == 0);
}

template <>
bool isNotEqual<const char*>(const char* const &a, const char* const &b)
{
  return (strcmp(a,b) != 0);
}

template <>
bool isMoreOrEqual<const char*>(const char* const &a, const char* const &b)
{
  return (strcmp(a,b) >= 0);
}

template <>
bool isMore<const char*>(const char* const &a, const char* const &b)
{
  return (strcmp(a,b) > 0);
}
