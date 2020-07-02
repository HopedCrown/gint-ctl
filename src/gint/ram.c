#include <gint/std/stdio.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

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

static uint32_t region_size(uint8_t volatile *mem, int *reason)
{
	uint32_t size = 0;

	while(size <= 16384)
	{
		if(!writable(mem+size))
		{
			*reason = 1;
			break;
		}
		if(size > 0 && same_location(mem, mem+size))
		{
			*reason = 2;
			break;
		}
		size++;
	}

	return size;
}

/* gintctl_gint_ram(): Determine the size of some memory areas */
void gintctl_gint_ram(void)
{
	uint8_t *ILRAM = (void *)0xe5200000;
	uint8_t *XRAM  = (void *)0xe5007000;
	uint8_t *YRAM  = (void *)0xe5017000;
	uint8_t *MERAM = (void *)0xe8080000;
	uint8_t *XRAM0 = (void *)0xfe240000;

	/* Size of these sections */
	uint32_t IL=0, X=0, Y=0, ME=0, X0=0;
	/* Reason why the region stops (1=not writable, 2=wraps around) */
	int ILr=0, Xr=0, Yr=0, MEr=0, X0r=0;

	GUNUSED char const *reasons[] = {
		"Not tested yet",
		"%d bytes (not writable)",
		"%d bytes (wraps around)",
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
		row_print(6, 2, "MERAM: %d bytes", ME);

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
		row_print(7, 2, "MERAM:");
		row_print(8, 2, "XRAM0:");

		row_print(4, 10, "E5200000");
		row_print(5, 10, "E5007000");
		row_print(6, 10, "E5017000");
		row_print(7, 10, "E8080000");
		row_print(8, 10, "FE240000");

		row_print(4, 21, reasons[ILr], IL);
		row_print(5, 21, reasons[Xr], X);
		row_print(6, 21, reasons[Yr], Y);
		row_print(7, 21, reasons[MEr], ME);
		row_print(8, 21, reasons[X0r], X0);

		fkey_button(1, "ILRAM");
		fkey_button(2, "XRAM");
		fkey_button(3, "YRAM");
		fkey_button(4, "MERAM");
		fkey_button(5, "XRAM0");
		#endif

		dupdate();

		key = getkey().key;
		if(key == KEY_F1) IL = region_size(ILRAM, &ILr);
		if(key == KEY_F2) X  = region_size(XRAM, &Xr);
		if(key == KEY_F3) Y  = region_size(YRAM, &Yr);
		if(key == KEY_F4) ME = region_size(MERAM, &MEr);
		if(key == KEY_F5) X0 = region_size(XRAM0, &X0r);
	}
}
