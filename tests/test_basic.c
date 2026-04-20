#include "project.h"

#include <criterion/criterion.h>

Test(project, add_basic)
{
    int result = project_add(2, 2);
    cr_assert_eq(result, 4, "Expected 2 + 2 to equal 4, but got %d", result);
}

Test(project, add_negative)
{
    int result = project_add(-5, 5);
    cr_assert_eq(result, 0, "Expected -5 + 5 to equal 0, but got %d", result);
}
