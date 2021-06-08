#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/hardware.h>
#include <gint/dma.h>
#include <gint/mmu.h>

#include <gintctl/perf.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>

#include <libprof.h>

#include <string.h>

extern void memory_read(volatile uint8_t *area, uint32_t size);
extern void memory_write(volatile uint8_t *area, uint32_t size);
extern void memory_dsp_xram_memset(volatile uint8_t *area, uint32_t size);
extern void memory_dsp_yram_memset(volatile uint8_t *area, uint32_t size);
extern void memory_dsp_xyram_memcpy(volatile uint8_t *dst,
	volatile uint8_t *src, uint32_t size);

GILRAM GALIGNED(32) static char ilram_buffer[0x800];
GXRAM  GALIGNED(32) static char xram_buffer[0x800];
GYRAM  GALIGNED(32) static char yram_buffer[0x800];

struct results
{
	void *address;
	uint32_t size;
	int rounds;

	/* In microseconds for the whole area */
	uint32_t read_C_u8_time;
	uint32_t write_C_u8_time;
	uint32_t read_u8_time;
	uint32_t write_u8_time;
	uint32_t memcpy_time;
	uint32_t memset_time;
	uint32_t dma_memcpy_time;
	uint32_t dma_memset_time;
	uint32_t dsp_xram_memset_time;
	uint32_t dsp_yram_memset_time;
	uint32_t dsp_xyram_memcpy_time;

	/* In kbytes/second */
	uint32_t read_C_u8_speed;
	uint32_t write_C_u8_speed;
	uint32_t read_u8_speed;
	uint32_t write_u8_speed;
	uint32_t memcpy_speed;
	uint32_t memset_speed;
	uint32_t dma_memcpy_speed;
	uint32_t dma_memset_speed;
	uint32_t dsp_xram_memset_speed;
	uint32_t dsp_yram_memset_speed;
	uint32_t dsp_xyram_memcpy_speed;
};

static void test(struct results *r, void *address, uint32_t size, int rounds)
{
	volatile uint8_t *area = address;
	volatile uint8_t x;

	r->address = address;
	r->size = size;
	r->rounds = rounds;

	/* Defaults for conditional tests */
	r->dsp_xram_memset_time = 1;
	r->dsp_yram_memset_time = 1;
	r->dsp_xyram_memcpy_time = 1;

	r->read_C_u8_time = prof_exec({
		for(int i = 0; i < rounds; i++)
		{
			for(uint index = 0; index < size; index++)
				x = area[index];
		}
	});

	r->write_C_u8_time = prof_exec({
		for(int i = 0; i < rounds; i++)
		{
			for(uint index = 0; index < size; index++)
				area[index] = x;
		}
	});

	r->read_u8_time = prof_exec({
		for(int i = 0; i < rounds; i++)
			memory_read(area, size);
	});

	r->write_u8_time = prof_exec({
		for(int i = 0; i < rounds; i++)
			memory_write(area, size);
	});

	r->memset_time = prof_exec({
		for(int i = 0; i < rounds; i++)
			memset(address, 0, size);
	});

	r->memcpy_time = 2 * prof_exec({
		for(int i = 0; i < rounds; i++)
			memcpy(address + size / 2, address, size / 2);
	});

	r->dma_memset_time = prof_exec({
		for(int i = 0; i < rounds; i++)
			if(isSH4()) dma_memset(address, 0, size);
	});

	r->dma_memcpy_time = 2 * prof_exec({
		for(int i = 0; i < rounds; i++)
			if(isSH4()) dma_memcpy(address + size / 2, address, size / 2);
	});

	if(address == &xram_buffer)
	{
		/* Since the buffers are small, repeat 16 times */
		r->dsp_xram_memset_time = prof_exec({
			for(int i = 0; i < rounds; i++)
				if(isSH4()) memory_dsp_xram_memset(address, size);
		});
	}
	if(address == &yram_buffer)
	{
		r->dsp_yram_memset_time = prof_exec({
			for(int i = 0; i < rounds; i++)
				if(isSH4()) memory_dsp_yram_memset(address, size);
		});
	}
	if(address == &xram_buffer)
	{
		void *x = xram_buffer;
		void *y = yram_buffer;

		/* Since the buffers are small, repeat 16 times */
		r->dsp_xyram_memcpy_time = prof_exec({
			for(int i = 0; i < rounds; i++)
				if(isSH4()) memory_dsp_xyram_memcpy(y, x, size);
		});
	}
	if(address == &yram_buffer)
	{
		void *x = xram_buffer;
		void *y = yram_buffer;

		r->dsp_xyram_memcpy_time = prof_exec({
			for(int i = 0; i < rounds; i++)
				if(isSH4()) memory_dsp_xyram_memcpy(x, y, size);
		});
	}

	/* Convert from us/(size bytes) to kb/(1 second) */
	uint64_t factor = size * 1000 * rounds;
	r->read_C_u8_speed        = factor / r->read_C_u8_time;
	r->write_C_u8_speed       = factor / r->write_C_u8_time;
	r->read_u8_speed          = factor / r->read_u8_time;
	r->write_u8_speed         = factor / r->write_u8_time;
	r->memcpy_speed           = factor / r->memcpy_time;
	r->memset_speed           = factor / r->memset_time;
	r->dma_memcpy_speed       = factor / r->dma_memcpy_time;
	r->dma_memset_speed       = factor / r->dma_memset_time;
	r->dsp_xram_memset_speed  = factor / r->dsp_xram_memset_time;
	r->dsp_yram_memset_speed  = factor / r->dsp_yram_memset_time;
	r->dsp_xyram_memcpy_speed = factor / r->dsp_xyram_memcpy_time;
}

static void results_line(int row, uint32_t time, uint32_t speed)
{
	int y = _(8+6*row, row_y(row));
	dprint_opt(_(80,260), y, C_BLACK, C_NONE, DTEXT_RIGHT, DTEXT_TOP,
		"%d us", time);
	dprint_opt(_(125,370), y, C_BLACK, C_NONE, DTEXT_RIGHT, DTEXT_TOP,
		_("%3.1j MB/s", "%3.3j MB/s"), _(speed/100, speed));
}

/* gintctl_perf_memory(): Memory primitives and reading/writing speed */
void gintctl_perf_memory(void)
{
	int key = 0;
	struct results r = { 0 };

	/* Get the physical VRAM address */
	void *vram_address = gint_vram;
	#ifdef FX9860G
	uint32_t virt_page = (uint32_t)vram_address & 0xfffff000;
	uint32_t phys_page = 0x80000000 + mmu_translate(virt_page, NULL);
	vram_address = (void *)phys_page + (vram_address - (void *)virt_page);
	#endif

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);
		row_title("Memory access speed");
		font_t const *old_font = dfont(_(&font_mini, dfont_default()));

		#ifdef FX9860G
		/* Due to less space, focus on the non-trivial methods */
		dprint(1, 14, C_BLACK, "gint memcpy:");
		dprint(1, 20, C_BLACK, "gint memset:");
		if(isSH4()) {
			dprint(1, 26, C_BLACK, "dma_memcpy:");
			dprint(1, 32, C_BLACK, "dma_memset:");

			if(r.address == &xram_buffer)
				dprint(1, 38, C_BLACK, "DSP memset:");
			if(r.address == &yram_buffer)
				dprint(1, 38, C_BLACK, "DSP memset:");
			if(r.address == &xram_buffer || r.address == &yram_buffer)
				dprint(1, 44, C_BLACK, "DSP memcpy:");
		}

		if(!r.address) dprint(1, 8, C_BLACK, "No test yet");
		else
		{
			dprint(1, 8, C_BLACK, "Area: %08X (%d B, %d round%s)",
				(uint32_t)r.address, r.size, r.rounds, (r.rounds>1)?"s":"");
			results_line(1, r.memcpy_time,     r.memcpy_speed);
			results_line(2, r.memset_time,     r.memset_speed);
			if(isSH4()) {
				results_line(3, r.dma_memcpy_time, r.dma_memcpy_speed);
				results_line(4, r.dma_memset_time, r.dma_memset_speed);
				if(r.address == &xram_buffer)
					results_line(5, r.dsp_xram_memset_time,
						r.dsp_xram_memset_speed);
				if(r.address == &yram_buffer)
					results_line(5, r.dsp_yram_memset_time,
						r.dsp_yram_memset_speed);
				if(r.address==&xram_buffer || r.address==&yram_buffer)
					results_line(6, r.dsp_xyram_memcpy_time,
						r.dsp_xyram_memcpy_speed);
			}
		}

		if(isSH3())
			dimage(0, 56, &img_opt_perf_memory_sh3);
		else
			dimage(0, 56, &img_opt_perf_memory);
		#endif

		#ifdef FXCG50
		row_print( 3, 1, "Naive C-loop u8 read:");
		row_print( 4, 1, "Naive C-loop u8 write:");
		row_print( 5, 1, "Rolled asm u8 read:");
		row_print( 6, 1, "Rolled asm u8 write:");
		row_print( 7, 1, "gint's memcpy():");
		row_print( 8, 1, "gint's memset():");
		row_print( 9, 1, "gint's dma_memcpy():");
		row_print(10, 1, "gint's dma_memset():");

		if(r.address == &xram_buffer)
			row_print(11, 1, "DSP XRAM memset():");
		if(r.address == &yram_buffer)
			row_print(11, 1, "DSP YRAM memset():");
		if(r.address == &xram_buffer || r.address == &yram_buffer)
			row_print(12, 1, "DSP XRAM->YRAM memcpy():");

		if(!r.address) row_print(1, 1, "No test yet");
		else
		{
			row_print(1, 1, "Results for area %08x (%d bytes, %d "
				"round%s)", (uint32_t)r.address, r.size,
				r.rounds, (r.rounds > 1) ? "s" : "");
			results_line(3, r.read_C_u8_time,  r.read_C_u8_speed);
			results_line(4, r.write_C_u8_time, r.write_C_u8_speed);
			results_line(5, r.read_u8_time,    r.read_u8_speed);
			results_line(6, r.write_u8_time,   r.write_u8_speed);
			results_line(7, r.memcpy_time,     r.memcpy_speed);
			results_line(8, r.memset_time,     r.memset_speed);
			results_line(9, r.dma_memcpy_time, r.dma_memcpy_speed);
			results_line(10,r.dma_memset_time, r.dma_memset_speed);

			if(r.address == &xram_buffer)
				results_line(11, r.dsp_xram_memset_time,
					r.dsp_xram_memset_speed);
			if(r.address == &yram_buffer)
				results_line(11, r.dsp_yram_memset_time,
					r.dsp_yram_memset_speed);
			if(r.address==&xram_buffer || r.address==&yram_buffer)
				results_line(12, r.dsp_xyram_memcpy_time,
					r.dsp_xyram_memcpy_speed);
		}

		fkey_button(1, "RAM");
		fkey_button(2, "ILRAM");
		fkey_button(3, "XRAM");
		fkey_button(4, "YRAM");
		#endif

		dfont(old_font);
		dupdate();
		key = getkey().key;

		if(key == KEY_F1) test(&r, vram_address, _(0x400,0x8000), _(32,1));
		if(isSH4()) {
			if(key == KEY_F2) test(&r, &ilram_buffer, 0x800, 64);
			if(key == KEY_F3) test(&r, &xram_buffer, 0x800, 64);
			if(key == KEY_F4) test(&r, &yram_buffer, 0x800, 64);
		}
	}
}
