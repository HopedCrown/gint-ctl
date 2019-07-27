#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/std/stdio.h>

#include <gintctl/mem.h>
#include <gintctl/util.h>

int c(int c)
{
	return (c >= 0x20 && c < 0x7f) ? c : '.';
}

/* gintctl_mem(): Memory browser */
void gintctl_mem(void)
{
	uint32_t base = 0x88010758;
	int key = 0, ascii = 0;

	#ifdef FX9860G
	extern font_t font_hexa;
	font_t const *old_font = dfont(&font_hexa);

	char header[12];
	char bytes[24];

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		uint32_t addr = base;
		uint8_t *mem = (void *)addr;

		for(int i = 0; i <= 8; i++)
		{
			if(!ascii)
			{
				sprintf(header, "%08X:", addr);
			}
			else
			{
				for(int k = 0; k < 8; k++)
					header[k] = c(mem[k]);
				header[8] = 0;
			}

			sprintf(bytes, "%02X%02X %02X%02X %02X%02X %02X%02X",
				mem[0], mem[1], mem[2], mem[3],
				mem[4], mem[5], mem[6], mem[7]
			);

			dtext( 5, 6 * i + 1, header, C_BLACK, C_NONE);
			dtext(45, 6 * i + 1, bytes,  C_BLACK, C_NONE);

			mem += 8;
			addr += 8;
		}

		extern image_t img_opt_mem;
		dsubimage(0, 56, &img_opt_mem, 0, 0, 128, 8, DIMAGE_NONE);

		if(ascii)
		{
			dsubimage(107, 56, &img_opt_mem, 107, 9, 21, 8, DIMAGE_NONE);
		}

		dupdate();
		key = getkey().key;

		if(key == KEY_UP) base -= 72;
		if(key == KEY_DOWN) base += 72;

		if(key == KEY_F6) ascii = !ascii;
	}

	dfont(old_font);
	#endif
}
