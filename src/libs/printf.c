#include <gint/std/stdio.h>
#include <gint/std/string.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#include <stdarg.h>
#include <stdint.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#define SCROLL_HEIGHT _(8,12)

struct printf_test {
	char const *format;

	/* Type, argument, string, answer, passed are set by macros below */
	enum { TYPE_I32, TYPE_U32, TYPE_STR, TYPE_PTR, TYPE_U64 } type;
	union {
		int i32;
		uint32_t u32;
		char const *str;
		void *ptr;
		uint64_t u64;
	};
	char const *argument_as_string;

	char answer[32];
	int passed;
	char const *solution;
};

#define I32(i) TYPE_I32, { .i32 = i }, #i, "", 0
#define U32(u) TYPE_U32, { .u32 = u }, #u, "", 0
#define STR(s) TYPE_STR, { .str = s }, #s, "", 0
#define PTR(p) TYPE_PTR, { .ptr = (void *)p }, #p, "", 0
#define U64(u) TYPE_U64, { .u64 = u }, #u, "", 0

static struct printf_test tests[] = {
	/* Base cases with length and precision */
	{ "%d",       I32(-849),              "-849"           },
	{ "%7i",      I32(78372),             "  78372"        },
	{ "%3d",      I32(65536),             "65536"          },
	{ "%6.4d",    I32(17),                "  0017"         },
	{ "%6.3d",    I32(-1876),             " -1876"         },
	/* Sign */
	{ "%+i",      I32(15),                "+15"            },
	{ "% 7i",     I32(78372),             "  78372"        },
	{ "% d",      I32(65536),             " 65536"         },
	/* Alignment */
	{ "%-6.4d",   I32(17),                "0017  "         },
	{ "%-+6.4i",  I32(17),                "+0017 "         },
	/* Bases */
	{ "%d",       I32(0xcb7),             "3255"           },
	{ "%x",       U32(0xcb7),             "cb7"            },
	{ "%X",       U32(0xcb7),             "CB7"            },
	{ "%o",       U32(0xcb7),             "6267"           },
	/* Argument size */
	{ "%lld",     U64(10000000000ll),     "10000000000"    },
	{ "%llx",     U64(0x123456789abull),  "123456789ab"    },
	/* Alternative prefixes */
	{ "%#x",      U32(0x7b),              "0x7b"           },
	{ "%#X",      U32(0x7b),              "0X7B"           },
	{ "%#o",      U32(255),               "0377"           },
	/* Pointers */
	{ "%p",       PTR(0xa44b0000),        "0xa44b0000"     },
	/* Characters and strings */
	{ "%s",       STR("HellWrld!"),       "HellWrld!"      },
	{ "%-8.5s",   STR("Hello, World!"),   "Hello   "       },
	{ "%c",       I32(100),               "d"              },
	{ "%6c",      I32('#'),               "     #",        },
	/* NULL terminator */
	{ NULL }
};

static void run_tests(struct printf_test *tests)
{
	for(int i = 0; tests[i].format; i++)
	{
		struct printf_test *t = &tests[i];

		#define run(TYPE, field) case TYPE:			\
			snprintf(t->answer, 32, t->format, t->field);	\
			break;

		switch(t->type)
		{
		run(TYPE_I32, i32)
		run(TYPE_U32, u32)
		run(TYPE_STR, str)
		run(TYPE_PTR, ptr)
		run(TYPE_U64, u64)
		}

		t->passed = !strcmp(t->answer, t->solution);
	}
}

static void draw(struct printf_test const *tests, int offset)
{
	int passed=0, total=0;
	for(int i = 0; tests[i].format; i++)
	{
		passed += (tests[i].passed);
		total++;
	}

	dclear(C_WHITE);

	#ifdef FX9860G
	extern font_t font_hexa;
	font_t *old_font = dfont(&font_hexa);

	dprint( 1, 0, C_BLACK, C_NONE, "ID");
	dprint(13, 0, C_BLACK, C_NONE, "Format");
	dprint(43, 0, C_BLACK, C_NONE, "Output");
	dprint(91, 0, C_BLACK, C_NONE, "Valid");

	for(int i = 0; i < SCROLL_HEIGHT; i++)
	{
		struct printf_test const *t = &tests[offset+i];
		int y = (i+1) * 6;
		dprint( 1, y, C_BLACK, C_NONE, "%d", offset+i+1);
		dprint(13, y, C_BLACK, C_NONE, "%s", t->format);
		dprint(43, y, C_BLACK, C_NONE, "%s", t->answer);
		dprint(91, y, C_BLACK, C_NONE, "%s", t->passed?"Ok":"Err");
	}

	dfont(old_font);
	row_print(8, 1, "Passed %d of %d.", passed, total);
	#endif

	#ifdef FXCG50
	row_title("libc: Formatted printing functions");

	row_print(1,  2, "ID");
	row_print(1,  5, "Format");
	row_print(1, 13, "Argument");
	row_print(1, 29, "Answer");

	for(int i = 0; i < SCROLL_HEIGHT; i++)
	{
		struct printf_test const *t = &tests[offset+i];
		row_print(i+2,  2, "%d", offset+i+1);
		row_print(i+2,  5, "%s", t->format);
		row_print(i+2, 13, "%s", t->argument_as_string);

		int fg = t->passed ? C_RGB(0,31,0) : C_RGB(31,0,0);
		row_print_color(i+2, 29, fg, C_NONE, "%s", t->solution);
	}

	row_print(14, 1, "Passed: %d/%d", passed, total);
	#endif

	if(offset > 0) triangle_up(_(7,38));
	if(offset < total -  SCROLL_HEIGHT) triangle_down(_(49,192));

	dupdate();
}

/* gintctl_libs_printf(): printf() function */
void gintctl_libs_printf(void)
{
	key_event_t ev;
	int key=0, total=0, offset=0;
	for(int i = 0; tests[i].format; i++) total++;

	run_tests(tests);

	while(key != KEY_EXIT)
	{
		draw(tests, offset);
		key = (ev = getkey()).key;

		if(key == KEY_UP && offset > 0) offset--;
		if(key == KEY_UP && ev.shift) offset = 0;
		if(key == KEY_DOWN && offset < total - SCROLL_HEIGHT) offset++;
		if(key == KEY_DOWN && ev.shift) offset = total - SCROLL_HEIGHT;
	}
}
