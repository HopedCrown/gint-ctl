#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/mpu/power.h>
#include <gint/mpu/spu.h>
#include <gint/mmu.h>
#include <gint/timer.h>
#include <gint/clock.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

/* From the official documentation teaser (SH7724 section 44):
   * DMA can send interrupts to DSP - try and get a flag raised!
   * Therefore DSP has an interrupt handler
   * "Bus bridge: Provides access to the SPU2 registers from the DSP."
     Therefore the DSP can access its own registers. Might be through pointers
     and memory accesses, but or through intrinsics of some sort (like ldc). */

enum { PAT_FF, PAT_32BIT, PAT_24BIT_HIGH, PAT_24BIT_LOW, PAT__MAX };

GUNUSED static u32 *PRAM0 = (void *)0xfe200000;
GUNUSED static u32 *XRAM0 = (void *)0xfe240000;
GUNUSED static u32 *YRAM0 = (void *)0xfe280000;
GUNUSED static u32 *PRAM1 = (void *)0xfe300000;
GUNUSED static u32 *XRAM1 = (void *)0xfe340000;
GUNUSED static u32 *YRAM1 = (void *)0xfe380000;

static u32 _RAMBUF[256 / 4];

static u32 *RAMBUF(void)
{
    u32 addr = (u32)&_RAMBUF;
    addr = (u32)mmu_uram() + (addr - 0x08100000);
    addr = (addr & 0x1fffffff) + 0xa0000000;
    return (void *)addr;
}

static void fill_area(u32 *area, int n, int pattern_id)
{
    static u32 const pattern_ff[1] = {
        0xffffffff,
    };
    static u32 const pattern_32bit[2] = {
        0x01234567, 0x89abcdef,
    };
    static u32 const pattern_24bit[16] = {
        0x00112200, 0x33445500, 0x66778800, 0x99aabb00,
        0xccddee00, 0xff001100, 0x22334400, 0x55667700,
        0x8899aa00, 0xbbccdd00, 0xeeff0000, 0x11223300,
        0x44556600, 0x77889900, 0xaabbcc00, 0xddeeff00,
    };

    u32 const *pattern = pattern_ff;
    int cycle_length = 1;
    int shr = 0;

    if(pattern_id == PAT_32BIT)
        pattern = pattern_32bit, cycle_length = 2;
    else if(pattern_id == PAT_24BIT_HIGH)
        pattern = pattern_24bit, cycle_length = 16;
    else if(pattern_id == PAT_24BIT_LOW)
        pattern = pattern_24bit, cycle_length = 16, shr = 8;

    for(int i = 0; i < n; i++)
        area[i] = pattern[i % cycle_length] >> shr;
}

// Note: this is incomplete, only translates userspace RAM.
static u32 ptr_to_physical(void const *ptr)
{
    u32 addr = (u32)ptr;

    if(addr >= 0x08100000 && addr < 0x08100000 + (1 << 20))
        addr = (u32)mmu_uram() + (addr - 0x08100000);

    if(addr >= 0x80000000 && addr < 0xc0000000)
        return addr & 0x1fffffff;
    else
        return addr;
}

static void run_dma(void *dst_ptr, void const *src_ptr, u32 TS, u32 TCR)
{
    u32 volatile *OR = &SH7305_DSP0.OR;
    spu_dsp_dma_t volatile *DMA = &SH7305_DSP0.DMA[0];

    // Try to reset internal state for piecewise transfers
    DMA->CHCR = 0;

    u32 dst = ptr_to_physical(dst_ptr);
    u32 src = ptr_to_physical(src_ptr);

    DMA->SBAR = src & 0xff800000;
    DMA->SAR  = src & 0x007ffffc;
    DMA->DBAR = dst & 0xff800000;
    DMA->DAR  = dst & 0x007ffffc;

    DMA->TCR = TCR;
    /* EC16=0 EC8=0 : No endianness conversion)
       DM=0 SM=0    : Both source and target auto-increment)
       LP=0 LPS=*   : No loop
       TE=* DE=1    : Enable transfer */
    DMA->CHCR = 0x00000001 + ((TS & 7) << 12);

    *OR = 1;
}

static void print_register(int x, int y, char const *name, u32 value)
{
    dprint(x + 12, y, C_BLACK, "%s:", name);
    dprint(x + 80, y, C_BLACK, "%08X", value);
}

static void render_show_dma(int dma_dir, int dma_TS)
{
    spu_dsp_t volatile *DSP = &SH7305_DSP0;
    spu_dsp_dma_t volatile *DMA = &SH7305_DSP0.DMA[0];

    int y = 20 - 12;
    dprint(6, y += 12, C_BLACK, "Registers of DSP0:");
    print_register(6, y += 12, "OR", DSP->OR);

    dprint(6, y += 12, C_BLACK, "Registers of DMA channel 0 of DSP0:");
    print_register(6, y += 12, "SBAR",  DMA->SBAR);
    print_register(6, y += 12, "SAR",   DMA->SAR);
    print_register(6, y += 12, "DBAR",  DMA->DBAR);
    print_register(6, y += 12, "DAR",   DMA->DAR);
    print_register(6, y += 12, "TCR",   DMA->TCR);
    print_register(6, y += 12, "SHPRI", DMA->SHPRI);
    print_register(6, y += 12, "CHCR",  DMA->CHCR);

    y += 12;
    dprint(6, y += 12, C_BLACK, "RAMBUF is at %p", RAMBUF());
    dprint(6, y += 12, C_BLACK, "F4: Choose value of TS: %d", dma_TS);
    dprint(6, y += 12, C_BLACK, "F5: Choose direction: %s",
        dma_dir == 0 ? "XRAM0 -> RAM" : "RAM -> XRAM0");
    dprint(6, y += 12, C_BLACK, "F6: Run DMA transfer!");
}

static void show_memory_block(int x, int y, u32 *area, int size)
{
    for(int i = 0; i < size / 4; i++)
        dprint(x + 80 * (i & 3), y + 12 * (i >> 2), C_BLACK, "%08X", area[i]);
}

static void render_show_memory(int memory_area, int memory_pattern)
{
    int y = 20;
    dprint(6, y, C_BLACK, "XRAM0 (64 bytes):");
    y += 12;
    show_memory_block(30, y, XRAM0, 64);
    y += 60;

    dprint(6, y, C_BLACK, "RAM buffer (64 bytes):");
    y += 12;
    show_memory_block(30, y, RAMBUF(), 64);
    y += 48;

    static char const *pattern_names[PAT__MAX] = {
        "All ff", "32-bit", "24-bit (high)", "24-bit (low)",
    };
    dprint(6, y += 12, C_BLACK, "F4: Select buffer: %s",
        memory_area == 0 ? "XRAM0" : "RAM");
    dprint(6, y += 12, C_BLACK, "F5: Select fill: %s",
        pattern_names[memory_pattern % PAT__MAX]);
    dprint(6, y += 12, C_BLACK, "F6: Fill buffer!");
}

static void reset_dsp0(void)
{
    spu_dsp_t volatile *DSP = &SH7305_DSP0;
    u32 *PRAM0 = (void *)0xfe200000;

    srand(77);
    for(int i = 0; i < 0x28000 / 4; i++)
        PRAM0[i] = rand();

    /* We will do a full reset to stage 1 */
    DSP->DSPCORERST = 1;
    /* Allow interrupts */
    DSP->IEMASKC = 0;
    DSP->IMASKC = 0;
    DSP->IEMASKD = 0;
    DSP->IMASKD = 0;
    /* Reset request (inverted?!) */
    DSP->DSPRST = 1;
    /* Wait a little bit, then lift the reset request */
    sleep_us_spin(10000);
    DSP->DSPRST = 0;
}

static void render_show_dsp(void)
{
    uint32_t volatile *PASCR = (void *)0xff000070;
    spu_dsp_t volatile *DSP = &SH7305_DSP0;

    int y = 20;
    dprint(6, y, C_BLACK, "MSTPCR2: %08x", SH7305_POWER.MSTPCR2.lword);
    dprint(198, y, C_BLACK, "PASCR: %08x", *PASCR);
    y += 12;
    dprint(6, y, C_BLACK, "DSPRST: %08x", DSP->DSPRST);
    dprint(198, y, C_BLACK, "DSPCORERST: %08x", DSP->DSPCORERST);
    y += 12;
    dprint(6, y, C_BLACK, "DSPHOLD: %08x", DSP->DSPHOLD);
    dprint(198, y, C_BLACK, "DSPRESTART: %08x", DSP->DSPRESTART);
    y += 12;
    dprint(6, y, C_BLACK, "IEMASKC: %08x", DSP->IEMASKC);
    dprint(198, y, C_BLACK, "IMASKC: %08x", DSP->IMASKC);
    y += 12;
    dprint(6, y, C_BLACK, "IEVENTC: %08x", DSP->IEVENTC);
    dprint(198, y, C_BLACK, "IEMASKD: %08x", DSP->IEMASKD);
    y += 12;
    dprint(6, y, C_BLACK, "IMASKD: %08x", DSP->IMASKD);
    dprint(198, y, C_BLACK, "IESETD: %08x", DSP->IESETD);
    y += 12;
    dprint(6, y, C_BLACK, "IECLRD: %08x", DSP->IECLRD);
    dprint(198, y, C_BLACK, "SPUSTS: %08x", DSP->SPUSTS);
    y += 12;

    static int i = 0;
    dprint(6, y, C_BLACK, "[%d]", ++i);

}

void gintctl_gint_spu_aux(int volatile *timeout)
{
    int current_tab = 0;
    int memory_area = 0;
    int memory_pattern = 0;
    int dma_dir = 0;
    int dma_TS = 0;
    int key = 0;

    while(key != KEY_EXIT)
    {
        dclear(C_WHITE);

        #if GINT_RENDER_RGB
        row_title("SPU DMA and control");
        fkey_menu(1, "DMA");
        fkey_menu(2, "MEMORY");
        fkey_menu(3, "DSP");
        #endif

        if(current_tab == 0) {
            render_show_dma(dma_dir, dma_TS);
            fkey_button(4, "DIR");
            fkey_button(5, "TS");
            fkey_action(6, "XFER");
        }
        if(current_tab == 1) {
            render_show_memory(memory_area, memory_pattern);
            fkey_button(4, "AREA");
            fkey_button(5, "PATTERN");
            fkey_action(6, "FILL");
        }
        if(current_tab == 2) {
            render_show_dsp();
            fkey_button(6, "RESET");
            fkey_button(5, "STOP");
        }

        dupdate();
        key = getkey_opt(GETKEY_DEFAULT, timeout).key;
        *timeout = 0;

        if(current_tab == 0) {
            if(key == KEY_F4)
                dma_dir = !dma_dir;
            if(key == KEY_F5)
                dma_TS = (dma_TS + 1) % 8;
            if(key == KEY_F6) {
                void *src = (dma_dir == 0) ? XRAM0 : RAMBUF();
                void *dst = (dma_dir == 0) ? RAMBUF() : XRAM0;
                int TCR = 8;
                if(dma_TS == 7) TCR = 9; // must be multiple of 3
                run_dma(dst, src, dma_TS, TCR);
            }
        }
        else if(current_tab == 1) {
            if(key == KEY_F4)
                memory_area = (memory_area + 1) % 2;
            if(key == KEY_F5)
                memory_pattern = (memory_pattern + 1) % PAT__MAX;
            if(key == KEY_F6) {
                fill_area(memory_area == 0 ? XRAM0 : RAMBUF(), 64,
                          memory_pattern);
            }
        }
        else if(current_tab == 2) {
            if(key == KEY_F6)
                reset_dsp0();
        }

        if(key == KEY_F1)
            current_tab = 0;
        if(key == KEY_F2)
            current_tab = 1;
        if(key == KEY_F3)
            current_tab = 2;
    }
}

/* gintctl_gint_spu(): SPU2 DMA and general control. */
void gintctl_gint_spu(void)
{
    extern int spu_zero(void);
    spu_zero();

    int volatile timeout = 0;
    int timer = timer_configure(TIMER_ANY, 200000 /* 200 ms */,
        GINT_CALL_SET(&timeout));
    timer_start(timer);

    u32 P0 = SH7305_SPU.PBANKC0;
    u32 P1 = SH7305_SPU.PBANKC1;
    u32 X0 = SH7305_SPU.XBANKC0;
    u32 X1 = SH7305_SPU.XBANKC1;

    /* Give some memory to both */
    SH7305_SPU.PBANKC0 = 0x07;
    SH7305_SPU.PBANKC1 = 0x18;
    SH7305_SPU.XBANKC0 = 0x0f;
    SH7305_SPU.XBANKC1 = 0x70;

    gintctl_gint_spu_aux(&timeout);

    SH7305_SPU.PBANKC0 = P0;
    SH7305_SPU.PBANKC1 = P1;
    SH7305_SPU.XBANKC0 = X0;
    SH7305_SPU.XBANKC1 = X1;

    timer_stop(timer);
}
