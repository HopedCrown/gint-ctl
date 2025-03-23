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

#include <justui/jpainted.h>

#include <stdio.h>
#include <string.h>

//---
// State management
//---

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

struct paint_params {
	gint_world_t world;
	int i;
};

static void draw_state(int x, int y, struct paint_params *params)
{
	gint_world_t world = params->world;
	int i = params->i;
	font_t const *old_font = dfont(_(&font_mini, dfont_default()));

	if(i > 0)
		dprint(_(86,10), _(57,row_y(1)), C_BLACK, "<");
	if(i < (int)gint_driver_count() - 1)
		dprint(DWIDTH - _(4,16), _(57,row_y(1)), C_BLACK, ">");

	dprint_opt(_(106, DWIDTH / 2), _(57, row_y(1)), C_BLACK, C_NONE,
		DTEXT_CENTER, DTEXT_TOP, "%s (%s)", gint_drivers[i].name,
		world == gint_world_os ? "OS" : "gint");

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

static void draw_manual(jlabel *label, struct switch_stats *stats)
{
	#if GINT_RENDER_MONO
	jlabel_asprintf(label,
		"World switches: %d\n"
		"Return-to-menu: %d\n"
		"\n" // "Switch time: %d µs\n", stats->world_switch_time
		"[1]: World switch\n"
		"[2]: Return-to-menu\n"
		"[3]: Measure perf (TODO)",
		stats->world_switch_count,
		stats->return_to_menu_count);
	#endif

	#if GINT_RENDER_RGB
	jlabel_asprintf(label,
		"World switches performed: %d\n"
		"Return-to-menu performed: %d\n"
		"\n" // "Switch time: %d µs\n", stats->world_switch_time
		"[1]: Standard world switch\n"
		"[2]: Return-to-menu with gint_osmenu()\n"
		"[3]: World switch with shared libprof (TODO)",
		stats->world_switch_count,
		stats->return_to_menu_count);
	#endif
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
		"Drivers and world switches", "@DRIVERS;@SWITCH;;;;|#WORLD", NULL);
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

	struct paint_params paint_params;
	paint_params.world = gint_world_os;
	paint_params.i = 0;
	jpainted *painted_details = jpainted_create(draw_state, &paint_params,
		0, 0, NULL);
	gscreen_add_tab(s, painted_details, NULL);
	gscreen_set_tab_fkeys_level(s, 1, 1);

	jlabel *label_switches = jlabel_create("<switches>", NULL);
	gscreen_add_tab(s, label_switches, NULL);

	while(key != KEY_EXIT)
	{
		jevent e = jscene_run(gintctl_scene());
		int Fkey = (e.type == JFKEYS_TRIGGERED) ? e.data + 1 : -1;
		int Tab = gscreen_current_tab(s);
		int Key = e.type == JWIDGET_KEY &&
		          (e.key.type == KEYEV_DOWN || e.key.type == KEYEV_HOLD) ?
		          e.key.key : 0;
		bool repaint_switches = false;

		if(Tab == 1 && Key == KEY_EXIT)
			gscreen_show_tab(s, 0);
		else if(Key == KEY_EXIT)
			break;

		if(Tab == 1 && (Key == KEY_LEFT || Key == KEY_RIGHT)) {
		   	int amount = (Key == KEY_LEFT) ? -1 : +1;
			int cursor = gtable_select_move(table_drv, amount);
			paint_params.i = cursor;
			s->widget.update = 1;
		}

		if(e.type == GTABLE_ROW_TRIGGERED) {
			paint_params.i = e.data;
			gscreen_show_tab(s, 1);
		}

		if(Tab == 1 && Fkey == 1) {
			paint_params.world = (paint_params.world == gint_world_os) ?
				gint_world_addin : gint_world_os;
			s->widget.update = 1;
		}
		else if(Fkey == 1)
			gscreen_show_tab(s, 0);

		if(Tab != 1 && Fkey == 2) {
			gscreen_show_tab(s, 2);
			repaint_switches = true;
		}

		/* Actions for the manual tab */
		if(Tab == 2 && Key == KEY_1) {
			gint_world_switch(GINT_CALL_INC(&stats.world_switch_count));
			repaint_switches = true;
		}
		if(Tab == 2 && Key == KEY_2) {
			/* TODO: Should render next frame in advance for seamless return */
			stats.return_to_menu_count++;
			dupdate();

			gint_osmenu();
			/* Wait for KEY_2 to be released before calling next getkey() */
			while(keydown(KEY_2)) waitevent(NULL);
			repaint_switches = true;
		}
		if(Tab == 2 && Key == KEY_3) {
			/* TODO: World switch with performance statistics */
			repaint_switches = true;
		}

		if(repaint_switches)
			draw_manual(label_switches, &stats);
	}
}
