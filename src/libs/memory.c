//---
//	gintctl:libs:memory - Automated core memory function tests
//
//	These tests are meant to check all size/alignment scenarii for the core
//	memory functions. Two intervals of sizes are tested:
//	* 12..15 bytes, expected to be handled naively
//	* 192..195 bytes, expected to trigger alignment-related optimizations
//	For each of these intervals, all alignments possibilities are tested:
//	* 4n + { 0,1,2,3 } for the source address
//	* 4n + { 0,1,2,3 } for destination address
//	Also, the source and destination regions are made to overlap to allow
//	non-trivial memmove() cases to be checked.
//---

#include <gint/defs/types.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/exc.h>

#include <gintctl/libs.h>
#include <gintctl/util.h>

#include <string.h>

/* Source buffer, used as a data source when copying */
GALIGNED(4) static uint8_t *src;
/* Destination buffer, used as destination when copying or clearing */
GALIGNED(4) static uint8_t *dst;
/* System buffer, used to reproduce the behavior on the system and compare */
GALIGNED(4) static uint8_t *sys;
/* Temporary buffer, used by the naive memmove() */
GALIGNED(4) static uint8_t *tmp;

/* Fill buffer with non-zero and position-sensitive data */
static void fill(uint8_t *buf, int start)
{
	for(int i = 0; i < 256; i++) buf[i] = start+i;
}
/* Clear buffer */
static void clear(uint8_t *buf)
{
	for(int i = 0; i < 256; i++) buf[i] = 0;
}
/* Check buffer equality (returns zero on equal) */
static int cmp(uint8_t *lft, uint8_t *rgt)
{
	for(int i = 0; i < 256; i++) if(lft[i] != rgt[i]) return 1;
	return 0;
}

/* Code of exception that occurs during a memory access */
static uint32_t exception = 0;
/* Exception-catching function */
static int catch_exc(uint32_t code)
{
	if(code == 0x100 || code == 0x0e0)
	{
		exception = code;
		gint_exc_skip(1);
		return 0;
	}
	return 1;
}

//---
//	Naive functions (reference behaviour)
//---

static void *naive_memcpy(void *_dst, void const *_src, size_t len)
{
	uint8_t *dst = _dst;
	uint8_t const *src = _src;

	while(len--) *dst++ = *src++;
	return _dst;
}
static void *naive_memset(void *_dst, int byte, size_t len)
{
	uint8_t *dst = _dst;

	while(len--) *dst++ = byte;
	return _dst;
}
static void *naive_memmove(void *_dst, void const *_src, size_t len)
{
	naive_memcpy(tmp, _src, len);
	naive_memcpy(_dst, tmp, len);
	return _dst;
}
static int naive_memcmp(void const *_s1, void const *_s2, size_t len)
{
	uint8_t const *s1 = _s1, *s2 = _s2;

	for(size_t i = 0; i < len; i++)
	{
		if(s1[i] != s2[i]) return s1[i] - s2[i];
	}
	return 0;
}

//---
//	Testing functions
//---

static int test_memcpy(int off_dst, int off_src, size_t len)
{
	clear(dst);
	clear(sys);

	memcpy(dst + off_dst, src + off_src, len);
	naive_memcpy(sys + off_dst, src + off_src, len);

	return cmp(dst, sys);
}

static int test_memset(int off_dst, GUNUSED int off_src, size_t len)
{
	fill(dst, 0);
	fill(sys, 0);

	memset(dst + off_dst, 0, len);
	naive_memset(sys + off_dst, 0, len);

	return cmp(dst, sys);
}

static int test_memmove(int off_dst, int off_src, size_t len)
{
	fill(dst, 0);
	fill(sys, 0);

	memmove(dst + off_dst, dst + off_src, len);
	naive_memmove(sys + off_dst, sys + off_src, len);

	return cmp(dst, sys);
}

static int test_memcmp(int off_dst, int off_src, size_t len)
{
	/* Create data that matches at the provided offsets */
	fill(dst, -off_dst);
	fill(sys, -off_src);

	/* Check equality */
	if(memcmp(dst + off_dst, sys + off_src, len) !=
	   naive_memcmp(dst + off_dst, sys + off_src, len)) return 1;

	/* Check that changing any single byte results in a mismatch */
	for(size_t i = 0; i < len; i++)
	{
		dst[off_dst + i] ^= 0xff;
		if(memcmp(dst + off_dst, sys + off_src, len) !=
		   naive_memcmp(dst + off_dst, sys + off_src, len)) return 1;
		dst[off_dst + i] ^= 0xff;
	}

	return 0;
}

//---
//	Automated tests
//---

/* exc(): Wrapper that accounts for exceptions */
static int exc(int (*func)(int of_dst, int off_src, size_t len), int off_dst,
	int off_src, size_t len)
{
	exception = 0;
	gint_exc_catch(catch_exc);

	int ret = func(off_dst, off_src, len);

	gint_exc_catch(NULL);

	return exception ? (int)exception : ret;
}

/* test(): Check core memory functions in various size/alignment scenarii

   The function to test takes three arguments: two buffer offsets and the size
   of the operation. Bounds need no be checked. It must return 0 in case of
   success and non-zero in case of failure.

   @func   Function to test, will be called with various sizes and alignments
   @count  If non-null, set to number of tests performed
   Returns the number of failed tests; thus, non-zero indicates failure. */
static void test(int (*func)(int off_dst, int off_src, size_t len),
	uint8_t *results)
{
	int current_test = 0;

	/* For each source and destination alignment... */
	for(int src_al = 0; src_al < 4; src_al++)
	for(int dst_al = 0; dst_al < 4; dst_al++, current_test += 16)
	/* For each "alignment" of operation size... */
	for(int len_al = 0; len_al < 4; len_al++)
	{
		int b = current_test + len_al;

		/* Try a small size first */
		results[b]    = exc(func, 96+dst_al, 96+src_al, 12+len_al);
		/* Then a medium region without overlapping */
		results[b+4]  = exc(func, 4+dst_al, 128+src_al, 92+len_al);
		/* A large region with left-right overlapping */
		results[b+8]  = exc(func, 64+dst_al, 96 +src_al, 128+len_al);
		/* A large region with right-left overlapping */
		results[b+12] = exc(func, 96+dst_al, 64 +src_al, 128+len_al);
	}
}

//---
//	Main function
//---

#define dcenter(x, y, ...) \
	dprint_opt(x, y, C_BLACK, C_NONE, DTEXT_CENTER, DTEXT_TOP, __VA_ARGS__)
#define dright(x, y, ...) \
	dprint_opt(x, y, C_BLACK, C_NONE, DTEXT_RIGHT, DTEXT_TOP, __VA_ARGS__)

/* gintctl_libs_memory(): Core memory functions */
void gintctl_libs_memory(void)
{
	dclear(C_WHITE);
	dtext_opt(DWIDTH / 2, DHEIGHT / 2, C_BLACK, C_NONE, DTEXT_CENTER,
		DTEXT_MIDDLE, "Testing...");
	dupdate();

	uint8_t buf_src[256];
	uint8_t buf_dst[256];
	uint8_t buf_sys[256];
	uint8_t buf_tmp[256];

	src = buf_src;
	dst = buf_dst;
	sys = buf_sys;
	tmp = buf_tmp;

	GUNUSED int key = 0, tab = 0;

	uint8_t results[4][256];
	char const *names[4] = { "memcpy", "memset", "memmove", "memcmp" };
	int scores[4] = { 0 };

	test(test_memcpy,  results[0]);
	test(test_memset,  results[1]);
	test(test_memmove, results[2]);
	test(test_memcmp,  results[3]);

	for(int i = 0; i < 4; i++)
	for(int t = 0; t < 256; t++)
	{
		scores[i] += !results[i][t];
	}

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		row_title("Core memory functions");

		for(int i = 0; i < 4; i++)
			row_print(i+3, 1, "%-7s: %d/256", names[i], scores[i]);
		#endif

		#ifdef FXCG50
		row_title("libc: Core memory functions");

		dprint(10, 19, C_BLACK, "%s", names[tab]);
		dprint(10, 31, C_BLACK, "%d/256", scores[tab]);

		dcenter(234, 19, "Source align and destination align");
		for(int i = 0; i < 16; i++)
			dprint(84+19*i, 31, C_BLACK, "%d%d", i >> 2, i & 3);

		dprint(10,  44, C_BLACK, "Small");
		dprint(10,  84, C_BLACK, "Large");
		dprint(10, 124, C_BLACK, "Inter1");
		dprint(10, 164, C_BLACK, "Inter2");
		for(int i = 0; i < 16; i++)
			dright(78, 44+10*i, "%d", i & 3);

		for(int y = 0; y < 16; y++)
		for(int x = 0; x < 16; x++)
		{
			int x1=82+19*x, y1=43+10*y;

			int fg = C_BLACK;
			if(results[tab][16 * x + y] == 1) fg = C_RED;
			if(results[tab][16 * x + y] == 0) fg = C_GREEN;

			drect_border(x1,y1,x1+19,y1+10, fg, 1, C_BLACK);
		}

		fkey_menu(1, "MEMCPY");
		fkey_menu(2, "MEMSET");
		fkey_menu(3, "MEMMOVE");
		fkey_menu(4, "MEMCMP");
		#endif

		dupdate();
		key = getkey().key;

		if(key == KEY_F1) tab = 0;
		if(key == KEY_F2) tab = 1;
		if(key == KEY_F3) tab = 2;
		if(key == KEY_F4) tab = 3;
	}
}
