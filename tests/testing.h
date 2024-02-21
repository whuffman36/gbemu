#include "stdio.h"


static int _TESTS_RUN = 0;
static int _TESTS_PASSED = 0;


#define tmodbegin_ \
printf("\nTesting %s...\n\n", __func__);\


#define  tmodend_ \
if (_TESTS_PASSED == _TESTS_RUN) {\
  printf("All tests passed.\n\n");\
}\
else {\
  printf("%d of %d tests passed.\n\n", _TESTS_PASSED, _TESTS_RUN);\
}\


#define tbegin_ \
printf("%s...", __func__);\
_TESTS_RUN++;\


#define tend_ \
printf("PASSED\n\n");\
_TESTS_PASSED++;\


#define tassert_(x)\
if (!(x)) {\
  printf("FAILED\n");\
  __tassert(#x, __FILE__, __func__, __LINE__);\
  return;\
}\


void __tassert(const char* message, const char* file, const char* function, int line) {
  printf("Assertion failed: %s (%s:%s:%d)\n\n", message, file, function, line);
}
