#include "../../unity/src/unity.h"
#include "../src/vector.h"
#include <gc.h>

/* included for time and rand functions */
#include <time.h>
#include <stdlib.h>

/* magic number */
#define BUFFER_SIZE 32

/* number of items to add to test lists */
#define TEST_ITERATIONS 10000

Vector *vector_reverse(Vector *vec);

/* utility functions */
char *make_test_str(int i) {

  char *buf = GC_MALLOC(BUFFER_SIZE);
  snprintf(buf, BUFFER_SIZE - 1, "test_string_%d", i);
  return buf;
}

void setUp(void) {
  /* set up global state here */
}

void tearDown(void) {
  /* clean up global state here */
}

/* tests */
void test_vector_make(void)
{
  /* empty create vector */
  Vector *vec = vector_make();

  TEST_ASSERT_EQUAL_INT(0, vector_count(vec));
  TEST_ASSERT_EQUAL_INT(1, vector_empty(vec));
}

void test_vector_push(void)
{
  Vector *vec = vector_make();

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    vec = vector_push(vec, (void *)val);
    /* check data at each index */
    TEST_ASSERT_EQUAL_STRING(val, (char*)vector_get(vec, i));
  }

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    /* check data at each index */
    TEST_ASSERT_EQUAL_STRING(val, (char*)vector_get(vec, i));
  }
}

void test_vector_pop(void)
{
  Vector *vec = vector_make();

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    vec = vector_push(vec, (void *)val);
  }

  for (int i = TEST_ITERATIONS - 1; i >= 0; i--) {

    char* val = make_test_str(i);

    /* check number of elements decrments */
    TEST_ASSERT_EQUAL_INT(vector_count(vec), i + 1);
    /* check data element at end of vector */
    TEST_ASSERT_EQUAL_STRING(val, (char*)vector_get(vec, i));

    vec = vector_pop(vec);
  }
  /* final vector is empty */
  TEST_ASSERT_EQUAL_INT(1, vector_empty(vec));
}

void test_vector_get(void)
{
  srand((int)time(NULL));

  Vector *vec = vector_make();

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    vec = vector_push(vec, (void *)val);
  }

  for (int i = 0; i < TEST_ITERATIONS; i++) {

    int r = (rand() % TEST_ITERATIONS);
    char* val = make_test_str(r);
    char* nth_val = vector_get(vec, r);

    /* test a random index has the right values */
    TEST_ASSERT_EQUAL_STRING(val, nth_val);
  }

  /* test out of bounds */
  TEST_ASSERT_NULL(vector_get(vec, -1));
  TEST_ASSERT_NULL(vector_get(vec, TEST_ITERATIONS+1));
}

void test_vector_set(void) {

  Vector *vec = vector_make();

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    vec = vector_push(vec, (void *)val);
  }

  /* test out of bounds */
  vec = vector_set(vec, -1 ,"updated");
  TEST_ASSERT_NULL(vector_get(vec, -1));

  vec = vector_set(vec, TEST_ITERATIONS+1,"updated");
  TEST_ASSERT_NULL(vector_get(vec, TEST_ITERATIONS+1));

  /* in bounds */
  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = "updated";
    vec = vector_set(vec, i, (void *)val);
    TEST_ASSERT_EQUAL_STRING(val, vector_get(vec,i));
  }
}

void test_vector_empty(void) {

  /* test empty vector */
  Vector *vec = vector_make();

  TEST_ASSERT_NOT_NULL(vec);
  TEST_ASSERT_EQUAL_INT(1, vector_empty(vec));

  /* test non-empty vector */
  vec = vector_push(vec, (void *)"data");
  TEST_ASSERT_EQUAL_INT(0, vector_empty(vec));
}

void test_vector_count(void)
{
  Vector *vec = vector_make();

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    vec = vector_push(vec, (void *)val);
    /* check size as vector increases */
    TEST_ASSERT_EQUAL_INT(i + 1, vector_count(vec));
  }

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    /* check size as vector decreases */
    TEST_ASSERT_EQUAL_INT(TEST_ITERATIONS - i, vector_count(vec));
    vec = vector_pop(vec);
  }
}

void test_vector_iterator(void) {

  Vector *vec = vector_make();

  for (int i = 0; i < TEST_ITERATIONS; i++) {

    char* val = make_test_str(i);
    vec = vector_push(vec, (void *)val);
  }

  /* make an interator */
  Iterator *iter = vector_iterator_make(vec);

  int i = 0;
  while(iter) {

    char* val = make_test_str(i);
    char* iter_val = (char*)iterator_value(iter);

    /* check iterator value */
    TEST_ASSERT_EQUAL_STRING(val, iter_val);

    i++;
    iter = iterator_next(iter);
  }
  TEST_ASSERT_EQUAL_INT(TEST_ITERATIONS, i);
}

void test_vector_readme(void)
{
  Vector *v = vector_make();

  /* add items at the end of the vector */
  v = vector_push(v, "item0");
  v = vector_push(v, "item1");
  v = vector_push(v, "item2");
  v = vector_push(v, "item3");
  printf("count is: %i\n", vector_count(v));
  //=> 4
  TEST_ASSERT_EQUAL_INT(4, vector_count(v));

  /* save a reference to the original vector */
  Vector *v_original = v;

  /* remove an item from the end */
  v = vector_pop(v);
  printf("count is: %i\n", vector_count(v));
  //=> 3;
  TEST_ASSERT_EQUAL_INT(3, vector_count(v));

  /* get the value at index 1 */
  char* item1 = vector_get(v, 1);
  printf("item at index 1 is: %s\n", item1);
  //=> "item1"
  TEST_ASSERT_EQUAL_STRING("item1", item1);

  /* change the value at index 1 */
  v = vector_set(v, 1, "item1_updated");
  item1 = vector_get(v, 1);
  printf("item at index 1 is: %s\n", item1);
  //=> "item1_updated"
  TEST_ASSERT_EQUAL_STRING("item1_updated", item1);

  /* original vector is unaffected */
  printf("original count is: %i\n", vector_count(v_original));
  //=> 4
  TEST_ASSERT_EQUAL_INT(4, vector_count(v_original));

  item1 = vector_get(v_original, 1);
  printf("item at index 1 is: %s\n", item1);
  //=> "item1"
  TEST_ASSERT_EQUAL_STRING("item1", item1);
}

/* run tests */
int main(void)
{
  UNITY_BEGIN();

  RUN_TEST(test_vector_make);
  RUN_TEST(test_vector_push);
  RUN_TEST(test_vector_pop);
  RUN_TEST(test_vector_get);
  RUN_TEST(test_vector_set);
  RUN_TEST(test_vector_empty);
  RUN_TEST(test_vector_count);
  RUN_TEST(test_vector_iterator);
  RUN_TEST(test_vector_readme);

  return UNITY_END();
}



/* Unity MACROS */

/*
TEST_ASSERT_TRUE(condition)
Evaluates whatever code is in condition and fails if it evaluates to false

TEST_ASSERT_FALSE(condition)
Evaluates whatever code is in condition and fails if it evaluates to true

TEST_ASSERT(condition)
Another way of calling TEST_ASSERT_TRUE

TEST_ASSERT_UNLESS(condition)
Another way of calling TEST_ASSERT_FALSE

TEST_FAIL()
TEST_FAIL_MESSAGE(message)
This test is automatically marked as a failure. The message is output stating why.

Numerical Assertions: Integers

TEST_ASSERT_EQUAL_INT(expected, actual)
TEST_ASSERT_EQUAL_INT8(expected, actual)
TEST_ASSERT_EQUAL_INT16(expected, actual)
TEST_ASSERT_EQUAL_INT32(expected, actual)
TEST_ASSERT_EQUAL_INT64(expected, actual)
Compare two integers for equality and display errors as signed integers. A cast will be performed to your natural integer size so often this can just be used. When you need to specify the exact size, like when comparing arrays, you can use a specific version:

TEST_ASSERT_EQUAL_UINT(expected, actual)
TEST_ASSERT_EQUAL_UINT8(expected, actual)
TEST_ASSERT_EQUAL_UINT16(expected, actual)
TEST_ASSERT_EQUAL_UINT32(expected, actual)
TEST_ASSERT_EQUAL_UINT64(expected, actual)
Compare two integers for equality and display errors as unsigned integers. Like INT, there are variants for different sizes also.

TEST_ASSERT_EQUAL_HEX(expected, actual)
TEST_ASSERT_EQUAL_HEX8(expected, actual)
TEST_ASSERT_EQUAL_HEX16(expected, actual)
TEST_ASSERT_EQUAL_HEX32(expected, actual)
TEST_ASSERT_EQUAL_HEX64(expected, actual)
Compares two integers for equality and display errors as hexadecimal. Like the other integer comparisons, you can specify the size... here the size will also effect how many nibbles are shown (for example, HEX16 will show 4 nibbles).

TEST_ASSERT_EQUAL(expected, actual)
Another way of calling TEST_ASSERT_EQUAL_INT

TEST_ASSERT_INT_WITHIN(delta, expected, actual)
Asserts that the actual value is within plus or minus delta of the expected value. This also comes in size specific variants.

TEST_ASSERT_GREATER_THAN(threshold, actual)
Asserts that the actual value is greater than the threshold. This also comes in size specific variants.

TEST_ASSERT_LESS_THAN(threshold, actual)
Asserts that the actual value is less than the threshold. This also comes in size specific variants.

Arrays
_ARRAY
You can append _ARRAY to any of these macros to make an array comparison of that type. Here you will need to care a bit more about the actual size of the value being checked. You will also specify an additional argument which is the number of elements to compare. For example:

TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, actual, elements)

_EACH_EQUAL
Another array comparison option is to check that EVERY element of an array is equal to a single expected value. You do this by specifying the EACH_EQUAL macro. For example:

TEST_ASSERT_EACH_EQUAL_INT32(expected, actual, elements)
Numerical Assertions: Bitwise
TEST_ASSERT_BITS(mask, expected, actual)
Use an integer mask to specify which bits should be compared between two other integers. High bits in the mask are compared, low bits ignored.

TEST_ASSERT_BITS_HIGH(mask, actual)
Use an integer mask to specify which bits should be inspected to determine if they are all set high. High bits in the mask are compared, low bits ignored.

TEST_ASSERT_BITS_LOW(mask, actual)
Use an integer mask to specify which bits should be inspected to determine if they are all set low. High bits in the mask are compared, low bits ignored.

TEST_ASSERT_BIT_HIGH(bit, actual)
Test a single bit and verify that it is high. The bit is specified 0-31 for a 32-bit integer.

TEST_ASSERT_BIT_LOW(bit, actual)
Test a single bit and verify that it is low. The bit is specified 0-31 for a 32-bit integer.

Numerical Assertions: Floats
TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)
Asserts that the actual value is within plus or minus delta of the expected value.

TEST_ASSERT_EQUAL_FLOAT(expected, actual)
TEST_ASSERT_EQUAL_DOUBLE(expected, actual)
Asserts that two floating point values are "equal" within a small % delta of the expected value.

String Assertions
TEST_ASSERT_EQUAL_STRING(expected, actual)
Compare two null-terminate strings. Fail if any character is different or if the lengths are different.

TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, len)
Compare two strings. Fail if any character is different, stop comparing after len characters.

TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, message)
Compare two null-terminate strings. Fail if any character is different or if the lengths are different. Output a custom message on failure.

TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(expected, actual, len, message)
Compare two strings. Fail if any character is different, stop comparing after len characters. Output a custom message on failure.

Pointer Assertions
Most pointer operations can be performed by simply using the integer comparisons above. However, a couple of special cases are added for clarity.

TEST_ASSERT_NULL(pointer)
Fails if the pointer is not equal to NULL

TEST_ASSERT_NOT_NULL(pointer)
Fails if the pointer is equal to NULL

Memory Assertions
TEST_ASSERT_EQUAL_MEMORY(expected, actual, len)
Compare two blocks of memory. This is a good generic assertion for types that can't be coerced into acting like standard types... but since it's a memory compare, you have to be careful that your data types are packed.

_MESSAGE
You can append \_MESSAGE to any of the macros to make them take an additional argument. This argument is a string that will be printed at the end of the failure strings. This is useful for specifying more information about the problem.

*/
