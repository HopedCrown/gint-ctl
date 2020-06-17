#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/std/stdio.h>
#include <gint/exc.h>

#include <gintctl/mem.h>
#include <gintctl/util.h>

/* Code of exception that occurs during a memory access */
static uint32_t exception = 0;
/* Exception-catching function */
GMAPPED static int catch_exc(uint32_t code)
{
	if(code == 0x040 || code == 0x0e0)
	{
		exception = code;
		gint_exc_skip(1);
		return 0;
	}
	return 1;
}

int line(uint8_t *mem, char *header, char *bytes, char *ascii, int n)
{
	/* First do a naive access to the first byte, and record possible
	   exceptions - TLB miss read and CPU read error.
	   I use a volatile asm statement so that the read can't be optimized
	   away or moved by the compiler. */
	exception = 0;
	gint_exc_catch(catch_exc);
	uint8_t z;
	__asm__ volatile(
		"mov.l	%1, %0"
		: "=r"(z)
		: "m"(*mem));
	gint_exc_catch(NULL);

	sprintf(header, "%08X:", (uint32_t)mem);

	if(exception == 0x040)
	{
		sprintf(bytes, "TLB error");
		*ascii = 0;
		return 1;
	}
	if(exception == 0x0e0)
	{
		sprintf(bytes, "Read error");
		*ascii = 0;
		return 1;
	}

	for(int k = 0; k < n; k++)
	{
		int c = mem[k];
		ascii[k] = (c >= 0x20 && c < 0x7f) ? c : '.';
	}
	ascii[n] = 0;

	for(int k = 0; 2 * k < n; k++)
	{
		sprintf(bytes + 5 * k, "%02X%02X ", mem[2*k], mem[2*k+1]);
	}
	return 0;
}

/* gintctl_mem(): Memory browser */
void gintctl_mem(void)
{
	uint32_t base = 0x88000000;
	int key = 0;

	#ifdef FX9860G
	extern font_t font_hexa;
	font_t const *old_font = dfont(&font_hexa);
	int view_ascii = 0;
	#endif

	char header[12];
	char bytes[48];
	char ascii[24];

	int size = 8;
	int lines = _(9,14);

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		uint32_t addr = base;
		uint8_t *mem = (void *)addr;

		for(int i = 0; i < lines; i++)
		{
			GUNUSED int status = line(mem,header,bytes,ascii,size);

			#ifdef FX9860G
			dtext( 5, 6*i + 1, view_ascii ? ascii : header,
				C_BLACK, C_NONE);
			dtext(45, 6*i + 1, bytes,  C_BLACK, C_NONE);
			#endif

			#ifdef FXCG50
			dtext(25,  26 + 12*i, header, C_BLACK, C_NONE);
			dtext(110, 26 + 12*i, bytes, status ? C_RED : C_BLACK,
				C_NONE);

			for(int k = size - 1; k >= 0; k--)
			{
				ascii[k+1] = 0;
				dtext(275 + 9*k, 26 + 12*i, ascii + k, C_BLACK,
					C_NONE);
			}
			#endif

			mem += size;
			addr += size;
		}

		#ifdef FX9860G
		extern bopti_image_t img_opt_mem;
		dsubimage(0, 56, &img_opt_mem, 0, 0, 128, 8, DIMAGE_NONE);

		if(view_ascii) dsubimage(23, 56, &img_opt_mem, 23, 9, 21, 8,
			DIMAGE_NONE);
		#endif

		#ifdef FXCG50
		row_title("Memory browser");
		fkey_button(1, "JUMP");
		fkey_action(3, "ROM");
		fkey_action(4, "RAM");
		fkey_action(5, "ILRAM");
		fkey_action(6, "ADDIN");
		#endif

		dupdate();
		key = getkey().key;

		if(key == KEY_UP) base   -= size * lines;
		if(key == KEY_DOWN) base += size * lines;

		#ifdef FX9860G
		if(key == KEY_F2) view_ascii = !view_ascii;
		#endif

		if(key == KEY_F3) base = 0x80000000;
		if(key == KEY_F4) base = 0x88000000;
		if(key == KEY_F5) base = 0xe5200000;
		if(key == KEY_F6) base = 0x00300000;
	}

	#ifdef FX9860G
	dfont(old_font);
	#endif
}
