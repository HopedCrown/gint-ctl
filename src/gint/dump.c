#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/bfile.h>
#include <gint/std/stdio.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

struct region {
	char const *name;
	uint32_t start;
	uint32_t end;
};

static struct region const regs[] = {
	{ "ROM",    0x80000000, 0x807fffff },
	{ "RAM",    0x88000000, 0x88040000 },
	{ "RS",     0xfd800000, 0xfd8007ff },
};

static void filename(char *out, int region_id, int rom_segment)
{
	if(region_id == 0)
	{
		sprintf(out, "ROM-%d.bin", rom_segment);
	}
	else
	{
		sprintf(out, "%s.bin", regs[region_id].name);
	}
}

#ifdef FX9860G
static void dump(int region_id, int rom_segment)
{
	uint32_t start = regs[region_id].start;
	int size = regs[region_id].end + 1 - start;

	if(region_id == 0)
	{
		start += rom_segment << 20;
		size = 1 << 20;
	}

	/* Make sure the size is *even* */
	size &= ~1;

	uint16_t file[30];
	char file_u8[30] = "\\\\fls0\\";

	filename(file_u8 + 7, region_id, rom_segment);
	for(int i = 0; i < 30; i++) file[i] = file_u8[i];

	BFile_Remove(file);
	BFile_Create(file, BFile_File, &size);

	int fd = BFile_Open(file, BFile_WriteOnly);
	BFile_Write(fd, (void *)start, size);
	BFile_Close(fd);
}
#endif

/* gintctl_gint_dump(): Dump memory to filesystem */
void gintctl_gint_dump(void)
{
	int region_id=0, rom_segment=0;
	char output_file[20];

	int key = 0;
	while(key != KEY_EXIT)
	{
		filename(output_file, region_id, rom_segment);

		dclear(C_WHITE);

		#ifdef FX9860G
		row_print(1, 1, "Memory dump");

		row_print(3, 1, "Region:  %s", regs[region_id].name);
		if(region_id == 0)
			row_print(4, 1, "Segment: %d", rom_segment);
		row_print(5, 1, "File:    %s", output_file);

		extern image_t img_opt_dump;
		dimage(0, 56, &img_opt_dump);
		#endif

		#ifdef FXCG50
		row_title("Memory dump to filesystem");

		fkey_button(1, "ROM");
		fkey_button(2, "RAM");
		fkey_button(3, "RS");
//		fkey_action(6, "DUMP");
		#endif

		dupdate();

		key = getkey().key;
		if(key == KEY_F1)
		{
			if(region_id == 0) rom_segment = (rom_segment + 1) % 8;
			region_id = 0;
		}
		if(key == KEY_F2) region_id = 1;
		if(key == KEY_F3) region_id = 2;

		#ifdef FX9860G
		if(key == KEY_F6) dump(region_id, rom_segment);
		#endif
	}
}
