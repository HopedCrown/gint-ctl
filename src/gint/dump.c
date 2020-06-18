#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/bfile.h>
#include <gint/gint.h>
#include <gint/std/stdio.h>
#include <gint/hardware.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

struct region {
	char const *name;
	uint32_t start;
	uint32_t end;
	int segment_count;
};

struct {
	int region;
	int segment;
	char filename[30];
	int retcode;
} dump;

static struct region const regs[] = {
	#ifdef FX9860G
	{ "ROM",    0x80000000, 0x807fffff, 8 },
	{ "RAM",    0x88000000, 0x88040000, 1 },
	{ "RS",     0xfd800000, 0xfd8007ff, 1 },
	#endif

	#ifdef FXCG50
	{ "ROM",    0x80000000, 0x81ffffff, 32 },
	{ "RAM",    0x88000000, 0x881fffff, 2 },
	{ "RS",     0xfd800000, 0xfd8007ff, 1 },
	#endif
};

static void switch_dump(void)
{
	uint32_t start = regs[dump.region].start;
	int size = regs[dump.region].end + 1 - start;

	/* For segmented regions, use blocks of 1M */
	if(regs[dump.region].segment_count > 1)
	{
		start += dump.segment << 20;
		size = 1 << 20;
	}

	/* Make sure the size is *even* */
	size &= ~1;

	uint16_t file[30] = { 0 };
	for(int i = 0; i < 30; i++) file[i] = dump.filename[i];

	dump.retcode = 1;

	int x = BFile_Remove(file);
	if(x < 0 && x != -1) { dump.retcode = x; return; }

	x = BFile_Create(file, BFile_File, &size);
	if(x < 0) { dump.retcode = x; return; }

	int fd = BFile_Open(file, BFile_WriteOnly);
	if(fd < 0) { dump.retcode = fd; return; }

	x = BFile_Write(fd, (void *)start, size);
	if(x < 0) { dump.retcode = x; return; }

	BFile_Close(fd);
}

static int do_dump(int region, int segment)
{
	/* Pass around parameters through the global variable */
	dump.region = region;
	dump.segment = segment;
	sprintf(dump.filename, "\\\\fls0\\%s%02x.bin", regs[region].name,
		segment);

	gint_switch(switch_dump);
	return dump.retcode;
}

/* gintctl_gint_dump(): Dump memory to filesystem */
void gintctl_gint_dump(void)
{
	int region=0, segment=0;
	char filename[30];
	int retcode = 0;

	int key = 0;
	while(key != KEY_EXIT)
	{
		sprintf(filename, "%s%02x.bin", regs[region].name, segment);

		dclear(C_WHITE);

		#ifdef FX9860G
		row_print(1, 1, "Memory dump");

		row_print(3, 1, "Region:  %s", regs[region].name);
		row_print(4, 1, "Segment: %d (total %d)", segment,
			regs[region].segment_count);
		row_print(5, 1, "File:    %s", filename);

		extern bopti_image_t img_opt_dump;
		dimage(0, 56, &img_opt_dump);

		if(retcode == 1) dprint(77, 56, C_BLACK, "Done!");
		if(retcode < 0)  dprint(77, 56, C_BLACK, "E%d",retcode);
		#endif

		#ifdef FXCG50
		row_title("Memory dump to filesystem");

		row_print(1, 1, "Region:");
		row_print(2, 1, "Segment:");
		row_print(3, 1, "File:");

		row_print(1, 10, "%s", regs[region].name);
		row_print(2, 10, "%d (total %d)", segment,
			regs[region].segment_count);
		row_print(3, 10, "%s", filename);

		if(retcode == 1) row_print(5, 1, "Done!");
		if(retcode < 0)  row_print(5, 1, "Error %d", retcode);

		fkey_button(1, "ROM");
		fkey_button(2, "RAM");
		fkey_button(3, "RS");
		fkey_action(6, "DUMP");
		#endif

		dupdate();

		key = getkey().key;

		int select = -1;
		if(key == KEY_F1) select = 0;
		if(key == KEY_F2) select = 1;
		if(key == KEY_F3) select = 2;

		if(select >= 0 && select == region)
		{
			segment = (segment + 1) % regs[region].segment_count;
		}
		else if(select >= 0 && select != region)
		{
			region = select;
			segment = 0;
		}

		retcode = 0;
		if(key == KEY_F6) retcode = do_dump(region, segment);
	}
}
