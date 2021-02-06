#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/std/stdio.h>
#include <gint/exc.h>

#include <gintctl/mem.h>
#include <gintctl/util.h>

/* Code of exception that occurs during a memory access */
static uint32_t exception = 0;
/* Exception-catching function */
static int catch_exc(uint32_t code)
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

	/* Read single bytes when possible, but longwords when in SPU memory */
	for(int k = 0; k < n; k++)
	{
		uint32_t addr = (uint32_t)mem;
		int c = 0x11;

		/* XRAM, YRAM, PRAM */
		if((addr & 0xffc00000) == 0xfe000000)
		{
			uint32_t l = *(uint32_t *)(addr & ~3);
			c = (l << (addr & 3) * 8) >> 24;
		}
		else
		{
			c = mem[k];
		}

		ascii[k] = (c >= 0x20 && c < 0x7f) ? c : '.';
	}
	ascii[n] = 0;

	for(int k = 0; 2 * k < n; k++)
	{
		sprintf(bytes + 5 * k, "%02X%02X ", mem[2*k], mem[2*k+1]);
	}
	return 0;
}

void draw_input(int x, int y, char *text, int cursor_pos)
{
	int w, h;
	int next = text[cursor_pos];
	text[cursor_pos] = 0;
	dsize(text, NULL, &w, &h);
	text[cursor_pos] = next;

	dtext(x, y, C_BLACK, text);
	dline(x+w, y, x+w, y+h-1, C_BLACK);
}

/* gintctl_mem(): Memory browser */
void gintctl_mem(void)
{
	uint32_t base = 0x88000000;
	key_event_t ev;
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

	char input[9];
	int input_pos = -1;
	int input_len = -1;
	int input_keys[16] = {
		KEY_0,   KEY_1,    KEY_2,    KEY_3,
		KEY_4,   KEY_5,    KEY_6,    KEY_7,
		KEY_8,   KEY_9,    KEY_XOT,  KEY_LOG,
		KEY_LN,  KEY_SIN,  KEY_COS,  KEY_TAN,
	};

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		uint32_t addr = base;
		uint8_t *mem = (void *)addr;

		for(int i = 0; i < lines; i++)
		{
			GUNUSED int status = line(mem,header,bytes,ascii,size);

			#ifdef FX9860G
			dtext( 5, 6*i + 1, C_BLACK, view_ascii?ascii:header);
			dtext(45, 6*i + 1, C_BLACK, bytes);
			#endif

			#ifdef FXCG50
			dtext(25,  26 + 12*i, C_BLACK, header);
			dtext(110, 26 + 12*i, status ? C_RED : C_BLACK, bytes);

			for(int k = size - 1; k >= 0; k--)
			{
				ascii[k+1] = 0;
				dtext(275 + 9*k, 26 + 12*i, C_BLACK, ascii+k);
			}
			#endif

			mem += size;
			addr += size;
		}

		#ifdef FX9860G
		if(input_pos < 0)
		{
			extern bopti_image_t img_opt_mem;
			dsubimage(0, 56, &img_opt_mem, 0, 0, 128, 8,
				DIMAGE_NONE);
			if(view_ascii) dsubimage(23, 56, &img_opt_mem, 23, 9,
				21, 8, DIMAGE_NONE);
		}
		else
		{
			extern font_t font_mini;
			font_t const *old_font = dfont(&font_mini);

			dtext(1, 57, C_BLACK, "Go to:");
			draw_input(24, 57, input, input_pos);

			dfont(old_font);
		}
		#endif

		#ifdef FXCG50
		row_title("Memory browser");
		if(input_pos < 0)
		{
			fkey_button(1, "JUMP");
			fkey_action(3, "ROM");
			fkey_action(4, "RAM");
			fkey_action(5, "ILRAM");
			fkey_action(6, "ADDIN");
		}
		else
		{
			dtext(4, 210, C_BLACK, "Go to:");
			draw_input(52, 210, input, input_pos);
		}
		#endif

		dupdate();
		ev = getkey();
		key = ev.key;

		int move_speed = 1;
		if(ev.shift || keydown(KEY_SHIFT)) move_speed = 8;

		if(key == KEY_UP)   base -= move_speed * size * lines;
		if(key == KEY_DOWN) base += move_speed * size * lines;

		if(key == KEY_F1 && input_pos < 0)
		{
			input[0] = 0;
			input_pos = 0;
			input_len = 0;
		}
		if(key == KEY_EXIT && input_pos >= 0)
		{
			input_pos = -1;
			input_len = -1;
			/* Don't quit the memory viewer */
			key = 0;
		}
		if(key == KEY_EXE && input_pos >= 0)
		{
			/* Parse string into hexa */
			uint32_t target = 0;
			for(int k = 0; k < input_len; k++)
			{
				target <<= 4;
				if(input[k] <= '9') target += (input[k] - '0');
				else target += (input[k] - 'A' + 10);

				base = target & ~7;
			}

			input_pos = -1;
			input_len = -1;
		}

		for(int i = 0; i < 16; i++)
		if(key == input_keys[i] && input_pos >= 0 && input_len < 8)
		{
			/* Insert at input_pos, shift everything else right */
			for(int k = 8; k >= input_pos; k--)
				input[k + 1] = input[k];
			input[input_pos++] = i + '0' + 7 * (i > 9);
			input_len++;
		}
		if(key == KEY_DEL && input_pos > 0)
		{
			/* Shift everything after input_pos left one place */
			for(int k = input_pos - 1; k < 8; k++)
				input[k] = input[k + 1];
			input_pos--;
			input_len--;
		}
		if(key == KEY_LEFT  && input_pos > 0) input_pos--;
		if(key == KEY_RIGHT && input_pos < input_len) input_pos++;

		#ifdef FX9860G
		if(key == KEY_F2 && input_pos < 0) view_ascii = !view_ascii;
		#endif

		if(key == KEY_F3 && input_pos < 0) base = 0x80000000;
		if(key == KEY_F4 && input_pos < 0) base = 0x88000000;
		if(key == KEY_F5 && input_pos < 0) base = 0xe5200000;
		if(key == KEY_F6 && input_pos < 0) base = 0x00300000;
	}

	#ifdef FX9860G
	dfont(old_font);
	#endif
}
