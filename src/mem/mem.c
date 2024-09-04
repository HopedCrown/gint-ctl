#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/exc.h>

#include <gintctl/mem.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>

#include <gintctl/widgets/gscreen.h>
#include <justui/jpainted.h>
#include <justui/jinput.h>

#include <stdio.h>

struct view {
	uint32_t base;
	bool ascii;
	int lines;
};

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
	__asm__ volatile("mov.l	%1, %0" : "=r"(z) : "m"(*mem));
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
		sprintf(bytes + 5 * k, "%02X%02X ", mem[2*k], mem[2*k+1]);

	return 0;
}

static void paint_mem(int x, int y, struct view *v)
{
	char header[12], bytes[48], ascii[24];
	uint32_t addr = v->base;
	uint8_t *mem = (void *)addr;

	for(int i = 0; i < v->lines; i++, mem += 8, addr += 8)
	{
		GUNUSED int status = line(mem, header, bytes, ascii, 8);

		#if GINT_RENDER_MONO
		font_t const *old_font = dfont(&font_hexa);
		dtext(x,      y + 6*i, C_BLACK, v->ascii ? ascii : header);
		dtext(x + 40, y + 6*i, C_BLACK, bytes);
		dfont(old_font);
		#endif

		#if GINT_RENDER_RGB
		dtext(x,      y + 12*i, C_BLACK, header);
		dtext(x + 85, y + 12*i, status ? C_RED : C_BLACK, bytes);

		for(int k = 7; k >= 0; k--)
		{
			ascii[k+1] = 0;
			dtext(x + 250 + 9*k, y + 12*i, C_BLACK, ascii+k);
		}
		#endif
	}
}

/* gintctl_mem(): Memory browser */
void gintctl_mem(void)
{
	struct view v = { .base = 0x88000000, .ascii = false, .lines = _(9,14) };

	jscene *scene = jscene_create_fullscreen(NULL);
	gscreen *s = gscreen_create2(NULL, &img_opt_mem,
		"Memory browser", "@JUMP;;#ROM;#RAM;#ILRAM;#ADDIN", scene);
	jwidget *tab = jwidget_create(NULL);
	jpainted *mem = jpainted_create(paint_mem, &v, _(115,321), _(53,167), tab);
	jinput *input = jinput_create("Go to:" _(," "), 12, tab);

	jwidget_set_margin(mem, _(0,6), 0, _(0,6), 0);
	jwidget_set_margin(input, 0, 0, 0, _(1,4));
	jwidget_set_stretch(input, 1, 0, false);
	jwidget_set_visible(input, false);
	jlayout_set_vbox(tab)->spacing = _(3,4);
	gscreen_add_tab(s, tab, NULL);

	int key = 0;
	while(key != KEY_EXIT)
	{
		bool input_focus = (jscene_focused_widget(scene) == input);
		jevent e = jscene_run(scene);

		if(e.type == JSCENE_PAINT)
		{
			dclear(C_WHITE);
			jscene_render(scene);
			dupdate();
		}
		if(e.type == JINPUT_VALIDATED)
		{
			/* Parse string into hexa */
			uint32_t target = 0;
			char const *str = jinput_value(input);

			for(int k = 0; k < str[k]; k++)
			{
				target <<= 4;
				if(str[k] <= '9') target += (str[k] - '0');
				else target += ((str[k]|0x20) - 'a' + 10);
			}
			v.base = target & ~7;
		}
		if(e.type == JINPUT_VALIDATED || e.type == JINPUT_CANCELED)
		{
			jwidget_set_visible(input, false);
			gscreen_set_tab_fkeys_visible(s, 0, true);
			jscene_set_focused_widget(scene, NULL);
		}

		if(e.type != JSCENE_KEY || e.key.type == KEYEV_UP) continue;
		key = e.key.key;

		int move_speed = (e.key.shift ? 8 : 1);
		if(key == KEY_UP)   v.base -= move_speed * 8 * v.lines;
		if(key == KEY_DOWN) v.base += move_speed * 8 * v.lines;

		if(key == KEY_F1 && !input_focus)
		{
			jinput_clear(input);
			jwidget_set_visible(input, true);
			gscreen_set_tab_fkeys_visible(s, 0, false);
			jscene_set_focused_widget(scene, input);
		}

		#if GINT_RENDER_MONO
		if(key == KEY_F2 && !input_focus)
		{
			v.ascii = !v.ascii;
			jfkeys_set_level(s->fkeys, v.ascii);
		}
		#endif

		if(key == KEY_F3 && !input_focus) v.base = 0x80000000;
		if(key == KEY_F4 && !input_focus) v.base = 0x88000000;
		if(key == KEY_F5 && !input_focus) v.base = 0xe5200000;
		if(key == KEY_F6 && !input_focus) v.base = 0x00300000;
		mem->widget.update = 1;
	}
	jwidget_destroy(scene);
}
