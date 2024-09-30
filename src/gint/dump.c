#include <gint/config.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/bfile.h>
#include <gint/gint.h>
#include <gint/hardware.h>
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>

#include <gintctl/config.h>
#include <gintctl/gint.h>
#include <gintctl/ui.h>

#include <stdio.h>
#include <string.h>

#define ENABLE_FS_DUMP !GINT_HW_CP

#if ENABLE_FS_DUMP
# define CONFIG_FKEY_FSDUMP "#SMEM"
#else
# define CONFIG_FKEY_FSDUMP ""
#endif

#if GINTCTL_ENABLE_USB
# define CONFIG_FKEY_USBDUMP "#USB"
#else
# define CONFIG_FKEY_USBDUMP ""
#endif

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

#if ENABLE_FS_DUMP
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
#endif

static void do_dump_usb(int region)
{
#if GINTCTL_ENABLE_USB
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
#endif
}

void generate_filename(char *filename, int region, int segment)
{
	sprintf(filename, "%s%02x.bin", regs[region].name, segment);
}

void generate_label_text(
	jlabel *label, int region, int segment, char const *filename, int rc)
{
	char rc_str[16] = "";
	if(rc == 1) strcpy(rc_str, "DONE");
	if(rc < 0) sprintf(rc_str, _("E", "Error ") "%d", rc);
	jlabel_asprintf(label,
		"Region: %s\n"
		"Segment: %d (total %d)\n"
		"File: %s\n"
		_("","\n") "%s",
		regs[region].name, segment, regs[region].segment_count, filename,
		rc_str);
}

/* gintctl_gint_dump(): Dump memory to filesystem */
void gintctl_gint_dump(void)
{
	int region=0, segment=0;
	char filename[30];
	int rc = 0;

	extern bopti_image_t img_opt_dump;
	gscreen *s = gscreen_create2("Memory dump", &img_opt_dump,
			"Memory dump to USB/filesystem",
			"@ROM;@RAM_88;@RAM_8C;@RS;" CONFIG_FKEY_USBDUMP ";"
			CONFIG_FKEY_FSDUMP, NULL);
	gintctl_scene_push(s);

	jlabel *label = jlabel_create("<info>", NULL);
	generate_filename(filename, region, segment);
	generate_label_text(label, region, segment, filename, rc);
	gscreen_add_tab(s, label, NULL);

	while(true)
	{
		jevent e = jscene_run(gintctl_scene());
		if(jevent_is_press(e, KEY_EXIT))
			break;

		int select = -1;
		bool changed = false;
		if(e.type == JFKEYS_TRIGGERED && e.data == 0) select = 0;
		if(e.type == JFKEYS_TRIGGERED && e.data == 1) select = 1;
		if(e.type == JFKEYS_TRIGGERED && e.data == 2) select = _(3,2);
		if(e.type == JFKEYS_TRIGGERED && e.data == 3 && _(0,1)) select = 3;

		if(select >= 0 && select == region)
		{
			segment = (segment + 1) % regs[region].segment_count;
			changed = true;
		}
		else if(select >= 0 && select != region)
		{
			region = select;
			segment = 0;
			changed = true;
		}

		rc = 0;
		if(e.type == JFKEYS_TRIGGERED && e.data == 4) {
			do_dump_usb(region);
			changed = true;
		}
		if(e.type == JFKEYS_TRIGGERED && e.data == 5 && ENABLE_FS_DUMP) {
			rc = do_dump_smem(region, segment);
			changed = true;
		}

		if(changed)
		{
			generate_filename(filename, region, segment);
			generate_label_text(label, region, segment, filename, rc);
		}
	}
}
