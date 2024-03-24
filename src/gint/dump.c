#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/bfile.h>
#include <gint/gint.h>
#include <gint/hardware.h>
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

#include <stdio.h>

struct region {
	char const *name;
	uint32_t start;
	uint32_t end;
	int segment_count;
};

static struct region const regs[] = {
	#if GINT_HW_FX
	{ "ROM",    0x80000000, 0x807fffff, 8 },
	{ "RAM",    0x88000000, 0x88040000, 1 },
	{ "RS",     0xfd800000, 0xfd803fff, 1 },
	#elif GINT_HW_CG
	{ "ROM",    0x80000000, 0x81ffffff, 32 },
	{ "RAM_88", 0x88000000, 0x881fffff, 2 },
	{ "RAM_8C", 0x8c000000, 0x8c7fffff, 8 },
	{ "RS",     0xfd800000, 0xfd803fff, 1 },
	#endif
};

static void switch_dump(int region, int segment, char *filename, int *retcode)
{
	uint32_t start = regs[region].start;
	int size = regs[region].end + 1 - start;

	/* For segmented regions, use blocks of 1M */
	if(regs[region].segment_count > 1)
	{
		start += segment << 20;
		size = 1 << 20;
	}

	/* Make sure the size is *even* */
	size &= ~1;

	uint16_t file[30] = { 0 };
	for(int i = 0; i < 30; i++) file[i] = filename[i];

	*retcode = 1;

	int x = BFile_Remove(file);
	if(x < 0 && x != -1) { *retcode = x; return; }

	x = BFile_Create(file, BFile_File, &size);
	if(x < 0) { *retcode = x; return; }

	int fd = BFile_Open(file, BFile_WriteOnly);
	if(fd < 0) { *retcode = fd; return; }

	x = BFile_Write(fd, (void *)start, size);
	if(x < 0) { *retcode = x; return; }

	BFile_Close(fd);
}

static int do_dump_smem(int region, int segment)
{
	char filename[30];
	int retcode = 0;

	sprintf(filename, "\\\\fls0\\%s%02x.bin", regs[region].name, segment);
	gint_world_switch(GINT_CALL(switch_dump,region,segment,filename,&retcode));

	return retcode;
}

static void do_dump_usb(int region)
{
	bool open = usb_is_open();
	if(!open) {
		usb_interface_t const *interfaces[] = { &usb_ff_bulk, NULL };
		usb_open(interfaces, GINT_CALL_NULL);
		usb_open_wait();
	}

	int size = regs[region].end - regs[region].start + 1;

	usb_fxlink_header_t header;
	usb_fxlink_fill_header(&header, "gintctl", "dump", size);

	int pipe = usb_ff_bulk_output();
	usb_write_sync(pipe, &header, sizeof header, false);
	usb_write_sync(pipe, (void *)regs[region].start, size, false);
	usb_commit_sync(pipe);

	/* Close the USB link if it wasn't open before */
	if(!open) usb_close();
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

		#if GINT_RENDER_MONO
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

		#if GINT_RENDER_RGB
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
		fkey_button(2, "RAM_88");
		fkey_button(3, "RAM_8C");
		fkey_button(4, "RS");
		fkey_action(5, "USB");
		fkey_action(6, "SMEM");
		#endif

		dupdate();

		key = getkey().key;

		int select = -1;
		if(key == KEY_F1) select = 0;
		if(key == KEY_F2) select = 1;
		if(key == KEY_F3) select = 2;
		if(key == KEY_F4 && _(0,1)) select = 3;

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
		if(key == KEY_F5) do_dump_usb(region);
		if(key == KEY_F6) retcode = do_dump_smem(region, segment);
	}
}
