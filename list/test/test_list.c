#include <string.h>
#include "../../Unity/src/unity.h"
#include "../src/list.h"
#include "gc.h"

/* included for time and rand functions */
#include <time.h>
#include <stdlib.h>

/* magic number */
#define BUFFER_SIZE 32

/* number of items to add to test lists */
#define TEST_ITERATIONS 100


/* utility functions */
char *make_test_str(int i) {

  char *buf = GC_MALLOC(BUFFER_SIZE);
  snprintf(buf, BUFFER_SIZE - 1, "test_string_%d", i);
  return buf;
}

int cmp_chars(void *key1, void *key2) {
  return (strcmp((char*)key1, (char*)key2) == 0);
}

void setUp(void) {
  /* set up global state here */
}

void tearDown(void) {
  /* clean up global state here */
}

/* tests */
void test_list_make(void)
{
  List *lst = NULL;

  /* create a list NULL value*/
  lst = list_make(NULL);

  /* test the data */
  TEST_ASSERT_NULL(lst->data);
  /* test the pointer */
  TEST_ASSERT_NULL(lst->next);
  /* test the count */
  TEST_ASSERT_EQUAL_INT(1, list_count(lst));

  /* create list with single item */
  char* val = "Test value";
  lst = list_make((void *)val);

  /* test the data */
  TEST_ASSERT_EQUAL_STRING(val, (char*)lst->data);
  /* test the pointer */
  TEST_ASSERT_NULL(lst->next);
  /* test the count */
  TEST_ASSERT_EQUAL_INT(1, list_count(lst));
}

void test_list_empty(void) {

  List *lst = list_make(NULL);

  /* test non-empty list */
  TEST_ASSERT_NOT_NULL(lst);
  TEST_ASSERT_EQUAL_INT(0, list_empty(lst));

  /* test empty list */
  TEST_ASSERT_EQUAL_INT(1, list_empty(NULL));
}

void test_list_cons(void)
{
  List *lst = NULL;
  List *prev = NULL;

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    prev = lst;
    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);

    /* test the data */
    TEST_ASSERT_EQUAL_STRING(val, (char*)lst->data);
    /* test the pointer*/
    TEST_ASSERT_EQUAL_INT(prev, lst->next);
  }
}

void test_list_first(void)
{
  List *lst = NULL;

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);

    /* test the data */
    TEST_ASSERT_EQUAL_STRING(val, list_first(lst));
  }
}

void test_list_rest(void)
{
  List *lst = NULL;
  List *prev = NULL;

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    prev = lst;
    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);
  }

  for (int i = TEST_ITERATIONS - 1; i > 0; i--) {
    prev = lst;
    char* val = make_test_str(i - 1);
    lst = list_rest(lst);

    /* test the data */
    TEST_ASSERT_EQUAL_STRING(val, (char*)lst->data);
    /* test the pointer */
    TEST_ASSERT_EQUAL_INT(prev->next, lst);
  }

  /* taking rest of a one-element list gives a NULL list */
  TEST_ASSERT_NULL(list_rest(list_make(NULL)));

  /* taking rest of an empty list has no effect */
  TEST_ASSERT_NULL(list_rest(NULL));
}

void test_list_count(void)
{
  List *lst = NULL;

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);

    /* test the count changes as list grows */
    TEST_ASSERT_EQUAL_INT(i + 1, list_count(lst));
  }

  for (int i = 0; i < TEST_ITERATIONS; i++) {

    /* test the count changes as list shrinks */
    TEST_ASSERT_EQUAL_INT(TEST_ITERATIONS - i, list_count(lst));
    lst = list_rest(lst);
  }

  /* count of empty list is 0 */
  TEST_ASSERT_EQUAL_INT(0, list_count(NULL));
}

void test_list_nth(void)
{
  srand((int)time(NULL));

  List *lst = NULL;

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);
  }

  for (int i = 0; i < TEST_ITERATIONS; i++) {

    int r = (rand() % TEST_ITERATIONS);
    char* val = make_test_str(TEST_ITERATIONS - 1 - r);
    char* nth_val = list_nth(lst, r);

    /* test a random index has the right values */
    TEST_ASSERT_EQUAL_STRING(val, nth_val);
  }
}

void test_list_reverse(void)
{
  List *lst = NULL;
  List *rev = NULL;

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);
  }

  rev = list_reverse(lst);
  lst = NULL;

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);
  }

  for (int i = 0; i < TEST_ITERATIONS; i++) {

    char* val = list_nth(lst, i);
    char* rev_val = list_nth(rev, TEST_ITERATIONS - 1 - i);

    TEST_ASSERT_EQUAL_STRING(val, rev_val);
  }

  /* reversing an empty list does nothing */
  TEST_ASSERT_NULL(list_reverse(NULL));

  /* reversing a single element list does nothing */
  lst = list_make("val");
  rev = list_reverse(lst);
  TEST_ASSERT_EQUAL_INT(lst, rev);

  lst = NULL;
  rev = NULL;
  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);
  }

  /* reversing a reversed list has no effect */
  rev = list_reverse(list_reverse(lst));
  TEST_ASSERT_EQUAL_INT(lst, rev);
}

void test_list_copy(void)
{
  List *lst = NULL;
  List *copy = NULL;

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);
  }

  copy = list_copy(lst);

  /* check head pointers are different */
  TEST_ASSERT_NOT_EQUAL_INT(lst, copy);

  for (int i = 0; i < TEST_ITERATIONS; i++) {

    char* val = list_nth(lst, i);
    char* copy_val = list_nth(copy, i);

    /* check data is the same */
    TEST_ASSERT_EQUAL_STRING(val, copy_val);
  }
}

void test_list_concatenate(void)
{
  List *lst_1 = NULL;
  List *lst_2 = NULL;
  List *lst_cat = NULL;

  /* data for first list */
  for (int i = (TEST_ITERATIONS / 4) - 1; i >= 0; i--) {
    char* val = make_test_str(i);
    lst_1 = list_cons(lst_1, (void *)val);
  }

  /* data for second list */
  for (int i = (TEST_ITERATIONS / 2) - 1; i >= (TEST_ITERATIONS / 4); i--) {
    char* val = make_test_str(i);
    lst_2 = list_cons(lst_2, (void *)val);
  }

  lst_cat = list_concatenate(lst_1, lst_2);

  /* check combined list */
  for (int i = 0; i < (TEST_ITERATIONS / 2); i++) {

    char* val = make_test_str(i);
    char* cat_val = list_nth(lst_cat, i);

    /* check the data is correct */
    TEST_ASSERT_EQUAL_STRING(val, cat_val);
  }

  /* concatenating an empty list returns a copy of the original list */
  List *lst = list_make("val");

  lst_cat = list_concatenate(lst, NULL);
  TEST_ASSERT_EQUAL_STRING("val", (char*)lst_cat->data);

  lst_cat = list_concatenate(NULL, lst);
  TEST_ASSERT_EQUAL_STRING("val", (char*)lst_cat->data);

  /* concatenating empty lists returns an empty list */
  TEST_ASSERT_NULL(list_concatenate(NULL, NULL));
}

void test_list_find(void)
{
  srand((int)time(NULL));

  List *lst = NULL;

  for (int i = 0; i < TEST_ITERATIONS; i++) {
    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);
  }

  for (int i = 0; i < TEST_ITERATIONS; i++) {

    int r = (rand() % TEST_ITERATIONS);
    char* val = make_test_str(TEST_ITERATIONS - 1 - r);

    int result = list_find(lst, val, cmp_chars);
    /* check present values are found in the right position */
    TEST_ASSERT_EQUAL_INT(r, result);
  }

  /* item not found returns -1 */
  TEST_ASSERT_EQUAL_INT(-1, list_find(lst, "NOT PRESENT", cmp_chars));
  /* null item returns -1 */
  TEST_ASSERT_EQUAL_INT(-1, list_find(lst, NULL, cmp_chars));
}

void test_list_iterator(void) {

  List *lst = NULL;

  for (int i = 0; i < TEST_ITERATIONS; i++) {

    char* val = make_test_str(i);
    lst = list_cons(lst, (void *)val);
  }

  /* make an interator */
  Iterator *iter = list_iterator_make(lst);

  int i = TEST_ITERATIONS;
  while(iter) {
    i--;

    char* val = make_test_str(i);
    char* iter_val = (char*)iterator_value(iter);

    /* check iterator value */
    TEST_ASSERT_EQUAL_STRING(val, iter_val);

    iter = iterator_next(iter);
  }
  TEST_ASSERT_EQUAL_INT(0, i);
}


/* run tests */
int main(void)
{
  UNITY_BEGIN();

  RUN_TEST(test_list_make);
  RUN_TEST(test_list_empty);
  RUN_TEST(test_list_cons);
  RUN_TEST(test_list_first);
  RUN_TEST(test_list_rest);
  RUN_TEST(test_list_count);
  RUN_TEST(test_list_nth);
  RUN_TEST(test_list_reverse);
  RUN_TEST(test_list_copy);
  RUN_TEST(test_list_concatenate);
  RUN_TEST(test_list_find);
  RUN_TEST(test_list_iterator);

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
