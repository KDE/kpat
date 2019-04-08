/*
 * fcs_soft_suspend_test.c
 * Copyright (C) 2019 Shlomi Fish <shlomif@cpan.org>
 *
 * Distributed under terms of the Expat license.
 */

#include <freecell-solver/fcs_user.h>
#include <stdio.h>

int main(void)
{
    printf("%d\n", (int)FCS_STATE_SOFT_SUSPEND_PROCESS);

    return 0;
}
