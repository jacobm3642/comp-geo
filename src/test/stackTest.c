#include <check.h>
#include <stdio.h>
#include <strings.h>

#include "dataTypes.h"
#include "stackallocator.h"

extern stackAllocator mainMem;

START_TEST(allocator_frame_test)
{
        stackAllocator s;
        init_allocator(&s, 100);

        ck_assert(allocate_block(&s, 101) == -1);
        for (int i = 0; i < 10; i++) {
                ck_assert(allocate_block(&s, 1) == i);
        }
        ck_assert(allocate_block(&s, 1) == -1);

        free_allocator(&s);
}END_TEST

START_TEST(store_test)
{
        stackAllocator s;
        init_allocator(&s, 1000);
        dataFrame *dfPtr = &s.allocatedFrames[allocate_block(&s, sizeof(i32))];
        i32 *i32ptr = (i32 *)dfPtr->sptr;

        *i32ptr = 100;
        ck_assert(*i32ptr == 100);


        free_allocator(&s);
}END_TEST

START_TEST(dealloc_test)
{
        stackAllocator s;
        init_allocator(&s, 100);

        for (int i = 0; i < 10; i++) {
                ck_assert(allocate_block(&s, 1) == i);
        }
        ck_assert(s.frameCount == 10);
        for (int i =0; i < 10; i++) {
                free_block(&s, i);
        }
        ck_assert(s.offset == 0);

        ck_assert(allocate_block(&s, 100) == 0);

        free_block(&s, 0);

        size_t b0 = allocate_block(&s, 50);
        allocate_block(&s, 50);

        free_block(&s, b0);

        ck_assert(allocate_block(&s, 50) == -1);

        free_allocator(&s);
}END_TEST

START_TEST(ptr_dealloc_test)
{
        stackAllocator s;
        init_allocator(&s, 100);
        vec2 *v1 = s.allocatedFrames[allocate_block(&s, sizeof(vec2))].sptr;
        vec2 *v2 = s.allocatedFrames[allocate_block(&s, sizeof(vec2))].sptr;
        free_block_ptr(&s, v1);
        free_block_ptr(&s, &v2->y);

        ck_assert(s.frameCount == 0);

}END_TEST;

START_TEST(global_memory_pool_test)
{
        i32 index = allocate_block(&mainMem, 10);
        ck_assert(index != -1);
        free_block(&mainMem, index);
}END_TEST

Suite *stack_test_suite()
{
        Suite *s;
        TCase *tc;

        s = suite_create("stack allocator tests");
        tc = tcase_create("Core");

        // add tests here
        tcase_add_test(tc, allocator_frame_test);
        tcase_add_test(tc, store_test);
        tcase_add_test(tc, dealloc_test);
        tcase_add_test(tc, ptr_dealloc_test);
        tcase_add_test(tc, global_memory_pool_test);

        suite_add_tcase(s, tc);

        return s;
}

vec2 stack_allocator_tests()
{
        int failed = 0;
        int run = 0;
        Suite *s;
        SRunner *runner;

        printf("running stack allocator tests\n");

        s = stack_test_suite();
        runner = srunner_create(s);

        srunner_run_all(runner, CK_NORMAL);

        run = srunner_ntests_run(runner);
        failed = srunner_ntests_failed(runner);

        srunner_free(runner);
        printf("\n%d of %d Passed\n", run-failed, run);
        vec2 out = {run-failed, run};
        return  out;
}
