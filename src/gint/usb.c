#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/mpu/usb.h>
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/drivers/asyncio.h>
#include <gint/intc.h>
#include <gint/cpu.h>
#include <gint/mpu/power.h>
#include <gint/mpu/cpg.h>
#include <gint/defs/util.h>
#include <gint/bfile.h>
#include <gint/gint.h>
#include <gint/config.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>

#include <libprof.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USB SH7305_USB

/* USB log buffer */
#define LOG_SIZE _(768, 4096)
static char log_buffer[LOG_SIZE] = { 0 };
static int log_pos = 0;
static int const log_lines = _(8,15);
static int volatile log_interrupt = 0;
static int volatile *global_interrupt_flag = NULL;

struct alignment_write_data {
	uint32_t al4_shbuf[4];
	uint8_t al4_shbuf_size[4];
};

static void reset_logger(void)
{
	memset(log_buffer, 0, LOG_SIZE);
	log_pos = 0;
}

static void usb_logger(char const *format, va_list args)
{
	/* Interrupt getkey() so that the log can be printed */
	if(global_interrupt_flag)
		*global_interrupt_flag = 1;
	log_interrupt = 1;
	if(log_pos >= LOG_SIZE) return;

	cpu_atomic_start();
	int length = vsnprintf(log_buffer+log_pos, LOG_SIZE-log_pos, format, args);
	log_pos += min(length, LOG_SIZE - 1 - log_pos);
	cpu_atomic_end();
}

static void save_logger(void)
{
	uint16_t const *file = u"\\\\fls0\\usb-log.txt";
	int size = log_pos;

	BFile_Remove(file);
	BFile_Create(file, BFile_File, &size);

	int fd = BFile_Open(file, BFile_WriteOnly);
	if(fd < 0) return;

	BFile_Write(fd, log_buffer, log_pos);
	BFile_Close(fd);
}

//---
// Rendering functions
//---

static bool xy(int order, int scroll, int *x, int *y)
{
	/* Item per line, total lines */
	int ipl = _(2, 3);
	int lines = _(8, 15);

	*x = *y = 0;
	order -= ipl * scroll;
	if(order < 0 || order >= ipl * lines)
		return false;

	*x = _(3,  6) + _(64, 128) * (order % ipl);
	*y = _(8, 20) + _( 6,  12) * (order / ipl);
	return true;
}

static void val(int order, char const *name, uint32_t value, int scroll)
{
	int x, y;
	if(xy(order, scroll, &x, &y)) {
		dtext(x, y, C_BLACK, name);
		dprint(x+_(40,88), y, C_BLACK, "%04X", value);
	}
}

static void txt(int order, char const *str, int scroll)
{
	int x, y;
	if(xy(order, scroll, &x, &y))
		dtext(x, y, C_BLACK, str);
}

/* Automatically insert ".word" for most registers */
#define reg(order, name, value) val(order, name, value.word)
/* Force casts to uint32_t for pointers */
#define val(order, name, value) val(order, name, (uint32_t)value, scroll)
/* Normal text */
#define TXT(STR) do { \
	order = (order % _(2,3)) ? (order - (order % _(2,3)) + _(2,3)) : order; \
	txt(order, STR, scroll); \
	order += _(2,3); \
} while(0)
#define VAL(STR, VALUE) do { \
	(val)(order, STR, (uint32_t)VALUE, scroll); \
	order++; \
} while(0)

static void draw_registers(GUNUSED int scroll)
{
	uint16_t volatile *MSELCRA = (void *)0xa4050180;
	uint16_t volatile *MSELCRB = (void *)0xa4050182;

	reg( 0, "SYSCFG",    USB.SYSCFG);
	reg( 1, "BUSWAIT",   USB.BUSWAIT);
	reg( 2, "SYSSTS",    USB.SYSSTS);
	reg( 3, "DVSTCTR",   USB.DVSTCTR);
	reg( 4, "CFIFOSEL",  USB.CFIFOSEL);
	reg( 5, "CFIFOCTR",  USB.CFIFOCTR);
	reg( 6, "INTENB0",   USB.INTENB0);
	reg( 7, "D0FIFOSEL", USB.D0FIFOSEL);
	reg( 8, "D0FIFOCTR", USB.D0FIFOCTR);
	reg( 9, "INTSTS0",   USB.INTSTS0);
	reg(10, "D1FIFOSEL", USB.D1FIFOSEL);
	reg(11, "D1FIFOCTR", USB.D1FIFOCTR);
	reg(12, "INTENB1",   USB.INTENB1);
	val(13, "BRDYENB",   USB.BRDYENB);
	val(14, "BRDYSTS",   USB.BRDYSTS);
	reg(15, "INTSTS1",   USB.INTSTS1);
	val(16, "NRDYENB",   USB.NRDYENB);
	val(17, "NRDYSTS",   USB.NRDYSTS);
	reg(18, "FRMNUM",    USB.FRMNUM);
	val(19, "BEMPENB",   USB.BEMPENB);
	val(20, "BEMPSTS",   USB.BEMPSTS);
	reg(21, "UFRMNUM",   USB.UFRMNUM);
	reg(22, "USBADDR",   USB.USBADDR);
	reg(23, "USBREQ",    USB.USBREQ);
	reg(24, "USBVAL",    USB.USBVAL);
	reg(25, "USBINDX",   USB.USBINDX);
	reg(26, "USBLENG",   USB.USBLENG);
	reg(27, "DCPCFG",    USB.DCPCFG);
	reg(28, "DCPMAXP",   USB.DCPMAXP);
	reg(29, "DCPCTR",    USB.DCPCTR);
	reg(30, "PIPESEL",   USB.PIPESEL);
	reg(31, "PIPECFG",   USB.PIPECFG);
	reg(32, "PIPEBUF",   USB.PIPEBUF);
	reg(33, "PIPEMAXP",  USB.PIPEMAXP);
	reg(34, "PIPEPERI",  USB.PIPEPERI);
	reg(35, "PIPE1CTR",  USB.PIPECTR[0]);

	reg(37, "SOFCFG",    USB.SOFCFG);
	reg(38, "UPONCR",    SH7305_USB_UPONCR);
	val(39, "REG_C2",    USB.REG_C2);
	val(40, "MSELCRA",   *MSELCRA);
	val(41, "MSELCRB",   *MSELCRB);

	#ifdef FX9860G
	if(scroll >= 14) dprint(3, 50 - 6 * (scroll - 14), C_BLACK,
		"USBCLKCR:%08X", SH7305_CPG.USBCLKCR.lword);
	if(scroll >= 15) dprint(3, 50 - 6 * (scroll - 15), C_BLACK,
		"MSTPCR2: %08X", SH7305_POWER.MSTPCR2.lword);
	#endif

	#ifdef FXCG50
	dprint(row_x(1), 188, C_BLACK, "USBCLKCR:%08X  MSTPCR2:%08X",
		SH7305_CPG.USBCLKCR.lword, SH7305_POWER.MSTPCR2.lword);
	#endif
}

static void draw_context(int scroll)
{
	usb_state_t *s = NULL;
	for(int i = 0; i < gint_driver_count(); i++) {
		if(!strcmp(gint_drivers[i].name, "USB")) s = gint_world_os[i];
	}
	if(!s) return;

	int order = 0;

	TXT("Module management:");
	VAL("SYSCFG",    s->SYSCFG);
	VAL("BUSWAIT",   s->BUSWAIT);
	VAL("DVSTCTR",   s->DVSTCTR);
	VAL("SOFCFG",    s->SOFCFG);
	VAL("TESTMODE",  s->TESTMODE);
	VAL("REG_C2",    s->REG_C2);

	TXT("Interrupt control:");
	VAL("INTENB0",   s->INTENB0);
	VAL("INTENB1",   s->INTENB1);
	order += 1;
	VAL("BRDYENB",   s->BRDYENB);
	VAL("NRDYENB",   s->NRDYENB);
	VAL("BEMPENB",   s->BEMPENB);

	TXT("DCP configuration:");
	VAL("DCPMAXP",   s->DCPMAXP);

#ifdef GINT_USB_DEBUG
	VAL("DCPCFG",    s->DCPCFG);
	VAL("DCPCTR",    s->DCPCTR);

	TXT("Debug (module):");
	VAL("SYSSTS",    s->SYSSTS);
	VAL("FRMNUM",    s->FRMNUM);
	VAL("UFRMNUM",   s->UFRMNUM);
	VAL("INTSTS0",   s->INTSTS0);
	VAL("INTSTS1",   s->INTSTS1);
	order += 1;
	VAL("BRDYSTS",   s->BRDYSTS);
	VAL("NRDYSTS",   s->NRDYSTS);
	VAL("BEMPSTS",   s->BEMPSTS);
	VAL("USBADDR",   s->USBADDR);
	VAL("USBREQ",    s->USBREQ);
	VAL("USBVAL",    s->USBVAL);
	VAL("USBINDX",   s->USBINDX);
	VAL("USBLENG",   s->USBLENG);

	TXT("Debug (FIFOs):");
	VAL("CFIFOSEL",  s->CFIFOSEL);
	VAL("D0FIFOSEL", s->D0FIFOSEL);
	VAL("D1FIFOSEL", s->D1FIFOSEL);
	VAL("CFIFOCTR",  s->CFIFOCTR);
	VAL("D0FIFOCTR", s->D0FIFOCTR);
	VAL("D1FIFOCTR", s->D1FIFOCTR);

	TXT("Debug (pipes):");
	VAL("PIPESEL",   s->PIPESEL);
	order += 2;
	for(int i = 0; i < 9; i++) {
		char str[16];

		sprintf(str, "PIPE%dCFG", i+1);
		VAL(str, s->PIPECFG[i]);
		sprintf(str, "PIPE%dCTR", i+1);
		VAL(str, s->PIPEnCTR[i]);
		sprintf(str, "PIPE%dBUF", i+1);
		VAL(str, s->PIPEBUF[i]);
	}
#endif
}

static int draw_log(int offset)
{
	int const y0=_(8,20), dy=_(6,12);
	int const x=_(2,6), w=DWIDTH-_(9,15);

	/* Split log into lines */
	char const *log = log_buffer;
	int y = y0 - offset*dy;
	int line = 0;

	while(log < log_buffer + log_pos)
	{
		if(log[0] == '\n') {
			y += dy;
			line++;
			log++;
			continue;
		}

		char const *endscreen = drsize(log, NULL, w, NULL);
		char const *endline = strchrnul(log, '\n');
		int len = min(endscreen-log, endline-log);

		if(y >= y0 && y < y0 + log_lines * dy)
			dtext_opt(x, y, C_BLACK, C_NONE, DTEXT_LEFT, DTEXT_TOP, log, len);
		y += dy;
		line++;
		log += len + (log[len] == '\n');
	}

	if(line > log_lines)
	{
		scrollbar_px(y0, y0 + log_lines*dy, 0, line, offset, log_lines);
		return line - log_lines;
	}

	return 0;
}

static void draw_pipes(void)
{
#ifdef FXCG50
	char const *PID[4] = { "NAK", "BUF", "STALL0", "STALL1" };
	char const *BSTS[2] = { "Disabled", "Enabled" };
	char const *FRDY[2] = { "NotReady", "Ready" };
	char const *PBUSY[2] = { "Idle", "Busy" };

	row_print(1,  1, "CFIFOSEL:");
	row_print(1, 11, "%04x", USB.CFIFOSEL.word);
	row_print(1, 16, "CFIFOCTR:");
	row_print(1, 26, "%04x %s", USB.CFIFOCTR.word,
		FRDY[USB.CFIFOCTR.FRDY]);
	row_print(2,  1, "DCPCTR:");
	row_print(2, 11, "%04x %s %s %s %d", USB.DCPCTR.word,
		BSTS[USB.DCPCTR.BSTS], PBUSY[USB.DCPCTR.PBUSY], PID[USB.DCPCTR.PID],
		USB.DCPCTR.CCPL);

	row_print(3,  1, "D0FIFOCTR:");
	row_print(3, 11, "%04x", USB.D0FIFOCTR.word);
	row_print(3, 16, "D0FIFOSEL:");
	row_print(3, 26, "%04x", USB.D0FIFOSEL.word);

	row_print(4,  1, "D1FIFOCTR:");
	row_print(4, 11, "%04x", USB.D1FIFOCTR.word);
	row_print(4, 16, "D1FIFOSEL:");
	row_print(4, 26, "%04x", USB.D1FIFOSEL.word);

	int PIPESEL = USB.PIPESEL.word;

	for(int i = 0; i < 9; i++)
	{
		USB.PIPESEL.word = i+1;

		row_print(i+5,  1, "PIPE%dCFG:", i+1);
		row_print(i+5, 10, "%04x", USB.PIPECFG.word);
		row_print(i+5, 16, "PIPE%dCTR:", i+1);
		row_print(i+5, 25, "%04x", USB.PIPECTR[i].word);
		row_print(i+5, 31, "PIPE%dBUF:", i+1);
		row_print(i+5, 40, "%04x", USB.PIPEBUF.word);
	}

	USB.PIPESEL.word = PIPESEL;
#endif
}

static void draw_tests(GUNUSED struct alignment_write_data *data)
{
#ifdef FX9860G
	dprint(1,  8, C_BLACK, "1: Screenshot");
	dprint(1, 14, C_BLACK, "2: Async screenshot");
	dprint(1, 20, C_BLACK, "3: Send text");
	dprint(1, 26, C_BLACK, "4: Send USB log");
	dprint(1, 32, C_BLACK, "5: FIFO alignm. test");
#endif

#ifdef FXCG50
	row_print(1, 1, "[1]: Take screenshot (fxlink API)");
	row_print(2, 1, "[2]: Take screenshot (asynchronous)");
	row_print(3, 1, "[3]: Send some text (fxlink API)");
	row_print(4, 1, "[4]: Send some logs (fxlink API)");
	row_print(5, 1, "[5]: Pipe write alignment tests");

	/* for(int i = 0; i < 4; i++) {
		int col = 1 + 20 * (i & 1);
		int row = 7 + (i/2);
		row_print(row, col, "%08x(%d)",
			data->al4_shbuf[i], data->al4_shbuf_size[i]);
	} */
#endif
}

//---
// Tests
//---

static prof_t screenshot_async_prof;

static void screenshot_async_4(void)
{
	prof_leave(screenshot_async_prof);
	USB_LOG("Async DMA screenshot: %d us\n", prof_time(screenshot_async_prof));
}
static void screenshot_async_3(void)
{
	int pipe = usb_ff_bulk_output();
	GUNUSED int rc = usb_commit_async(pipe, GINT_CALL(screenshot_async_4));
	USB_LOG("commit_async: %d\n", rc);
}
static void screenshot_async_2(void)
{
	int pipe = usb_ff_bulk_output();
	GUNUSED int rc = usb_write_async(pipe, gint_vram, _(1024, 396*224*2),
		true, GINT_CALL(screenshot_async_3));
	USB_LOG("write_async 2: %d\n", rc);
}
static void screenshot_async(void)
{
	USB_LOG("<TEST: Screenshot (async)>\n");
	int pipe = usb_ff_bulk_output();

	int data_size = _(1024, 396*224*2);

	screenshot_async_prof = prof_make();
	prof_enter(screenshot_async_prof);

	usb_fxlink_header_t header;
	if(!usb_fxlink_fill_header(&header, "gintctl", "asyncscreen", data_size))
		return;

	GUNUSED int rc = usb_write_async(pipe, &header, sizeof header, false,
		GINT_CALL(screenshot_async_2));
	USB_LOG("write_async 1: %d\n", rc);
}

static void alignment_write_tests(GUNUSED struct alignment_write_data *data)
{
	GALIGNED(4) static char const str[] =
		"Alright let's get usb_pipe_write4 and usb_pipe_flush4 out of the way "
		"so we can move on to IN pipes.\n";

	for(int i = 0; i < 1; i++) {
		char const *text = str;
		int size = 96;

		int pipe = usb_ff_bulk_output();
		usb_fxlink_header_t header;
		usb_fxlink_fill_header(&header, "fxlink", "text", size);
		usb_write_sync(pipe, &header, sizeof header, false);

		// int j = 0;
		while(size > 0) {
			int s = rand() % 9;
			s = max(s, 1);
			s = min(s, size);
			USB_LOG("%d bytes remain, doing %d\n", size, s);

			usb_write_sync(pipe, text, s, false);
			text += s;
			size -= s;

			/* if(j < 4) {
				extern asyncio_op_t pipe_transfers[10];
				data->al4_shbuf[i] = pipe_transfers[5].shbuf;
				data->al4_shbuf_size[i] = pipe_transfers[5].shbuf_size;
			}
			j++; */
		}

		usb_commit_sync(pipe);
	}
}

//---
// Main function
//---

/* gintctl_gint_usb(): USB communication */
void gintctl_gint_usb(void)
{
	int key=0, tab=0;
	int open = usb_is_open();
	GUNUSED int scroll0=0, scroll1=0, scroll2=0, maxscroll2=0;

	struct alignment_write_data awd = { 0 };

	usb_set_log(usb_logger);
	global_interrupt_flag = &gintctl_interrupt;
	srand(0xc0ffee);

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		row_title("USB 2.0 communication");
		dsubimage(0, 56, &img_opt_gint_usb, 0, open * 9, 128, 9, DIMAGE_NONE);
		if(tab == 0) scrollbar_px(8, 52, 0, 22, scroll0, 8);
		#endif

		font_t const *old_font = dfont(_(&font_mini, dfont_default()));

		if(tab == 0) draw_registers(scroll0);
		if(tab == 1) draw_context(scroll1);
		if(tab == 2) maxscroll2 = draw_log(scroll2);
		if(tab == 3) draw_pipes();
		if(tab == 4) draw_tests(&awd);

		#ifdef FXCG50
		if(tab == 2)
			row_title("USB logs (SHIFT+7: Save to file, SHIFT+8: Clear)");
		else
			row_title("USB 2.0 function module and communication");
		fkey_menu(1, "REGS");
		fkey_menu(2, "CTX");
		fkey_menu(3, "LOG");
		fkey_menu(4, "PIPES");
		fkey_menu(5, "TESTS");
		fkey_action(6, open ? "CLOSE" : "OPEN");
		#endif

		dfont(old_font);
		dupdate();

		key_event_t ev = gintctl_getkey();
		key = ev.key;

		/* Scroll down log automatically at the cost of a redraw */
		if(log_interrupt && tab == 2)
			scroll2 = draw_log(0);

		log_interrupt = 0;

		if(key == KEY_F1) tab = 0;
		if(key == KEY_F2) tab = 1;
		if(key == KEY_F3) tab = 2;
		if(key == KEY_F4) tab = 3;
		if(key == KEY_F5) tab = 4;

		if(key == KEY_F6)
		{
			if(open) usb_close(), open = 0;
			else
			{
				usb_interface_t const *interfaces[] = { &usb_ff_bulk, NULL };
				usb_open(interfaces, GINT_CALL_SET(&open));
			}
		}

		int scroll_speed = 1;
		if(keydown(KEY_SHIFT)) scroll_speed = 4;
		if(keydown(KEY_ALPHA)) scroll_speed = 16;

		#ifdef FX9860G
		if(tab == 0 && key == KEY_UP && scroll0 > 0) scroll0--;
		if(tab == 0 && key == KEY_DOWN && scroll0 < 15) scroll0++;
		#endif

		if(tab == 1 && key == KEY_UP && scroll1 > 0) scroll1--;
		if(tab == 1 && key == KEY_DOWN && scroll1 < _(0,13)) scroll1++;

		if(tab == 2 && key == KEY_UP)
			scroll2 = max(scroll2 - scroll_speed, 0);
		if(tab == 2 && key == KEY_DOWN)
			scroll2 = min(scroll2 + scroll_speed, maxscroll2);
		if(tab == 2 && ev.shift && key == KEY_7)
			gint_world_switch(GINT_CALL(save_logger));
		if(tab == 2 && ev.shift && key == KEY_8)
			reset_logger();

		if(tab == 4 && key == KEY_1 && usb_is_open()) {
//			extern prof_t usb_cpu_write_prof;
//			usb_cpu_write_prof = prof_make();

			uint32_t time = prof_exec({
				usb_fxlink_screenshot(true);
			});

			USB_LOG("Screenshot: %d us\n", time);
//			USB_LOG("Spent CPU writing: %d us\n", prof_time(usb_cpu_write_prof));
		}
		if(tab == 4 && key == KEY_2 && usb_is_open()) screenshot_async();
		if(tab == 4 && key == KEY_3 && usb_is_open()) {
			GALIGNED(4) char str[] = "Hi! This is gintctl!";
			usb_fxlink_text(str, 0);
		}
		if(tab == 4 && key == KEY_4 && usb_is_open()) {
			uint32_t time = prof_exec({
				int pipe = usb_ff_bulk_output();
				int data_size = _(1024, 396*224*2);
				usb_fxlink_header_t header;
				usb_fxlink_fill_header(&header, "gintctl", "dmascreen", data_size);

				usb_write_sync(pipe, &header, sizeof header, false);
//				usb_write_sync(pipe, (void *)0xfe200000, _(1024, 396*224*2), true);
				usb_write_sync(pipe, gint_vram, _(1024, 396*224*2), true);
				usb_commit_sync(pipe);
			});
			USB_LOG("DMA screenshot: %d us\n", time);
		}
		if(tab == 4 && key == KEY_5 && usb_is_open())
			alignment_write_tests(&awd);
	}

	usb_set_log(NULL);
	global_interrupt_flag = NULL;
}
