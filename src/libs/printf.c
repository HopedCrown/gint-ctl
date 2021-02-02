#include <gint/std/stdio.h>
#include <gint/std/string.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#include <stdarg.h>
#include <stdint.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#define NAN __builtin_nan("")
#define INFINITY __builtin_inf()

#define SCROLL_HEIGHT _(8,12)

struct printf_test {
	char const *format;

	/* Argument information is set by macros below */
	enum {
		TYPE_I32, TYPE_U32, TYPE_STR, TYPE_PTR, TYPE_U64, TYPE_DBL
	} type;
	union {
		int i32;
		uint32_t u32;
		char const *str;
		void *ptr;
		uint64_t u64;
		double dbl;
	};
	char const *argument_as_string;
	char const *solution;
};

#define I32(i) TYPE_I32, { .i32 = i }, #i
#define U32(u) TYPE_U32, { .u32 = u }, #u
#define STR(s) TYPE_STR, { .str = s }, #s
#define PTR(p) TYPE_PTR, { .ptr = (void *)p }, #p
#define U64(u) TYPE_U64, { .u64 = u }, #u
#define DBL(d) TYPE_DBL, { .dbl = d }, #d

static struct printf_test const tests[] = {
	/* Base cases with length and precision */
	{ "%d",       I32(-849),              "-849"           },
	{ "%7i",      I32(78372),             "  78372"        },
	{ "%3d",      I32(65536),             "65536"          },
	{ "%6.4d",    I32(17),                "  0017"         },
	{ "%6.3d",    I32(-1876),             " -1876"         },
	{ "%.0d",     I32(0),                 ""               },
	{ "%.d",      I32(0),                 ""               },
	/* Sign */
	{ "%+i",      I32(15),                "+15"            },
	{ "% 7i",     I32(78372),             "  78372"        },
	{ "% d",      I32(65536),             " 65536"         },
	/* Alignment */
	{ "%08d",     I32(-839),              "-0000839"       },
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
	/* Floating-point special values */
	{ "%f",       DBL(NAN),               "nan"            },
	{ "%3.5F",    DBL(NAN),               "NAN"            },
	{ "%+2F",     DBL(-INFINITY),         "-INF"           },
	{ "%10G",     DBL(INFINITY),          "INF"            },
	{ "%+g",      DBL(INFINITY),          "inf"            },
	{ "%7.3e",    DBL(-INFINITY),         "-inf"           },
	{ "%8E",      DBL(NAN),               "NAN"            },
	/* Simple floating-point cases */
	{ "%f",       DBL(13e3),              "13000.000000"   },
	{ "%.3f",     DBL(-13e3),             "-13000.000"     },
	{ "%.F",      DBL(13e3),              "13000"          },
	{ "%f",       DBL(-12.42),            "-12.420000"     },
	{ "%.0f",     DBL(-12.42),            "-12",           },
	{ "%.1f",     DBL(12.42),             "12.4",          },
	{ "%.8F",     DBL(12.42),             "12.42000000"    },
	{ "%F",       DBL(0.0312),            "0.031200"       },
	{ "%.4f",     DBL(0.0312),            "0.0312"         },
	{ "%.2f",     DBL(-0.0312),           "-0.03",         },
	{ "%.0f",     DBL(0.0312),            "0",             },
	/* Floating-point rounding */
	{ "%.f",      DBL(1.75),              "2",             },
	{ "%.1f",     DBL(1.75),              "1.8",           },
	{ "%.3F",     DBL(0.0625),            "0.063",         },
	{ "%.1F",     DBL(-99.99),            "-100.0"         },
	{ "%.2F",     DBL(12999.992),         "12999.99"       },
	{ "%.2f",     DBL(12999.995),         "13000.00"       },
	/* General options with floating-point */
	{ "%09.3F",   DBL(123.4567),          "00123.457"      },
	{ "%05.0f",   DBL(99.99),             "00100"          },
	{ "%+11f",    DBL(0.0035678),         "  +0.003568"    },
	{ "%- 11F",   DBL(0.0035678),         " 0.003568  "    },
	/* Simple exponent cases */
	{ "%e",       DBL(3.876),             "3.876000e+00"   },
	{ "%E",       DBL(-38473.34254),      "-3.847334E+04"  },
	{ "%.2e",     DBL(187.2),             "1.87e+02"       },
	{ "%.1e",     DBL(-18.27),            "-1.8e+01"       },
	{ "%e",       DBL(1e-10),             "1.000000e-10"   },
	{ "%E",       DBL(3.873e180),         "3.873000E+180"  },
	{ "%.e",      DBL(0.0005),            "5e-04"          },
	{ "%.E",      DBL(128.37),            "1E+02"          },
	{ "%.5e",     DBL(912.3),             "9.12300e+02"    },
	/* Exponent with rounding and general options */
	{ "%11.3e",   DBL(12.499),            "  1.250e+01"    },
	{ "% -11.E",  DBL(358.7),             " 4E+02     "    },
	{ "%+e",      DBL(14.99999),          "+1.499999e+01"  },
	{ "%+e",      DBL(14.999999),         "+1.500000e+01"  },
	/* Exponent sizes */
	{ "%.2e",     DBL(1e10),              "1.00e+10"       },
	{ "%.2e",     DBL(1e100),             "1.00e+100"      },
	{ "%.2e",     DBL(1e-10),             "1.00e-10"       },
	{ "%.2e",     DBL(1e-100),            "1.00e-100"      },
	/* Format selection and trailing zero elimination in %g */
	{ "%g",       DBL(124),               "124"            },
	{ "%g",       DBL(2.3),               "2.3"            },
	{ "%g",       DBL(0.0001),            "0.0001"         },
	{ "%G",       DBL(0.00001),           "1E-05"          },
	{ "%g",       DBL(834e-93),           "8.34e-91"       },
	{ "%g",       DBL(32842914732),       "3.28429e+10"    },
	{ "%g",       DBL(123456),            "123456"         },
	{ "%G",       DBL(1234567),           "1.23457E+06"    },
	{ "%G",       DBL(123000),            "123000"         },
	/* Rounding and general options in %g */
	{ "%.3g",     DBL(1278),              "1.28e+03"       },
	{ "%+.8g",    DBL(1.23e5),            "+123000"        },
	{ "%- 12.8g", DBL(123000.01),         " 123000.01  "   },
	{ "%0.8g",    DBL(123000.001),        "123000"         },
	{ "%08.8g",   DBL(123000.001),        "00123000"       },
	{ "%g",       DBL(1.234567),          "1.23457"        },
	{ "%.1g",     DBL(1.8),               "2"              },
	/* Edge cases of significant digit count in %g */
	{ "%.4g",     DBL(999.93),            "999.9"          },
	{ "%.4g",     DBL(999.97),            "1000"           },
	{ "%.5g",     DBL(999.97),            "999.97"         },
	{ "%.8G",     DBL(999.97),            "999.97"         },
	{ "%.4g",     DBL(1.0001),            "1",             },
	/* Elimination of trailing zeros in %g */
	{ "%.3g",     DBL(3002),              "3e+03"          },
	{ "%.5g",     DBL(0.00000034),        "3.4e-07"        },
	{ "%.6g",     DBL(999.9997),          "1000"           },
	/* NULL terminator */
	{ NULL }
};

static void run_tests(struct printf_test const *tests, char answers[][16],
	int *passed)
{
	for(int i = 0; tests[i].format; i++)
	{
		struct printf_test const *t = &tests[i];

		#define run(TYPE, field) case TYPE:			\
			snprintf(answers[i], 16, t->format, t->field);	\
			break;

		switch(t->type)
		{
		run(TYPE_I32, i32)
		run(TYPE_U32, u32)
		run(TYPE_STR, str)
		run(TYPE_PTR, ptr)
		run(TYPE_U64, u64)
		run(TYPE_DBL, dbl)
		}

		passed[i] = !strcmp(answers[i], t->solution);
	}
}

static void draw(struct printf_test const *tests, char answers[][16],
	int *passed, int offset)
{
	int total_passed=0, total=0;
	for(int i = 0; tests[i].format; i++)
	{
		total_passed += passed[i];
		total++;
	}

	dclear(C_WHITE);

	#ifdef FX9860G
	extern font_t font_hexa;
	font_t const *old_font = dfont(&font_hexa);

	dprint( 1, 0, C_BLACK, "ID");
	dprint(13, 0, C_BLACK, "Format");
	dprint(43, 0, C_BLACK, "Output");
	dprint(91, 0, C_BLACK, "Valid");

	for(int i = 0; i < SCROLL_HEIGHT; i++)
	{
		struct printf_test const *t = &tests[offset+i];
		int y = (i+1) * 6;
		dprint( 1, y, C_BLACK, "%d", offset+i+1);
		dprint(13, y, C_BLACK, "%s", t->format);
		dprint(43, y, C_BLACK, "%s", answers[offset+i][0]
			? answers[offset+i] : "<empty>");
		dprint(91, y, C_BLACK, "%s", passed[offset+i]?"Ok":"Err");
	}

	dfont(old_font);
	row_print(8, 1, "Passed %d of %d.", total_passed, total);
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

		int fg = passed[offset+i] ? C_RGB(0,31,0) : C_RGB(31,0,0);
		row_print_color(i+2, 29, fg, C_NONE, "%s", answers[offset+i][0]
			? answers[offset+i] : "<empty>");
	}

	row_print(14, 1, "Passed: %d/%d", total_passed, total);
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

	int test_count = sizeof tests / sizeof tests[0];
	char answers[test_count][16];
	int passed[test_count];
	run_tests(tests, answers, passed);

	while(key != KEY_EXIT)
	{
		draw(tests, answers, passed, offset);
		key = (ev = getkey()).key;

		if(key == KEY_UP && offset > 0) offset--;
		if(key == KEY_UP && ev.shift) offset = 0;
		if(key == KEY_DOWN && offset < total - SCROLL_HEIGHT) offset++;
		if(key == KEY_DOWN && ev.shift) offset = total - SCROLL_HEIGHT;
	}
}
