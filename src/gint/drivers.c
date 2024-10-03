#include <gint/gint.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/clock.h>
#include <gint/hardware.h>
#include <gint/mpu/tmu.h>
#include <gint/mpu/dma.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>
#include <gintctl/ui.h>

#include <stdio.h>
#include <string.h>

//---
// State management
//---

static void gen_label(jlabel *l, gint_world_t world, int i)
{
	if(gint_driver_flags[i] & GINT_DRV_SHARED)
		return jlabel_set_text(l, "Device is shared");
	if(world == gint_world_addin && (gint_driver_flags[i] & GINT_DRV_CLEAN))
		return jlabel_set_text(l, "Device is clean");

	jlabel_set_text(l, "No state");

	if(!strcmp(gint_drivers[i].name, "CPG")) {
		cpg_state_t const *s = world[i];
		if(!isSH3())
			jlabel_asprintf(l, "SSCGCR: %08X", s->SSCGCR);
	}

	else if(!strcmp(gint_drivers[i].name, "CPU")) {
		cpu_state_t const *s = world[i];
		jlabel_asprintf(l,
			"SR: %08X\n"
			"VBR: %08X\n"
			"CPUOPM: %08X",
			s->SR, s->VBR, s->CPUOPM);
	}

	else if(!strcmp(gint_drivers[i].name, "DMA")) {
		dma_state_t const *s = world[i];

		#if GINT_RENDER_MONO
		#define LINE(I) #I ": %08X->%08X %08X\n"
		#define ARGS(I) s->ch[I].SAR, s->ch[I].DAR, s->ch[I].CHCR
		#else
		#define LINE(I) #I ": %08X->%08X TCR:%08X CHCR:%08X\n"
		#define ARGS(I) s->ch[I].SAR, s->ch[I].DAR, s->ch[I].TCR, s->ch[I].CHCR
		#endif
		jlabel_asprintf(l,
			LINE(0) LINE(1) LINE(2) LINE(3) LINE(4) LINE(5) "OR: %08X",
			ARGS(0), ARGS(1), ARGS(2), ARGS(3), ARGS(4), ARGS(5), s->OR);
	}
}

static void draw_state(gint_world_t world, int i)
{
	font_t const *old_font = dfont(_(&font_mini, dfont_default()));

	if(i > 0)
		dprint(_(86,10), _(57,row_y(1)), C_BLACK, "<");
	if(i < (int)gint_driver_count() - 1)
		dprint(DWIDTH - _(4,16), _(57,row_y(1)), C_BLACK, ">");

	dprint_opt(_(106, DWIDTH / 2), _(57, row_y(1)), C_BLACK, C_NONE,
		DTEXT_CENTER, DTEXT_TOP, "%s", gint_drivers[i].name);

	if(gint_driver_flags[i] & GINT_DRV_SHARED)
	{
		dprint_opt(DWIDTH / 2, DHEIGHT / 2, C_BLACK, C_NONE, DTEXT_CENTER,
			DTEXT_MIDDLE, "Device is shared");
		return;
	}
	if(world == gint_world_addin && (gint_driver_flags[i] & GINT_DRV_CLEAN))
	{
		dprint_opt(DWIDTH / 2, DHEIGHT / 2, C_BLACK, C_NONE, DTEXT_CENTER,
			DTEXT_MIDDLE, "Device is clean");
		return;
	}

	if(!strcmp(gint_drivers[i].name, "CPG"))
	{
		cpg_state_t const *s = world[i];
		if(isSH3()) row_print(_(1,3), 1, "No state");
		else row_print(_(1,3), 1, "SSCGCR: %08X", s->SSCGCR);
	}
	else if(!strcmp(gint_drivers[i].name, "CPU"))
	{
		cpu_state_t const *s = world[i];
		row_print(_(1,3), 1, "SR: %08X", s->SR);
		row_print(_(2,4), 1, "VBR: %08X", s->VBR);
		row_print(_(3,5), 1, "CPUOPM: %08X", s->CPUOPM);
	}
	else if(!strcmp(gint_drivers[i].name, "DMA"))
	{
		dma_state_t const *s = world[i];

		#if GINT_RENDER_MONO
		for(int i = 0; i < 6; i++) {
			dprint(1, 1+6*i, C_BLACK, "%d: %08X->%08X %08X",
				i, s->ch[i].SAR, s->ch[i].DAR, s->ch[i].CHCR);
		}
		dprint(1, 43, C_BLACK, "OR: %08X", s->OR);
		#endif

		#if GINT_RENDER_RGB
		for(int i = 0; i < 6; i++) {
			int y=3+5*(i/3), x=1+16*(i%3);
			row_print(y,   x,   "%d:", i);
			row_print(y,   x+2, "SAR");
			row_print(y,   x+7, "%08X", s->ch[i].SAR);
			row_print(y+1, x+2, "DAR");
			row_print(y+1, x+7, "%08X", s->ch[i].DAR);
			row_print(y+2, x+2, "TCR");
			row_print(y+2, x+7, "%08X", s->ch[i].TCR);
			row_print(y+3, x+2, "CHCR");
			row_print(y+3, x+7, "%08X", s->ch[i].CHCR);
		}
		row_print(13, 1, "OR: %08X\n", s->OR);
		#endif
	}
	else if(!strcmp(gint_drivers[i].name, "INTC"))
	{
		intc_state_t const *s = world[i];

		#if GINT_RENDER_MONO
		for(int i = 0; i < 12; i++) {
			dprint(1+32*(i%4),1+6*(i/4), C_BLACK, "%c:%04X", 'A'+i, s->IPR[i]);
		}
		for(int i = 0; i < 13; i++) {
			dprint(1+32*(i%4),25+6*(i/4), C_BLACK, "%d:%02X", i, s->MSK[i]);
		}
		#endif

		#if GINT_RENDER_RGB
		for(int i = 0; i < 12; i++) {
			row_print(3+i/4, 1+11*(i%4), "IPR%c:", 'A'+i);
			row_print(3+i/4, 6+11*(i%4), "%04X", s->IPR[i]);
		}
		for(int i = 0; i < 13; i++) {
			row_print(7+i/4, 1+11*(i%4), "IMR%d:", i);
			row_print(7+i/4, 7+11*(i%4), "%02X", s->MSK[i]);
		}
		#endif
	}
	else if(!strcmp(gint_drivers[i].name, "KEYSC"))
	{
		row_print(_(1,3), 1, "No state");
	}
	else if(!strcmp(gint_drivers[i].name, "MMU"))
	{
		mmu_state_t const *s = world[i];
		row_print(_(1,3), 1, "PASCR: %08X", s->PASCR);
		row_print(_(2,4), 1, "IRMCR: %08X", s->IRMCR);
	}
	else if(!strcmp(gint_drivers[i].name, "R61524"))
	{
		r61524_state_t const *s = world[i];
		row_print(3, 1, "HSA..HEA: %d..%d", s->HSA, s->HEA);
		row_print(4, 1, "VSA..VEA: %d..%d", s->VSA, s->VEA);
	}
	else if(!strcmp(gint_drivers[i].name, "RTC"))
	{
		rtc_state_t const *s = world[i];
		row_print(_(1,3), 1, "RCR1: %02X  RCR2: %02X", s->RCR1, s->RCR2);
	}
	else if(!strcmp(gint_drivers[i].name, "SPU"))
	{
		spu_state_t const *s = world[i];

		#if GINT_RENDER_MONO
		dprint(1,  1, C_BLACK, "PBANKC0: %08X", s->PBANKC0);
		dprint(1,  7, C_BLACK, "PBANKC1: %08X", s->PBANKC1);
		dprint(1, 13, C_BLACK, "XBANKC0: %08X", s->XBANKC0);
		dprint(1, 19, C_BLACK, "XBANKC1: %08X", s->XBANKC1);
		#endif

		#if GINT_RENDER_RGB
		row_print(3,1, "PBANKC0: %08X  PBANKC1: %08X", s->PBANKC0, s->PBANKC1);
		row_print(4,1, "XBANKC0: %08X  XBANKC1: %08X", s->XBANKC0, s->XBANKC1);
		#endif
	}
	else if(!strcmp(gint_drivers[i].name, "T6K11"))
	{
		t6k11_state_t const *s = world[i];
		dprint(1,  1, C_BLACK, "STRD: %02X", s->STRD);
		dprint(1,  7, C_BLACK, "RESET:%d", (s->STRD & 0x08) != 0);
		dprint(1, 13, C_BLACK, "N/F:%d",   (s->STRD & 0x04) != 0);
		dprint(1, 19, C_BLACK, "X/Y:%d",   (s->STRD & 0x02) != 0);
		dprint(1, 25, C_BLACK, "U/D:%d",   (s->STRD & 0x01) != 0);
	}
	else if(!strcmp(gint_drivers[i].name, "TMU"))
	{
		tmu_state_t const *s = world[i];

		for(int k = 0; k < 9; k++) {
			#if GINT_RENDER_MONO
			if(k < 3) dprint(1, 6*k, C_BLACK, "TMU%d: CNT:%08X TCR:%04X %s",
				k, s->t[k].TCNT, s->t[k].TCR, (s->TSTR & (1<<k) ? "STR" : ""));
			else dprint(1, 6*k, C_BLACK, "E%d: TCNT:%08X TCR:%02X TSTR:%02X",
				k-3, s->t[k].TCNT, s->t[k].TCR, s->t[k].TSTR);
			#endif

			#if GINT_RENDER_RGB
			row_print(k+3, 1, "%sTMU%d:", (k<3 ? "" : "E"), (k<3 ? k : k-3));
			if(k < 3) row_print(k+3, 8, "%08X/%08X  TCR:%04X",
				s->t[k].TCNT, s->t[k].TCOR, s->t[k].TCR);
			else row_print(k+3, 8,"%08X/%08X  TCR:%02X    TSTR:%02X",
				s->t[k].TCNT, s->t[k].TCOR, s->t[k].TCR, s->t[k].TSTR);
			#endif
		}
	}
	else if(!strcmp(gint_drivers[i].name, "USB"))
	{
		row_print(_(1,3), 1, "In USB test");
	}

	dfont(old_font);
}

//---
// Manual switches with performance statistics
//---

struct switch_stats {
	/* Number of empty world switches */
	int world_switch_count;
	/* Number of return-to-menu */
	int return_to_menu_count;
	/* TODO: Performance statistics for each driver */
};

static void draw_manual(struct switch_stats *stats)
{
	#if GINT_RENDER_MONO
	row_print(2, 1, "World switches: %d", stats->world_switch_count);
	row_print(3, 1, "Return-to-menu: %d", stats->return_to_menu_count);
	// row_print(4, 1, "Switch time: %d µs", stats->world_switch_time);
	row_print(5, 1, "[1]: World switch");
	row_print(6, 1, "[2]: Return-to-menu");
	row_print(7, 1, "[3]: Measure perf");
	#endif

	#if GINT_RENDER_RGB
	row_print(1, 1, "World switches performed: %d",
		stats->world_switch_count);
	row_print(2, 1, "Return-to-menu performed: %d",
		stats->return_to_menu_count);

	row_print(11, 1, "[1]: Standard world switch");
	row_print(12, 1, "[2]: Return-to-menu with gint_osmenu()");
	row_print(13, 1, "[3]: World switch with shared libprof (TODO)");
	#endif
}

static void table_drv_gen(gtable *t, int row)
{
	char f1[8], f3[8], f4[64];
	gint_driver_t const *d = &gint_drivers[row];
	uint8_t flags = gint_driver_flags[row];

	sprintf(f1, "%d", row);
	sprintf(f3, "%d", d->state_size);
	sprintf(f4, "%s%s%s",
			(flags & GINT_DRV_CLEAN) ? "CLEAN " : "",
			(flags & GINT_DRV_FOREIGN_POWERED) ? _("FP ","FOREIGN_POW. ") : "",
			(flags & GINT_DRV_SHARED) ? "SHARED " : "");
	gtable_provide(t, f1, d->name, f3, f4);
}

//---
// Main test
//---

/* gintctl_gint_drivers(): Test the gint driver logic and world switch */
void gintctl_gint_drivers(void)
{
	int key=0, tab=0, list_scroll=0, list_max=_(7,12), selected_driver=0;
	struct switch_stats stats = { 0 };

	extern bopti_image_t img_opt_gint_drivers;
	gscreen *s = gscreen_create2("Drivers and worlds", &img_opt_gint_drivers,
		"Drivers and world switches", "@DRIVERS;@SWITCH;;;;", NULL);
	gintctl_scene_push(s);

	gtable *table_drv = gtable_create(4, table_drv_gen, NULL, NULL);
	gtable_set_rows(table_drv, gint_driver_count());
	gtable_set_column_titles(table_drv, "#", "Name", "Size", "Flags");
	gtable_set_selection_enabled(table_drv, true);
#if GINT_RENDER_MONO
	gtable_set_column_sizes(table_drv, 1, 4, 2, 8);
	gtable_set_font(table_drv, &font_mini);
#else
	gtable_set_column_sizes(table_drv, 1, 4, 2, 8);
	gtable_set_row_height(table_drv, 12);
	jwidget_set_margin(table_drv, 0, 4, 0, 4);
#endif
	gscreen_add_tab(s, table_drv, table_drv);

	jlabel *label_details = jlabel_create("<details>", NULL);
	gscreen_add_tab(s, label_details, NULL);

	while(key != KEY_EXIT)
	{
		jevent e = jscene_run(gintctl_scene());

		if(jevent_is_press(e, KEY_EXIT) && gscreen_current_tab(s) == 0)
			break;
		if(jevent_is_press(e, KEY_EXIT) && gscreen_current_tab(s) == 1) {
			gscreen_show_tab(s, 0);
		}

		if(e.type == GTABLE_ROW_TRIGGERED) {
			int i = e.data;
			// TODO: Also consider gint_world_addin
			gen_label(label_details, gint_world_os, i);
			gscreen_show_tab(s, 1);
		}

#if 0
		dclear(C_WHITE);

		#if GINT_RENDER_MONO
		if(tab == 0 || tab == 3) row_print(1, 1, "Drivers and worlds");
		dimage(0, 56, &img_opt_gint_drivers);
		#endif

		#if GINT_RENDER_RGB
		row_title("Drivers and world switches");
		fkey_menu(1, "DRIVERS");
		fkey_menu(2, "OS");
		fkey_menu(3, "ADDIN");
		fkey_menu(4, "MANUAL");
		#endif

		if(tab == 0) draw_list(list_scroll, list_max);
		if(tab == 1) draw_state(gint_world_os, selected_driver);
		if(tab == 2) draw_state(gint_world_addin, selected_driver);
		if(tab == 3) draw_manual(&stats);
		dupdate();

		key = getkey().key;
		if(key == KEY_F1) tab = 0;
		if(key == KEY_F2) tab = 1;
		if(key == KEY_F3) tab = 2;
		if(key == KEY_F4) tab = 3;

		/* Action for list tab */
		if(tab == 0 && key == KEY_UP && list_scroll > 0)
			list_scroll--;
		if(tab == 0 && key == KEY_DOWN && list_scroll <
			(int)gint_driver_count() - list_max)
			list_scroll++;

		/* Actions for OS state tab */
		if((tab == 1 || tab == 2) && key == KEY_LEFT && selected_driver > 0)
			selected_driver--;
		if((tab == 1 || tab == 2) && key == KEY_RIGHT
			&& selected_driver < (int)gint_driver_count()-1)
			selected_driver++;

		/* Actions for the manual tab */
		if(tab == 3 && key == KEY_1)
			gint_world_switch(GINT_CALL_INC(&stats.world_switch_count));
		if(tab == 3 && key == KEY_2)
		{
			/* TODO: Should render next frame in advance for seamless return */
			stats.return_to_menu_count++;
			dupdate();

			gint_osmenu();
			/* Wait for KEY_2 to be released before calling next getkey() */
			while(keydown(KEY_2)) waitevent(NULL);
		}
		if(tab == 3 && key == KEY_3)
		{
			/* TODO: World switch with performance statistics */
		}
#endif
	}
}
