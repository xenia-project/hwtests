#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <cstdint>

// Maybe helpful:
// https://github.com/revel8n/altivec-archc/tree/master/t

// #define XENIA

#ifdef LIBXENON
#include <console/console.h>
#include <debug.h>
#include <dirent.h>
#include <elf/elf.h>
#include <input/input.h>
#include <network/network.h>
#include <ppc/timebase.h>
#include <sys/iosupport.h>
#include <time/time.h>
#include <xenon_soc/xenon_power.h>
#include <xenos/xenos.h>
#endif

// Debug print helper.
void DebugPrint(const char *string, int length) {
#ifdef XENIA
  asm("mr 3, %0;"
      "mr 4, %1;"
      "twi 31, 0, 20;"
      :
      : "r"(string), "r"(length)
      : "r3", "r4");
#else
  printf("%s", string);
#endif
}

void DebugPrint(const char *str) { return DebugPrint(str, strlen(str)); }

void swap(char &a, char &b) {
  char tmp = a;
  a = b;
  b = tmp;
}

/* A utility function to reverse a string  */
void reverse(char str[], int length) {
  int start = 0;
  int end = length - 1;
  while (start < end) {
    swap(*(str + start), *(str + end));
    start++;
    end--;
  }
}

// Implementation of itoa()
char *itoa(int num, char *str, int base) {
  int i = 0;
  bool isNegative = false;

  /* Handle 0 explicitely, otherwise empty string is printed for 0 */
  if (num == 0) {
    str[i++] = '0';
    str[i] = '\0';
    return str;
  }

  // In standard itoa(), negative numbers are handled only with
  // base 10. Otherwise numbers are considered unsigned.
  if (num < 0 && base == 10) {
    isNegative = true;
    num = -num;
  }

  // Process individual digits
  while (num != 0) {
    int rem = num % base;
    str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
    num = num / base;
  }

  // If number is negative, append '-'
  if (isNegative) str[i++] = '-';

  str[i] = '\0';  // Append string terminator

  // Reverse the string
  reverse(str, i);

  return str;
}

bool constant_eq() {
  static volatile int constant = 'XEN\0';
  return constant == 'XEN\0';
}

bool float_nan() {
  // Volatile prevents GCC from optimizing these out.
  volatile float f1 = NAN;
  volatile float f2 = NAN;

  if (f1 != f2) {
    return true;
  }

  return false;
}

bool float_fabs() {
  volatile float f1 = -5.f;
  return fabs(f1) == 5.f;
}

bool float_sqrt() {
  volatile float f1 = 4.f;
  return sqrtf(f1) == 2.f;
}

// https://github.com/dolphin-emu/hwtests/tree/master/cputest
// VERIFIED WITH HARDWARE!
static bool FctiwzTest() {
  uint64_t values[][2] = {
      // input               expected output
      {0x0000000000000000, 0x0000000000000000},  // +0
      {0x8000000000000000, 0x0000000000000000},  // -0 (!)
      {0x0000000000000001, 0x0000000000000000},  // smallest positive subnormal
      {0x000fffffffffffff, 0x0000000000000000},  // largest subnormal
      {0x3ff0000000000000, 0x0000000000000001},  // +1
      {0xbff0000000000000, 0xffffffffffffffff},  // -1
      {0xc1e0000000000000, 0xffffffff80000000},  // -(2^31)
      {0x41dfffffffc00000, 0x000000007fffffff},  // 2^31 - 1
      {0x7ff0000000000000, 0x000000007fffffff},  // +infinity
      {0xfff0000000000000, 0xffffffff80000000},  // -infinity
      {0xfff8000000000000, 0xffffffff80000000},  // a QNaN
      {0xfff4000000000000, 0xffffffff80000000},  // a SNaN
  };

  bool success = true;
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
    uint64_t input = values[i][0];
    uint64_t expected = values[i][1];
    uint64_t result = 0;
    asm("fctiwz %0, %1" : "=f"(result) : "f"(input));
    if (result != expected) {
      char buf[1024];
      sprintf(buf,
              "fctiwz(0x%016llx):\n"
              "     got 0x%016llx\n"
              "expected 0x%016llx\n",
              input, result, expected);
      DebugPrint(buf);
      success = false;
    }
  }

  return success;
}

// https://github.com/dolphin-emu/hwtests/tree/master/cputest
// VERIFIED WITH HARDWARE!
static bool FrspTest() {
  // clang-format off
  uint64_t values[][3] = {
    // input               expected output   NI RN
    {0x0000000000000000, 0x0000000000000000, 0b000}, // +0
    {0x8000000000000000, 0x8000000000000000, 0b000}, // -0
    {0x0000000000000001, 0x0000000000000000, 0b000}, // smallest positive double subnormal
    {0x000fffffffffffff, 0x0000000000000000, 0b000}, // largest double subnormal
    {0x3690000000000000, 0x0000000000000000, 0b000}, // largest number rounded to zero
    {0x3690000000000001, 0x36a0000000000000, 0b000}, // smallest positive single subnormal
    {0x380fffffffffffff, 0x3810000000000000, 0b100}, // largest single subnormal
    {0x3810000000000000, 0x3810000000000000, 0b100}, // smallest positive single normal
    {0x7ff0000000000000, 0x7ff0000000000000, 0b000}, // +infinity
    {0xfff0000000000000, 0xfff0000000000000, 0b000}, // -infinity
    {0xfff7ffffffffffff, 0xffffffffe0000000, 0b000}, // a SNaN
    {0xffffffffffffffff, 0xffffffffe0000000, 0b000}, // a QNaN
  };
  // clang-format on

  bool success = true;
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
    // Set FPSCR[NI] and FPSCR[RN] (and FPSCR[XE] but that's okay).
    asm("mtfsf 7, %0" ::"f"(values[i][2]));

    uint64_t input = values[i][0];
    uint64_t expected = values[i][1];
    uint64_t result = 0;
    asm("frsp %0, %1" : "=f"(result) : "f"(input));
    if (result != expected) {
      char buf[1024];
      sprintf(buf,
              "frsp(0x%016llx, NI=%lld):\n"
              "     got 0x%016llx\n"
              "expected 0x%016llx\n",
              input, values[i][2] >> 2, result, expected);
      DebugPrint(buf);
      success = false;
    }
  }

  return success;
}

#ifndef _rotl
static inline uint32_t _rotl(uint32_t x, int shift) {
  shift &= 31;
  if (!shift) return x;
  return (x << shift) | (x >> (32 - shift));
}

static inline uint32_t _rotr(uint32_t x, int shift) {
  shift &= 31;
  if (!shift) return x;
  return (x >> shift) | (x << (32 - shift));
}
#endif

typedef bool (*TestFn)();
struct Test {
  TestFn fn;
  const char *name;
};

#define DEFINE_TEST(test) \
  { test, #test }
Test tests[] = {
    DEFINE_TEST(constant_eq), DEFINE_TEST(float_nan),  DEFINE_TEST(float_fabs),
    DEFINE_TEST(float_sqrt),  DEFINE_TEST(FctiwzTest), DEFINE_TEST(FrspTest),
};
#undef DEFINE_TEST

#ifdef XENIA
extern "C" int _start() {
#else
int main() {
#endif

#ifdef LIBXENON
  // Setup the console.
  if (!xenos_is_initialized()) {
    xenos_init(VIDEO_MODE_AUTO);
  }
  console_init();

  xenon_make_it_faster(XENON_SPEED_FULL);

  network_init();
  network_print_config();
  delay(5);
#endif
  DebugPrint("Beginning tests.\n");

  char buff[1024];
  int failed = 0;
  int succeeded = 0;
  for (int i = 0; i < sizeof(tests) / sizeof(Test); i++) {
    if (!tests[i].fn()) {
      memset(buff, 0, 1024);
      strcat(buff, "TEST \"");
      strcat(buff, tests[i].name);
      strcat(buff, "\" FAILED!\n");

      DebugPrint(buff);
      failed++;
    } else {
      memset(buff, 0, 1024);
      strcat(buff, "Test \"");
      strcat(buff, tests[i].name);
      strcat(buff, "\" Succeeded!\n");

      DebugPrint(buff);
      succeeded++;
    }
  }

  if (failed == 0) {
    DebugPrint("All tests succeeded!\n");
  }

#ifdef LIBXENON
  delay(10);
#endif
  return 0;
}
