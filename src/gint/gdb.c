#include <gint/display.h>
#include <gint/gdb.h>
#include <gint/mpu/power.h>
#include <gint/mpu/ubc.h>
#include <gint/ubc.h>

#include <gintctl/assets.h>
#include <gintctl/gint.h>
#include <gintctl/util.h>

#ifdef FXCG50
static void val(int y, const char *name, uint32_t value)
{
	dtext(0, 20 + y*12, C_BLACK, name);
	dprint(88, 20 + y*12, C_BLACK, "%08X", value);
}
#else
static void val(int y, const char *name, uint32_t value)
{
	dtext(0, 8 + y*6, C_BLACK, name);
	dprint(40, 8 + y*6, C_BLACK, "%08X", value);
}
#endif

#define reg(y,n,v) val(y,n,v.lword)

extern int (*gint_exc_catcher)(uint32_t code);

int gintctl_gint_gdb_bank1_test(void);

void gintctl_gint_gdb(void)
{
	int key = 0;
	int ret = -1;
	while (key != KEY_EXIT) {
		dclear(C_WHITE);
#ifdef FXCG50
		row_title("GDB remote serial protocol");
#else
		row_title("GDB");
#endif

#ifdef FXCG50
		fkey_action(6, "START");
		fkey_action(5, "PANICR");
		fkey_action(4, "PANICW");
		fkey_action(1, "BANK 1");
#endif

#ifdef FX9860G
		font_t const *old_font = dfont(&font_mini);
#endif
		val(0, "ret", ret);
		reg(1, "MSTPCR0", SH7305_POWER.MSTPCR0);
		reg(2, "CBR0", SH7305_UBC.CBR0);
		reg(3, "CRR0", SH7305_UBC.CRR0);
		val(4, "CAR0", SH7305_UBC.CAR0);
		val(5, "CAMR0", SH7305_UBC.CAMR0);
		reg(6, "CBR1", SH7305_UBC.CBR1);
		reg(7, "CRR1", SH7305_UBC.CRR1);
		val(8, "CAR1", SH7305_UBC.CAR1);
		val(9, "CAMR1", SH7305_UBC.CAMR1);
		val(10, "CDR1", SH7305_UBC.CDR1);
		val(11, "CDMR1", SH7305_UBC.CDMR1);
		reg(12, "CETR1", SH7305_UBC.CETR1);
		reg(13, "CCMFR", SH7305_UBC.CCMFR);
		reg(14, "CBCR", SH7305_UBC.CBCR);
		val(15, "DBR", (uint32_t) ubc_getDBR());

#ifdef FXCG50
		dprint(176, 20, C_BLACK, "exc_catcher: %p", gint_exc_catcher);
#endif

#ifdef FX9860G
		dfont(old_font);
#endif
		dupdate();

		key_event_t ev = gintctl_getkey();
		key = ev.key;

		volatile uint32_t* miss = (void*)0x41414140;
		if (key == KEY_F6) {
			ret = gdb_start();
		} else if (key == KEY_F5) {
			ret = *miss;
		} else if (key == KEY_F4) {
			*miss = 0x1337;
		} else if (key == KEY_F1) {
			ret = gintctl_gint_gdb_bank1_test();
		}
	}
}
