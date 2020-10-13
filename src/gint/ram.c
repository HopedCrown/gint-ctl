#include <gint/std/stdio.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

/* Byte-based memory detection functions */

static int writable(uint8_t volatile *mem)
{
	int save = *mem;
	*mem = save ^ 0xff;

	/* Read the written value and restore the pointed byte */
	int measured = *mem;
	*mem = save;

	/* The address is writable iff we succeeded in storing ~save */
	return (measured == (save ^ 0xff));
}

static int same_location(uint8_t volatile *m1, uint8_t volatile *m2)
{
	uint8_t s1=*m1, s2=*m2;

	*m1 = s1 ^ 0xf0;
	int equal1 = (*m2 == *m1);

	*m1 = s1 ^ 0x0f;
	int equal2 = (*m2 == *m1);

	*m1 = s1 ^ 0xff;
	int equal3 = (*m2 == *m1);

	*m1 = s1;
	*m2 = s2;

	return equal1 && equal2 && equal3;
}

/* Longword-based memory detection functions */

static int writable_lword(uint8_t volatile *mem_8)
{
	uint32_t volatile *mem = (void *)mem_8;

	uint32_t save = *mem;
	*mem = save ^ 0xffffff00;

	uint32_t measured = *mem;
	*mem = save;

	return (measured == (save ^ 0xffffff00));
}

static int same_location_lword(uint8_t volatile *m1_8, uint8_t volatile *m2_8)
{
	uint32_t volatile *m1 = (void *)m1_8, *m2 = (void *)m2_8;
	uint32_t s1=*m1, s2=*m2;

	*m1 = s1 ^ 0xffff0000;
	int equal1 = (*m2 == *m1);

	*m1 = s1 ^ 0x0000ff00;
	int equal2 = (*m2 == *m1);

	*m1 = s1 ^ 0xffffff00;
	int equal3 = (*m2 == *m1);

	*m1 = s1;
	*m2 = s2;

	return equal1 && equal2 && equal3;
}

/* Region size detection */

static uint32_t region_size(uint8_t volatile *mem, int *reason, int use_lword)
{
	uint32_t size = 0;

	while(size < (1 << 20))
	{
		int x = use_lword
			? writable_lword(mem + size)
			: writable(mem + size);

		*reason = 1;
		if(!x) return size;

		if(size > 0)
		{
			int y = use_lword
				? same_location_lword(mem, mem+size)
				: same_location(mem, mem+size);
			*reason = 2;
			if(y) return size;
		}

		size += use_lword ? 4 : 1;
	}

	*reason = 3;
	return size;
}

/* gintctl_gint_ram(): Determine the size of some memory areas */
void gintctl_gint_ram(void)
{
	uint8_t *ILRAM = (void *)0xe5200000;
	uint8_t *XRAM  = (void *)0xe5007000;
	uint8_t *YRAM  = (void *)0xe5017000;
	uint8_t *PRAM0 = (void *)0xfe200000;
	uint8_t *XRAM0 = (void *)0xfe240000;
	uint8_t *YRAM0 = (void *)0xfe280000;
	uint8_t *PRAM1 = (void *)0xfe300000;
	uint8_t *XRAM1 = (void *)0xfe340000;
	uint8_t *YRAM1 = (void *)0xfe380000;

	/* Size of these sections */
	GUNUSED uint32_t IL=0, X=0, Y=0, P0=0, X0=0, Y0=0, P1=0, X1=0, Y1=0;
	/* Reason why the region stops (1=not writable, 2=wraps around) */
	GUNUSED int ILr=0, Xr=0, Yr=0, P0r=0, X0r=0, Y0r=0, P1r=0, X1r=0,Y1r=0;

	GUNUSED char const *reasons[] = {
		"Not tested yet",
		"%d bytes (not writable)",
		"%d bytes (wraps around)",
		"%d bytes",
	};

	int key = 0;
	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		row_print(1, 1, "On-chip memory");

		row_print(3, 2, "ILRAM: %d bytes", IL);
		row_print(4, 2, "XRAM:  %d bytes", X);
		row_print(5, 2, "YRAM:  %d bytes", Y);

		extern bopti_image_t img_opt_gint_ram;
		dimage(0, 56, &img_opt_gint_ram);
		#endif

		#ifdef FXCG50
		row_title("On-chip memory discovery");

		row_print(1, 1, "This program measures the size of on-chip");
		row_print(2, 1, "memory sections by checking how far it can "
			"write.");

		row_print(4, 2, "ILRAM:");
		row_print(5, 2, "XRAM:");
		row_print(6, 2, "YRAM:");
		row_print(7, 2, "PRAM0:");
		row_print(8, 2, "XRAM0:");
		row_print(9, 2, "YRAM0:");
		row_print(10,2, "PRAM1:");
		row_print(11,2, "XRAM1:");
		row_print(12,2, "YRAM1:");

		row_print(4, 10, "E5200000");
		row_print(5, 10, "E5007000");
		row_print(6, 10, "E5017000");
		row_print(7, 10, "FE200000");
		row_print(8, 10, "FE240000");
		row_print(9, 10, "FE280000");
		row_print(10,10, "FE300000");
		row_print(11,10, "FE340000");
		row_print(12,10, "FE380000");

		row_print(4, 21, reasons[ILr], IL);
		row_print(5, 21, reasons[Xr], X);
		row_print(6, 21, reasons[Yr], Y);
		row_print(7, 21, reasons[P0r], P0);
		row_print(8, 21, reasons[X0r], X0);
		row_print(9, 21, reasons[Y0r], Y0);
		row_print(10,21, reasons[P1r], P1);
		row_print(11,21, reasons[X1r], X1);
		row_print(12,21, reasons[Y1r], Y1);

		fkey_button(1, "ILRAM");
		fkey_button(2, "XYRAM");
		fkey_button(3, "DSP0");
		fkey_button(4, "DSP1");
		#endif

		dupdate();

		key = getkey().key;
		if(key == KEY_F1)
		{
			IL = region_size(ILRAM, &ILr, 0);
		}
		if(key == KEY_F2)
		{
			X = region_size(XRAM, &Xr, 0);
			Y = region_size(YRAM, &Yr, 0);
		}
		#ifdef FXCG50
		if(key == KEY_F3)
		{
			P0 = region_size(PRAM0, &P0r, 1);
			X0 = region_size(XRAM0, &X0r, 1);
			Y0 = region_size(YRAM0, &Y0r, 1);
		}
		if(key == KEY_F4)
		{
			P1 = region_size(PRAM1, &P1r, 1);
			X1 = region_size(XRAM1, &X1r, 1);
			Y1 = region_size(YRAM1, &Y1r, 1);
		}
		#endif
	}
}
