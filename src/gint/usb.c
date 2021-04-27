#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/mpu/usb.h>
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/intc.h>
#include <gint/mpu/power.h>
#include <gint/mpu/cpg.h>
#include <gint/defs/util.h>
#include <gint/std/stdio.h>
#include <gint/std/string.h>
#include <gint/bfile.h>
#include <gint/gint.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>

#define USB SH7305_USB

/* USB log buffer */
#define LOG_SIZE _(1023, 16383)
static char log_buffer[LOG_SIZE+1];
static int log_pos;
static int volatile log_interrupt = 0;

static void reset_logger(void)
{
	memset(log_buffer, 0, LOG_SIZE+1);
	log_pos = 0;
}

static void usb_logger(char const *format, va_list args)
{
	if(log_pos >= LOG_SIZE) return;
	int length = vsnprintf(log_buffer+log_pos, LOG_SIZE-log_pos, format, args);
	log_pos = min(log_pos + length, LOG_SIZE);
	/* Interrupt getkey() so that the log can be printed */
	log_interrupt = 1;
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

static void val(int order, char const *name, uint32_t value,GUNUSED int scroll)
{
	#ifdef FX9860G
	order -= 2 * scroll;
	if(order < 0 || order > 15) return;
	int y = 6 * (order / 2) + 8;
	int x = 3 + 64 * (order % 2);
	#endif

	#ifdef FXCG50
	int y = 12 * (order / 3) + 20;
	int x = 6 + 128 * (order % 3);
	#endif

	dprint(x, y, C_BLACK, "%s", name);
	dprint(x+_(40,88), y, C_BLACK, "%04X", value);
}

/* Automatically insert ".word" for most registers */
#define reg(order, name, value) val(order, name, value.word)
/* Force casts to uint32_t for pointers */
#define val(order, name, value) val(order, name, (uint32_t)value, scroll)

static void draw_registers(GUNUSED int scroll)
{
	uint16_t volatile *MSELCRA = (void *)0xa4050180;
	uint16_t volatile *MSELCRB = (void *)0xa4050182;

	reg( 0, "SYSCFG",    USB.SYSCFG);
	reg( 1, "BUSWAIT",   USB.BUSWAIT);
	reg( 2, "SYSSTS",    USB.SYSSTS);
	reg( 3, "DVSTCTR",   USB.DVSTCTR);
	reg( 4, "TESTMODE",  USB.TESTMODE);
	reg( 5, "CFIFOSEL",  USB.CFIFOSEL);
	reg( 6, "CFIFOCTR",  USB.CFIFOCTR);
	reg( 7, "D0FIFOSEL", USB.D0FIFOSEL);
	reg( 8, "D0FIFOCTR", USB.D0FIFOCTR);
	reg( 9, "D1FIFOSEL", USB.D1FIFOSEL);
	reg(10, "D1FIFOCTR", USB.D1FIFOCTR);
	reg(11, "INTENB0",   USB.INTENB0);
	reg(12, "BRDYENB",   USB.BRDYENB);
	reg(13, "NRDYENB",   USB.NRDYENB);
	reg(14, "BEMPENB",   USB.BEMPENB);
	reg(15, "SOFCFG",    USB.SOFCFG);
	reg(16, "INTSTS0",   USB.INTSTS0);
	reg(17, "BRDYSTS",   USB.BRDYSTS);
	reg(18, "NRDYSTS",   USB.NRDYSTS);
	reg(19, "BEMPSTS",   USB.BEMPSTS);
	reg(20, "FRMNUM",    USB.FRMNUM);
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
	reg(36, "PIPE1TRE",  USB.PIPE1TRE);
	reg(37, "PIPE1TRN",  USB.PIPE1TRN);
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
	dprint(row_x(1), 188, C_BLACK, "USBCLKCR: %08X   MSTPCR2: %08X",
		SH7305_CPG.USBCLKCR.lword, SH7305_POWER.MSTPCR2.lword);
	#endif
}

static void draw_context(void)
{
	usb_state_t *s = NULL;
	for(int i = 0; i < gint_driver_count(); i++) {
		if(!strcmp(gint_drivers[i].name, "USB")) s = gint_world_os[i];
	}
	if(!s) return;

	GUNUSED int scroll = 0;
	val( 0, "SYSCFG",    s->SYSCFG);
	val( 1, "DVSTCTR",   s->DVSTCTR);
	val( 2, "TESTMODE",  s->TESTMODE);
	val( 3, "REG_C2",    s->REG_C2);
	val( 4, "CFIFOSEL",  s->CFIFOSEL);
	val( 5, "D0FIFOSEL", s->D0FIFOSEL);
	val( 6, "D1FIFOSEL", s->D1FIFOSEL);
	val( 7, "INTENB0",   s->INTENB0);
	val( 8, "BRDYENB",   s->BRDYENB);
	val( 9, "NRDYENB",   s->NRDYENB);
	val(10, "BEMPENB",   s->BEMPENB);
	val(11, "SOFCFG",    s->SOFCFG);
	val(12, "DCPCFG",    s->DCPCFG);
	val(13, "DCPMAXP",   s->DCPMAXP);
	val(14, "DCPCTR",    s->DCPCTR);

	// uint16_t PIPECFG[9], PIPEBUF[9], PIPEMAXP[9], PIPEPERI[9];
	// uint16_t PIPEnCTR[9], PIPEnTRE[5], PIPEnTRN[5];
}

static int draw_log(int offset)
{
	int const y0=_(8,20), dy=_(6,12), lines=_(8,15);
	int const x=_(2,6), w=DWIDTH-_(9,15);

	/* Split log into lines */
	char const *log = log_buffer;
	int y = y0 - offset*dy;
	int line = 0;

	while(log < log_buffer + log_pos)
	{
		char const *endscreen = drsize(log, NULL, w, NULL);
		char const *endline = strchrnul(log, '\n');
		int end = min(endscreen-log, endline-log);

		if(y >= y0 && y < y0 + lines * dy)
			dtext_opt(x, y, C_BLACK, C_NONE, DTEXT_LEFT, DTEXT_TOP, log, end);
		y += dy;
		line++;
		log += end + (log[end] == '\n');
	}

	if(line > lines)
	{
		scrollbar_px(y0, y0 + lines*dy, 0, line, offset, lines);
		return line - lines;
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

	for(int i = 0; i < 9; i++)
	{
		row_print(i+5,  1, "PIPE%dCTR:", i+1);
		row_print(i+5, 10, "%04x", USB.PIPECTR[i].word);
	}
#endif
}

static void open_callback(void)
{
	usb_log("Open callback, doing test!\n");

	int pipe = 5;
	usb_write_async(pipe, gint_vram, _(1024, 396*224*2), 4, false,
		GINT_CALL_NULL);
	usb_commit_async(pipe, GINT_CALL_NULL);
}

/* gintctl_gint_usb(): USB communication */
void gintctl_gint_usb(void)
{
	int key=0, tab=0;
	bool open=false;
	GUNUSED int scroll0=0, scroll2=0, maxscroll2=0;

	reset_logger();
	usb_set_log(usb_logger);

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
		if(tab == 1) draw_context();
		if(tab == 2) maxscroll2 = draw_log(scroll2);
		if(tab == 3) draw_pipes();

		#ifdef FXCG50
		row_title("USB 2.0 function module and communication");
		fkey_menu(1, "REGS");
		fkey_menu(2, "CTX");
		fkey_menu(3, "LOG");
		fkey_menu(4, "PIPES");
		if(tab == 2) fkey_button(5, "FILE");
		fkey_action(6, open ? "CLOSE" : "OPEN");
		#endif

//		dprint_opt(DWIDTH - 8, DHEIGHT - 6, C_BLACK, C_NONE, DTEXT_RIGHT,
//			DTEXT_BOTTOM, "Interrupts: %d", usb_interrupt_count);

		dfont(old_font);
		dupdate();

		key = getkey_opt(GETKEY_DEFAULT, &log_interrupt).key;
		log_interrupt = 0;

		if(key == KEY_F1) tab = 0;
		if(key == KEY_F2) tab = 1;
		if(key == KEY_F3) tab = 2;
		if(key == KEY_F4) tab = 3;

		if(tab == 2 && key == KEY_F5)
			gint_world_switch(GINT_CALL(save_logger));

		if(key == KEY_F6)
		{
			if(open) usb_close(), open = false;
			else
			{
				usb_interface_t const *interfaces[] = { &usb_ff_bulk, NULL };
				int rc = usb_open(interfaces, GINT_CALL(open_callback));
				open = (rc == 0);
			}
		}

		#ifdef FX9860G
		if(tab == 0 && key == KEY_UP && scroll0 > 0) scroll0--;
		if(tab == 0 && key == KEY_DOWN && scroll0 < 15) scroll0++;
		#endif

		if(tab == 2 && key == KEY_UP && scroll2 > 0) scroll2--;
		if(tab == 2 && key == KEY_DOWN && scroll2 < maxscroll2) scroll2++;
	}
}
