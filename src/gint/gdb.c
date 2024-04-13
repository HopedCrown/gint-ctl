#include <gint/display.h>
#include <gint/gdb.h>
#include <gint/mpu/power.h>
#include <gint/mpu/ubc.h>
#include <gint/ubc.h>
#include <gint/config.h>

#include <gintctl/assets.h>
#include <gintctl/gint.h>
#include <gintctl/util.h>
#include <stdio.h>

#ifdef FXCG50
static void val(int y, const char *name, uint32_t value)
{
	dtext(4, 20 + y*12, C_BLACK, name);
	dprint(92, 20 + y*12, C_BLACK, "%08X", value);
}
#else
static void val(int y, const char *name, uint32_t value)
{
	int px = 64 * (y / 8);
	int py = 8 + 6 * (y % 8);
	dtext(px, py, C_BLACK, name);
	dprint(px+30, py, C_BLACK, "%08X", value);
}
#endif

#define reg(y,n,v) val(y,n,v.lword)

extern int (*gint_exc_catcher)(uint32_t code);

int gintctl_gint_gdb_bank1_test(void);

void gintctl_gdb_dbgprint(void)
{
	fprintf(stderr, "debug=%d\n", 73);
}

void gintctl_gint_gdb(void)
{
	int key = 0;
	int ret = -1;

	gdb_start_on_exception();
	gdb_redirect_streams(false, true);

	while (key != KEY_EXIT) {
		dclear(C_WHITE);
		row_title(GINT_HW_SWITCH("GDB", "GDB remote serial protocol"));

#if GINT_RENDER_MONO
		dimage(0, 56, &img_opt_gint_gdb);
#elif GINT_RENDER_RGB
		fkey_action(6, "TRAP");
		fkey_action(5, "PANICR");
		fkey_action(4, "PANICW");
		fkey_action(3, "I/O");
		fkey_action(1, "BANK 1");
#endif

		font_t const *old_font = dfont(_(&font_mini, dfont_default()));
		reg(0, _("MSTP...", "MSTPCR0"), SH7305_POWER.MSTPCR0);
		reg(1, "CBR0", SH7305_UBC.CBR0);
		reg(2, "CRR0", SH7305_UBC.CRR0);
		val(3, "CAR0", SH7305_UBC.CAR0);
		val(4, "CAMR0", SH7305_UBC.CAMR0);
		reg(5, "CBR1", SH7305_UBC.CBR1);
		reg(6, "CRR1", SH7305_UBC.CRR1);
		val(7, "CAR1", SH7305_UBC.CAR1);
		val(8, "CAMR1", SH7305_UBC.CAMR1);
		val(9, "CDR1", SH7305_UBC.CDR1);
		val(10, "CDMR1", SH7305_UBC.CDMR1);
		reg(11, "CETR1", SH7305_UBC.CETR1);
		reg(12, "CCMFR", SH7305_UBC.CCMFR);
		reg(13, "CBCR", SH7305_UBC.CBCR);
		val(14, "DBR", (uint32_t) ubc_getDBR());

#ifdef FX9860G
		val(15, "ret", ret);
#endif
#ifdef FXCG50
		dprint(176, 20, C_BLACK, "ret: %d", ret);
		dprint(176, 32, C_BLACK, "exc_catcher: %p", gint_exc_catcher);
#endif

		dfont(old_font);
		dupdate();

		key_event_t ev = gintctl_getkey();
		key = ev.key;

		volatile uint32_t* miss = (void*)0x41414140;
		if (key == KEY_F6) {
			__asm__("trapa #42");
		} else if (key == KEY_F5) {
			ret = *miss;
		} else if (key == KEY_F4) {
			*miss = 0x1337;
		} else if (key == KEY_F3) {
			fprintf(stderr, "debug=%d\n", 42);
		} else if (key == KEY_F1) {
			ret = gintctl_gint_gdb_bank1_test();
		}
	}
}
