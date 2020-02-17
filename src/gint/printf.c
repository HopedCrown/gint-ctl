#include <gint/std/stdio.h>
#include <gint/std/string.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <stdarg.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

static int passed = 0;
static int total = 0;

static struct {
	char const *format;
	char const *expected;
} fails[10];

static void check(char const *expected, char const *format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[128];
	vsnprintf(buffer, 128, format, args);

	va_end(args);

	total++;

	if(!strcmp(buffer, expected))
	{
		passed++;
	}
	else if(total - passed <= 10)
	{
		fails[total - passed - 1].format = format;
		fails[total - passed - 1].expected = expected;
	}
}

static void check_all(void)
{
	passed = 0;
	total = 0;

	/* Base cases with length and precision */
	check("-849",       "%d", -849);
	check("  78372",    "%7i", 78372);
	check("65536",      "%3d", 65536);
	check("  0017",     "%6.4d", 17);
	check(" -1876",     "%6.3d", -1876);

	/* Sign */
	check("+15",        "%+i", 15);
	check("  78372",    "% 7i", 78372);
	check(" 65536",     "% d", 65536);

	/* Alignment */
	check("0017  ",     "%-6.4d", 17);
	check("+0017 ",     "%-+6.4i", 17);

	/* Bases */
	check("3255",       "%d", 0xcb7);
	check("cb7",        "%x", 0xcb7);
	check("CB7",        "%X", 0xcb7);
	check("6267",       "%o", 0xcb7);

	/* Argument size */
	check("10000000000", "%lld", 10000000000ll);
	check("123456789ab", "%llx", 0x123456789abull);

	/* Alternative prefixes */
	check("0x7b",       "%#x", 0x7b);
	check("0X7B",       "%#X", 0x7b);
	check("0377",       "%#o", 255);

	/* Pointers */
	check("0xa44b0000", "%p", (void *)0xa44b0000);

	/* Characters and strings */
	check("HellWrld!",  "%s",     "HellWrld!");
	check("Hello   ",   "%-8.5s", "Hello, World!");
	check("d",          "%c",     100);
	check("     #",     "%6c",    '#');
}

/* gintctl_gint_printf(): printf() function */
void gintctl_gint_printf(void)
{
	int key = 0;
	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		row_print(1, 1, "printf() tests");
		row_print(3, 1, "passed: %d/%d", passed, total);
		#endif

		#ifdef FXCG50
		row_title("Unit tests for the printf() family");

		row_print(1, 1, "Passed: %d/%d", passed, total);

		for(int i = 0; i < 10 && i <= total - passed - 1; i++)
		{
			int y = 22 + 14 * (i + 2);
			dtext(6,  y, fails[i].format,   C_BLACK, C_NONE);
			dtext(86, y, fails[i].expected, C_BLACK, C_NONE);
		}

		fkey_button(1, "RUN");
		#endif

		dupdate();

		key = getkey().key;
		if(key == KEY_F1) check_all();
	}
}
