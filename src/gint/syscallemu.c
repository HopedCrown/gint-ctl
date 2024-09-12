#include <gint/display.h>
#include <gint/keyboard.h>
#include <gintctl/util.h>
#include <stdarg.h>

/* TODO: integrate in gint */
static long syscall(long nr, ...)
{
    va_list args;
    va_start(args, nr);
    long a0 = va_arg(args, long);
    long a1 = va_arg(args, long);
    long a2 = va_arg(args, long);
    long a3 = va_arg(args, long);
    long a4 = va_arg(args, long);
    long a5 = va_arg(args, long);
    long a6 = va_arg(args, long);
    va_end(args);

    long ret0, ret1;
    asm volatile (
        "mov.l  %2, r4\n\t"
        "mov.l  %3, r5\n\t"
        "mov.l  %4, r6\n\t"
        "mov.l  %5, r7\n\t"
        "mov.l  %6, r0\n\t"
        "mov.l  %7, r1\n\t"
        "mov.l  %8, r2\n\t"
        "mov    %9, r3\n\t"
        "trapa  #31\n\t"
        "mov    r0, %0\n\t"
        "mov    r1, %1\n\t"
        : "=r"(ret0), "=r"(ret1)
        : "m"(a0), "m"(a1), "m"(a2), "m"(a3), "m"(a4), "m"(a5), "m"(a6),
          "r"(nr)
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "cc", "memory");

    // TODO: How is the second retval passed exactly?
    return ret0;
}

extern int debug_syscall_count;
extern int debug_syscall_lastnr;

void gintctl_gint_syscallemu(void)
{
    int key = 0;

    while(key != KEY_EXIT) {
        dclear(C_WHITE);
        row_title("Syscall emulation");

        #if GINT_RENDER_MONO
        row_print(3, 1, "Count: %d", debug_syscall_count);
        row_print(4, 1, "Last #: %d", debug_syscall_lastnr);
        /// TODO: F1 icon
        #endif

        #if GINT_RENDER_RGB
        row_print(1, 1, "Syscall invocations: %d", debug_syscall_count);
        row_print(2, 1, "Last syscall number: %d", debug_syscall_lastnr);
        fkey_button(1, "SYSCALL");
        #endif

        dupdate();
        key = getkey().key;

        if(key == KEY_F1)
            syscall(42, 1, 2, 3);
    }
}
