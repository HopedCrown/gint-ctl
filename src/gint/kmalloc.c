#include <gint/kmalloc.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/hardware.h>
#include <gint/rtc.h>
#include <gint/defs/util.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>

#include <string.h>
#include <stdlib.h>

//---
// Arena information
//---

static void draw_info(kmalloc_arena_t *arena)
{
	int expected_os_heap_kB = _(48, 128);

	#ifdef FX9860G
	dimage(0, 56, &img_opt_gint_kmalloc);
	row_title("Heap allocators");
	font_t const *old_font = dfont(&font_mini);
	dsubimage(107, 56, &img_opt_gint_kmalloc, 107, 9, 19, 8, DIMAGE_NONE);

	if(!strcmp(arena->name, "_os"))
	{
		dprint(1, 10, C_BLACK, "OS heap (guess: %d kB)", expected_os_heap_kB);
	}
	else
	{
		dprint(1, 10, C_BLACK, "gint heap: %08X-%08X",arena->start,arena->end);
		dprint(1, 17, C_BLACK, "Region size: %d", arena->end - arena->start);
		#ifdef GINT_KMALLOC_DEBUG
		dprint(1, 24, C_BLACK, "Sequence: %d blocks",
			kmallocdbg_sequence_length(arena));
		#endif
	}

	dfont(old_font);
	#endif

	#ifdef FXCG50
	row_print(1, 1, "Arena details:");
	fkey_action(6, "ARENA");

	if(!strcmp(arena->name, "_os"))
	{
		row_print(2, 2, "Type: Native OS heap");
		row_print(3, 2, "Expected OS heap size: %d kB", expected_os_heap_kB);
	}
	else
	{
		row_print(2, 2, "Type: gint's segregated list allocator");
		row_print(3, 2, "Region: %08X ... %08X (%d bytes)",
			arena->start, arena->end, arena->end - arena->start);

		/* kmalloc debugging information */
		row_print(5, 1, "Integrity checks:");
		#ifdef GINT_KMALLOC_DEBUG
		row_print(6, 2, "(SCR) Sequence covers region: %s",
			kmallocdbg_sequence_covers(arena) ? "true" : "false");
		row_print(7, 2, "(CUF) Coherent used flags: %s",
			kmallocdbg_sequence_coherent_used(arena) ? "true" : "false");
		row_print(8, 2, "(CFS) Correct footer size: %s",
			kmallocdbg_sequence_footer_size(arena) ? "true" : "false");
		row_print(9, 2, "(FBM) Free blocks merged: %s",
			kmallocdbg_sequence_merged_free(arena) ? "true" : "false");
		row_print(10, 2, "(FLL) Free linked list: %s",
			kmallocdbg_list_structure(arena) ? "true" : "false");
		row_print(11, 2, "(ICR) Index covers region: %s",
			kmallocdbg_index_covers(arena) ? "true" : "false");
		row_print(12, 2, "(ICS) Index class separation: %s",
			kmallocdbg_index_class_separation(arena) ? "true" : "false");
		row_print(13, 2, "Sequence length: %d",
			kmallocdbg_sequence_length(arena));
		#else
		row_print(6, 2, "kmalloc debug disabled!");
		#endif /* GINT_KMALLOC_DEBUG */
	}
	#endif
}

//---
// Manual allocation test
//---

#define MANUAL_COUNT 16

static int m_classes[6] = { 8, 12, 20, 24, 80, 260 };

static void m_clear(void *ptr[MANUAL_COUNT], uint16_t size[MANUAL_COUNT])
{
	for(int i = 0; i < MANUAL_COUNT; i++) if(ptr[i])
	{
		kfree(ptr[i]);
		ptr[i] = NULL;
		size[i] = 0;
	}
}

static void draw_manual(void *ptr[MANUAL_COUNT], uint16_t size[MANUAL_COUNT],
	int cursor)
{
	#ifdef FX9860G
	row_title("Manual allocation");
	font_t const *old_font = dfont(&font_mini);
	#endif


	for(int row = 0; row < 4; row++)
	for(int col = 0; col < 4; col++)
	{
		int x = _(20*col+12, 45*col+35);
		int y = _( 8*row+12, 18*row+40);

		int i = 4 * row + col;
		int fg = (cursor == i) ? C_WHITE : C_BLACK;

		if(cursor == i)
			drect(x-_(9,17), y-_(1,2), x+_(9,17), y+_(5,10), C_BLACK);

		if(!ptr[i]) dprint_opt(x,y,fg,C_NONE,DTEXT_CENTER,DTEXT_TOP, "NULL");
		else dprint_opt(x,y,fg,C_NONE,DTEXT_CENTER,DTEXT_TOP, "%d", size[i]);
	}

	for(int c = 0; c < 6; c++) dprint(_(88,275), _(15+6*c,row_y(c+2)), C_BLACK,
		"%c: %d", 'A'+c, m_classes[c]);

	#ifdef FXCG50
	row_print(1, 1, "Manual allocation test:");
	dprint(265, row_y(1), C_BLACK, "Allocate:");
	dprint(265, row_y(8), C_BLACK, "Free:");
	dprint(275, row_y(9), C_BLACK, "AC/ON");
	#endif

	#ifdef FX9860G
	dprint(88, 9, C_BLACK, "ON: 0");
	dfont(old_font);
	#endif
}

//---
// Fill test that exhausts arena with power-of-two blocks
//---

#define FILL_CLASSES 12
#define FILL_CLASS_BASE(i) (32768 >> (i))

static void run_fill(char const *arena, uint8_t classes[FILL_CLASSES])
{
	void *ptrs[256];
	int ptrs_size = 0;

	int size = FILL_CLASS_BASE(0);
	int size_class = 0;

	memset(classes, 0, FILL_CLASSES);

	while(size >= 16 && ptrs_size < 256)
	{
		void *ptr = kmalloc(size, arena);
		if(ptr)
		{
			ptrs[ptrs_size++] = ptr;
			classes[size_class]++;
		}
		else
		{
			size_class++;
			size = FILL_CLASS_BASE(size_class);
		}
	}

	for(int i = 0; i < ptrs_size; i++)
		kfree(ptrs[i]);
}

static void draw_fill(uint8_t classes[FILL_CLASSES])
{
	#ifdef FX9860G
	row_title("Heap filler");
	#endif

	font_t const *old_font = dfont(_(&font_mini, dfont_default()));
	int total = 0;

	#ifdef FXCG50
	row_print(1, 1, "Blocks allocated to fill, by size:");
	#endif

	for(int y = 0; y < 3; y++)
	for(int x = 0; x < 4 && 4*y+x < FILL_CLASSES; x++)
	{
		int i = 4*y+x;
		int x1 = _(28*x+1, row_x(11*x+2)), y1 = _(10+7*y, row_y(y+3));
		int size = FILL_CLASS_BASE(i);

		if(_(1,0) && size >= 1024)
			dprint(x1, y1, C_BLACK, "%dk:%d", size >> 10, classes[i]);
		else
			dprint(x1, y1, C_BLACK, "%d:%d", size, classes[i]);

		total += size * classes[i];
	}

	row_print(_(6,7), _(1,2), "Total size: %d", total);
	row_print(_(7,13), 1, "EXE: Run the fill test");

	dfont(old_font);
}

//---
// Mass operation test
//---

struct mass_test
{
	int done;

	void *fillers[16];
	int fill_size;
	int operations;
	int space_bound;

	int allocs;
	int frees;
	int smaller_reallocs;
	int larger_reallocs;

	uint32_t volume;
	int failed;
};

static void draw_mass(struct mass_test *test)
{
	#ifdef FX9860G
	row_title("Random operations");
	font_t const *old_font = dfont(&font_mini);
	dprint(1, 9, C_BLACK, "Filled %d (all blocks < %d)",
		test->fill_size, test->space_bound);
	dprint(1, 16, C_BLACK, "malloc(): %d - free(): %d",
		test->allocs, test->frees);
	dprint(1, 23, C_BLACK, "Larger realloc(): %d",
		test->larger_reallocs);
	dprint(1, 29, C_BLACK, "Smaller realloc(): %d",
		test->smaller_reallocs);
	dprint(1, 36, C_BLACK, "Volume: %u", test->volume);
	dprint(1, 43, C_BLACK, "Failed allocs: %d", test->failed);
	dprint(1, 50, C_BLACK, "EXE: Run the mass test");
	dfont(old_font);
	#endif

	#ifdef FXCG50
	if(!test->done)
	{
		row_print(1, 1, "Mass allocation in a small space");
		return;
	}

	row_print(1, 1, "Mass allocation in a small space:");
	row_print(2, 2, "Fill size: %d", test->fill_size);
	row_print(3, 2, "Space had no block of %d bytes", test->space_bound);

	row_print(5, 1, "Summary of operations:");
	row_print(6, 2, "malloc(): %d", test->allocs);
	row_print(7, 2, "free(): %d", test->frees);
	row_print(8, 2, "realloc(): %d smaller, %d larger",
		test->smaller_reallocs, test->larger_reallocs);

	row_print(10, 1, "Total volume allocated: %u bytes", test->volume);
	row_print(11, 1, "Failed allocations: %d", test->failed);
	row_print(13, 1, "Press EXE to perform the test.");
	#endif
}

static void run_mass(struct mass_test *test, char const *arena)
{
	/* Initialize test data */
	memset(test, 0, sizeof *test);

	/* Fill the whole heap except for some 2048 bytes */
	int fill_size = 65536;
	for(int i = 0; i < 16 && fill_size > 2048; )
	{
		test->fillers[i] = kmalloc(fill_size, arena);
		if(test->fillers[i]) test->fill_size += fill_size, i++;
		else fill_size >>= 1;
	}
	test->space_bound = fill_size;

	/* Perform a large set of random operations with 256 pointers */
	void *ptr[256];
	uint8_t size[256];
	for(int i = 0; i < 256; i++) ptr[i] = NULL, size[i] = 0;

	srand(rtc_ticks());
	for(int o = 0; o < _(8000,30000); o++)
	{
		int i = rand() % 256;
		int desired_size = rand() % 128;
		if(desired_size > 64) desired_size = 0;

		if(desired_size == 0 && ptr[i] == NULL)
		{
			continue;
		}
		else if(desired_size == 0 && ptr[i])
		{
			kfree(ptr[i]);
			ptr[i] = NULL;
			size[i] = 0;
			test->frees++;
		}
		else if(desired_size > 0 && ptr[i] == NULL)
		{
			ptr[i] = kmalloc(desired_size, arena);
			test->allocs++;
			if(ptr[i]) size[i] = desired_size, test->volume += desired_size;
			else test->failed++;
		}
		else
		{
			bool smaller = (desired_size <= size[i]);
			void *p = krealloc(ptr[i], desired_size);
			if(smaller) test->smaller_reallocs++;
			else test->larger_reallocs++;
			if(p) ptr[i]=p, size[i]=desired_size, test->volume+=desired_size;
			else test->failed++;
		}
	}

	/* Release the remaining pointers and filler blocks */
	for(int i = 0; i < 256; i++) kfree(ptr[i]);
	for(int i = 0; i < 16; i++) kfree(test->fillers[i]);

	test->done = 1;
}

//---
// Statistics
//---

static void draw_stats(kmalloc_arena_t *arena)
{
	struct kmalloc_stats *s = &arena->stats;
	kmalloc_gint_stats_t *S = kmalloc_get_gint_stats(arena);

	#ifdef FX9860G
	row_title("Statistics");
	font_t const *old_font = dfont(&font_mini);

	dprint(1, 9, C_BLACK, "Live blocks: %d (peak %d)",
		s->live_blocks, s->peak_live_blocks);
	dprint(1, 16, C_BLACK, "Volume: %d (%d blocks)",
		s->total_volume, s->total_blocks);
	dprint(1, 23, C_BLACK, "Failures: %d",
		s->total_failures);

	if(S) {
		dprint(1, 30, C_BLACK, "Free memory: %d",
			S->free_memory);
		dprint(1, 36, C_BLACK, "Used: %d (peak %d)",
			S->used_memory, S->peak_used_memory);
		dprint(1, 43, C_BLACK, "Failures: %d exh. %d frag.",
			S->exhaustion_failures, S->fragmentation_failures);
		dprint(1, 50, C_BLACK, "reallocs: %d exp. %d rel.",
			S->expanding_reallocs, S->relocating_reallocs);
	}

	dfont(old_font);
	#endif

	#ifdef FXCG50
	row_print(1, 1, "General arena statistics:");
	row_print(2, 2, "Live blocks: %d (peak: %d)",
		s->live_blocks, s->peak_live_blocks);
	row_print(3, 2, "Total volume: %d bytes (%d blocks)",
		s->total_volume, s->total_blocks);
	row_print(4, 2, "Allocation failures: %d",
		s->total_failures);

	if(!S) return;

	row_print(6, 1, "Statistics from gint's allocator:");
	row_print(7, 2, "Free memory: %d bytes",
		S->free_memory);
	row_print(8, 2, "Used memory: %d bytes (peak: %d)",
		S->used_memory, S->peak_used_memory);
	row_print(9, 2, "Failures: %d exhaustion, %d frag.",
		S->exhaustion_failures, S->fragmentation_failures);
	row_print(10, 2, "reallocs: %d expanding, %d relocating",
		S->expanding_reallocs, S->relocating_reallocs);
	#endif
}

//---
// Main routine
//---

/* Draw integrity tests in a discrete fashion on all screens */
static void draw_integrity(kmalloc_arena_t *arena)
{
	if(!strcmp(arena->name, "_os")) return;

	#ifdef GINT_KMALLOC_DEBUG
	bool SCR = kmallocdbg_sequence_covers(arena);
	bool CUF = kmallocdbg_sequence_coherent_used(arena);
	bool CFS = kmallocdbg_sequence_footer_size(arena);
	bool FBM = kmallocdbg_sequence_merged_free(arena);
	bool FLL = kmallocdbg_list_structure(arena);
	bool ICR = kmallocdbg_index_covers(arena);
	bool ICS = kmallocdbg_index_class_separation(arena);

	bool integrity[] = { SCR, CUF, CFS, FBM, FLL, ICR, ICS };
	char const *labels[] = { "SCR", "CUF", "CFS", "FBM", "FLL", "ICR", "ICS" };

	font_t const *old_font = dfont(_(&font_mini, dfont_default()));

	int w, h, max_width = 0;
	for(int i = 0; i < 7; i++)
	{
		dsize(labels[i], NULL, &w, &h);
		max_width = max(max_width, w);
	}

	for(int i = 0; i < 7; i++)
	{
		int x = DWIDTH - _(9,20), y = _(1+7*i, row_y(i+1));
		int fg = integrity[i] ? _(C_BLACK, C_WHITE) : _(C_WHITE, C_WHITE);
		int bg = integrity[i] ? _(C_NONE, C_RGB(6,27,6))
			: _(C_BLACK, C_RGB(31,6,6));

		drect(x-max_width/2, y-_(1,2), x+max_width/2+_(1,3), y+h+_(0,1), bg);
		dtext_opt(x+_(1,2), y, fg, C_NONE, DTEXT_CENTER, DTEXT_TOP, labels[i]);
	}

	dfont(old_font);
	#endif /* GINT_KMALLOC_DEBUG */
}

/* gintctl_gint_kmalloc(): Dynamic memory allocator */
void gintctl_gint_kmalloc(void)
{
	kmalloc_arena_t *arena = kmalloc_get_arena("_uram");
	int tab=0, key=0;

	/* Data for the manual allocation test */
	void *m_ptr[MANUAL_COUNT];
	uint16_t m_size[MANUAL_COUNT];
	int m_cursor = 0;
	for(int i = 0; i < MANUAL_COUNT; i++) m_ptr[i] = NULL, m_size[i] = 0;

	/* Data for the fill test */
	uint8_t fill_classes[FILL_CLASSES] = { 0 };

	/* Data for the mass operation test */
	struct mass_test mass_test = { .done = 0 };

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		dimage(0, 56, &img_opt_gint_kmalloc);
		font_t const *old_font = dfont(&font_mini);
		dprint_opt(DWIDTH-2, 54, C_BLACK, C_NONE, DTEXT_RIGHT, DTEXT_BOTTOM,
			"%s", arena->name);
		dfont(old_font);
		#endif

		if(tab == 0) draw_info(arena);
		if(tab == 1) draw_manual(m_ptr, m_size, m_cursor);
		if(tab == 2) draw_fill(fill_classes);
		if(tab == 3) draw_mass(&mass_test);
		if(tab == 4) draw_stats(arena);
		draw_integrity(arena);

		#ifdef FXCG50
		row_title("Integrity and stress tests for heap allocators");
		fkey_menu(1, "INFO");
		fkey_menu(2, "MANUAL");
		fkey_menu(3, "FILL");
		fkey_menu(4, "MASS");
		fkey_menu(5, "STATS");

		dprint_opt(DWIDTH-10, row_y(13), C_BLACK, C_NONE,
			DTEXT_RIGHT, DTEXT_TOP, "Arena: %s", arena->name);
		#endif

		dupdate();
		key = getkey().key;

		if(key == KEY_F1) tab = 0;
		if(key == KEY_F2) tab = 1;
		if(key == KEY_F3) tab = 2, m_cursor = 0;
		if(key == KEY_F4) tab = 3;
		if(key == KEY_F5) tab = 4;

		/* Actions on the info tab */
		if(tab == 0 && key == KEY_F6)
		{
			/* When changing arena, free all blocks currently held */
			m_clear(m_ptr, m_size);
			/* Reset fill info */
			for(int i = 0; i < FILL_CLASSES; i++) fill_classes[i] = 0;
			/* Reset mass test info */
			mass_test.done = 0;

			if(!strcmp(arena->name, "_os"))
				arena = kmalloc_get_arena("_uram");
			else
				arena = kmalloc_get_arena("_os");
		}

		/* Actions on the manual tab */
		if(tab == 1)
		{
			if(key == KEY_UP && m_cursor >= 4) m_cursor -= 4;
			if(key == KEY_DOWN && m_cursor < 12) m_cursor += 4;
			if(key == KEY_LEFT && (m_cursor % 4) > 0) m_cursor--;
			if(key == KEY_RIGHT && (m_cursor % 4) < 3) m_cursor++;

			int k[6] = { KEY_XOT, KEY_LOG, KEY_LN, KEY_SIN, KEY_COS, KEY_TAN };
			for(int i = 0; i < 6; i++) if(key == k[i])
			{
				if(!m_ptr[m_cursor])
				{
					m_ptr[m_cursor] = kmalloc(m_classes[i], arena->name);
					if(m_ptr[m_cursor]) m_size[m_cursor] = m_classes[i];
				}
				else
				{
					void *p = krealloc(m_ptr[m_cursor], m_classes[i]);
					if(p)
					{
						m_ptr[m_cursor] = p;
						m_size[m_cursor] = m_classes[i];
					}
				}
			}

			if(key == KEY_ACON)
			{
				kfree(m_ptr[m_cursor]);
				m_ptr[m_cursor] = NULL;
				m_size[m_cursor] = 0;
			}
		}

		/* Actions on the fill tab */
		if(tab == 2 && key == KEY_EXE) run_fill(arena->name, fill_classes);

		/* Actions on the mass test tab */
		if(tab == 3 && key == KEY_EXE) run_mass(&mass_test, arena->name);
	}

	m_clear(m_ptr, m_size);
}
