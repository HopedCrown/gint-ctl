#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/hardware.h>
#include <gint/dma.h>
#include <gint/mmu.h>

#include <gintctl/perf.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>

#include <gint/prof.h>
#include <string.h>
#include <stdlib.h>

#if GINT_RENDER_RGB

//---
// Functions for read/write access patterns
//---

/* Tight asm reads of different sizes. SPU2 memory only supports 32-bit */
extern void mem_read8          (void *mem, int size);
extern void mem_read16         (void *mem, int size);
extern void mem_read32         (void *mem, int size);
/* Tight asm writes of different sizes. SPU2 memory only supports 32-bit */
extern void mem_write8         (void *mem, int size);
extern void mem_write16        (void *mem, int size);
extern void mem_write32        (void *mem, int size);
/* Tight asm reads of 2 addresses; size is the total volume; no increment */
extern void mem_read8_alt      (void *mem1, void *mem2, int size);
extern void mem_read16_alt     (void *mem1, void *mem2, int size);
extern void mem_read32_alt     (void *mem1, void *mem2, int size);
/* Tight asm writes of 2 addresses; size is the total volume; no increment */
extern void mem_write8_alt     (void *mem1, void *mem2, int size);
extern void mem_write16_alt    (void *mem1, void *mem2, int size);
extern void mem_write32_alt    (void *mem1, void *mem2, int size);
/* Same using the DSP's XRAM addressing instructions (movx) */
extern void mem_dspx_read16    (void *mem, int size);
extern void mem_dspx_read32    (void *mem, int size);
extern void mem_dspx_write16   (void *mem, int size);
extern void mem_dspx_write32   (void *mem, int size);
/* Same with the DSP's external addressing instructions (movs) */
extern void mem_dsps_read16    (void *mem, int size);
extern void mem_dsps_read32    (void *mem, int size);
extern void mem_dsps_write16   (void *mem, int size);
extern void mem_dsps_write32   (void *mem, int size);
/* 32-byte-aligned dma_memset() */
extern void *dma_memset        (void *mem, uint32_t pattern, size_t size);

/* Copy with same-sized reads and writes (LS pipe saturated by unrolling) */
extern void mem_copy8          (void *dst, void *src, int size);
extern void mem_copy16         (void *dst, void *src, int size);
extern void mem_copy32         (void *dst, void *src, int size);
/* Same with DSP's XRAM -> YRAM addressing instructions (movx/movy) */
extern void mem_dspxy_copy16   (void *dst, void *src, int size);
extern void mem_dspxy_copy32   (void *dst, void *src, int size);
/* Copy using 32-byte-aligned DMA access in burst mode */
extern void *dma_memcpy        (void *dst, void const *src, size_t size);

//---
// Areas to check performance for
//---

#define READONLY       0x0001
#define ONLY32BIT      0x0002
#define DSPXRAM        0x0004
#define VIRTUAL        0x0008

GILRAM GALIGNED(32) static char ilram_buffer[0x800];
GXRAM  GALIGNED(32) static char xram_buffer[0x800];
GYRAM  GALIGNED(32) static char yram_buffer[0x800];
#define pram0_buffer ((void *)0xfe200000)

typedef struct
{
    void *pointer;
    int size;
    /* How many rounds per test, to compensate for small size */
    int rounds;
    /* Flags for which tests to perform */
    int flags;

} region_t;

/* Some pretty random selection of each region of interest */
region_t ROM_CF_MMU = { (void*)0x00300000, 2048, 16, READONLY | VIRTUAL };
region_t ROM_CU_MMU = { (void*)0x00300000, 65536, 1, READONLY | VIRTUAL };
region_t ROM_CF     = { (void*)0x80000000, 2048, 16, READONLY };
region_t ROM_CU     = { (void*)0x80000000, 65536, 1, READONLY };
region_t ROM_NC     = { (void*)0xa0000000, 2048, 16, READONLY };
region_t RAM_CF_MMU = { (void*)0x08100000, 2048, 16, READONLY };
region_t RAM_CU_MMU = { (void*)0x08100000, 65536, 1, READONLY };
region_t RAM_CF     = { (void*)0x8c200000, 2048, 16, 0 };
region_t RAM_CU     = { (void*)0x8c200000, 65536, 1, 0 };
region_t RAM_NC     = { (void*)0xac200000, 2048, 16, 0 };
region_t ILRAM      = {      ilram_buffer, 2048, 64, 0 };
region_t XRAM       = {       xram_buffer, 2048, 64, DSPXRAM };
region_t YRAM       = {       yram_buffer, 2048, 64, DSPXRAM };
region_t PRAM0      = {      pram0_buffer, 2048, 16, ONLY32BIT };

region_t const *REGIONS[] = {
    &ROM_CF_MMU, &ROM_CU_MMU, &ROM_CF, &ROM_CU, &ROM_NC,
    &RAM_CF_MMU, &RAM_CU_MMU, &RAM_CF, &RAM_CU, &RAM_NC,
    &ILRAM, &XRAM, &YRAM, &PRAM0,
};
char const *REGIONS_NAMES[] = {
    "ROM (cached, MMU)", "ROM (cached linear, MMU)",
    "ROM (cached, no MMU)", "ROM (cached linear, no MMU)",
    "ROM (uncached, no MMU)",
    "RAM (cached, MMU)", "RAM (cached linear, MMU)",
    "RAM (cached, no MMU)", "RAM (cached linear, no MMU)",
    "RAM (uncached, no MMU)",
    "ILRAM", "XRAM", "YRAM", "PRAM0",
};
#define REGIONS_COUNT ((int)(sizeof REGIONS / sizeof REGIONS[0]))

//---
// Result information
//---

typedef struct
{
    int mem_read8, mem_read16, mem_read32;
    int mem_read8_alt, mem_read16_alt, mem_read32_alt;
    int mem_write8, mem_write16, mem_write32;
    int mem_write8_alt, mem_write16_alt, mem_write32_alt;
    int dma_memset;

    union {
        struct {
            int mem_dspx_read16, mem_dspx_read32;
            int mem_dspx_write16, mem_dspx_write32;
        };
        struct {
            int mem_dsps_read16, mem_dsps_read32;
            int mem_dsps_write16, mem_dsps_write32;
        };
    };

} GPACKED(4) counters_t;

typedef struct
{
    /* In µs, counting all rounds */
    counters_t time;
    /* In kB/s overall */
    counters_t speed;

} GPACKED(4) info_t;

//---
// Running tests over a single region
//---

static void benchmark(region_t const *region, info_t *info)
{
    /* Initialize all times and rates to -1 */
    memset(info, 0xff, sizeof *info);

    int f = region->flags;
    int size = region->size;
    void *p1 = region->pointer;
    void *p2 = p1 + size / 2;

    /* Hack to switch page on XRAM/YRAM for reading and writing tests */
    if(f & DSPXRAM)
        p2 = (void *)((uint32_t)p1 ^ 0x00001000);

    if(~f & ONLY32BIT) {
        info->time.mem_read8 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_read8(p1, size);
        });
        info->time.mem_read8_alt = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_read8_alt(p1, p2, size);
        });
    }
    if(~f & ONLY32BIT) {
        info->time.mem_read16 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_read16(p1, size);
        });
        info->time.mem_read16_alt = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_read16_alt(p1, p2, size);
        });
    }
    info->time.mem_read32 = prof_exec({
        for(int i = 0; i < region->rounds; i++)
            mem_read32(p1, size);
    });
    info->time.mem_read32_alt = prof_exec({
        for(int i = 0; i < region->rounds; i++)
            mem_read32_alt(p1, p2, size);
    });

    if((~f & READONLY) && (~f & ONLY32BIT)) {
        info->time.mem_write8 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_write8(p1, size);
        });
        info->time.mem_write8_alt = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_write8_alt(p1, p2, size);
        });
    }
    if((~f & READONLY) && (~f & ONLY32BIT)) {
        info->time.mem_write16 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_write16(p1, size);
        });
        info->time.mem_write16_alt = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_write16_alt(p1, p2, size);
        });
    }
    if(~f & READONLY) {
        info->time.mem_write32 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_write32(p1, size);
        });
        info->time.mem_write32_alt = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_write32_alt(p1, p2, size);
        });
    }

    if((~f & READONLY) && (~f & VIRTUAL))
        info->time.dma_memset = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                dma_memset(p1, 0, size);
        });

    if(f & DSPXRAM) {
        info->time.mem_dspx_read16 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_dspx_read16(p1, size);
        });
        info->time.mem_dspx_read32 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_dspx_read32(p1, size);
        });
        info->time.mem_dspx_write16 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_dspx_write16(p1, size);
        });
        info->time.mem_dspx_write32 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_dspx_write32(p1, size);
        });
    }

    if((~f & DSPXRAM) && (~f & ONLY32BIT))
        info->time.mem_dsps_read16 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_dsps_read16(p1, size);
        });
    if(~f & DSPXRAM)
        info->time.mem_dsps_read32 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_dsps_read32(p1, size);
        });
    if((~f & DSPXRAM) && (~f & ONLY32BIT) && (~f & READONLY))
        info->time.mem_dsps_write16 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_dsps_write16(p1, size);
        });
    if((~f & DSPXRAM) && (~f & READONLY))
        info->time.mem_dsps_write32 = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                mem_dsps_write32(p1, size);
        });

    if(~f & READONLY)
        info->time.dma_memset = prof_exec({
            for(int i = 0; i < region->rounds; i++)
                dma_memset(p1, 0, size);
        });

    /* Cheeky method to read all ints in such a packed struct */
    int *time   = (int *)&info->time;
    int *speed  = (int *)&info->speed;
    int entry_count = sizeof(counters_t) / sizeof(int);

    /* Conversion from [µs for every size bytes] to [kB for every 1 second] */
    uint64_t conv = region->size * 1000 * region->rounds;

    for(int i = 0; i < entry_count; i++) {
        if(time[i] != -1)
            speed[i] = conv / time[i];
    }
}

//---
// Main interface
//---

void print_speed(int x, int y, int unit, int us, int kBps)
{
    if(us == -1 && kBps == -1) {
        dprint_opt(x, y, C_BLACK, C_NONE, DTEXT_CENTER, DTEXT_TOP, "-");
        return;
    }

    if(unit == 1) {
        dprint_opt(x, y, C_BLACK, C_NONE, DTEXT_CENTER, DTEXT_TOP, "%d us",us);
    }
    else {
        char const *fmt;
        if(kBps >= 100000) {
            fmt = "%.1D M/s";
            kBps /= 100;
        }
        else {
            fmt = "%.2D M/s";
            kBps /= 10;
        }
        dprint_opt(x, y, C_BLACK, C_NONE, DTEXT_CENTER, DTEXT_TOP, fmt, kBps);
    }
}
#define print_speed(x, y, unit, FIELD) \
    print_speed(x, y, unit, \
        info[selection].time.FIELD, info[selection].speed.FIELD)

/* gintctl_perf_memory(): Memory primitives and reading/writing speed */
void gintctl_perf_memory(void)
{
    // TODO: Also test copy speed
    int key=0, selection=0, unit=0;

    info_t *info = malloc(REGIONS_COUNT * sizeof *info);
    memset(info, 0xff, REGIONS_COUNT * sizeof *info);

    while(key != KEY_EXIT) {
        dclear(C_WHITE);
        row_title("Memory read/write speed");
        row_print(1, 1, "%s", REGIONS_NAMES[selection]);
        dprint_opt(DWIDTH-40, row_y(1), C_BLACK, C_NONE, DTEXT_CENTER,
            DTEXT_TOP, "%d/%d", selection+1, REGIONS_COUNT);
        row_print(2, 1, "%p (%d bytes, %d rounds)",
            REGIONS[selection]->pointer,
            REGIONS[selection]->size,
            REGIONS[selection]->rounds);

        dprint_opt(150, 53, C_BLACK, C_NONE, DTEXT_CENTER, DTEXT_TOP,
            "(8-bit)");
        dprint_opt(240, 53, C_BLACK, C_NONE, DTEXT_CENTER, DTEXT_TOP,
            "(16-bit)");
        dprint_opt(330, 53, C_BLACK, C_NONE, DTEXT_CENTER, DTEXT_TOP,
            "(32-bit)");

        dprint(6, 74, C_BLACK, "CPU read seq:");
        print_speed(155, 74, unit, mem_read8);
        print_speed(245, 74, unit, mem_read16);
        print_speed(335, 74, unit, mem_read32);

        dprint(6, 88, C_BLACK, "CPU read alt:");
        print_speed(155, 88, unit, mem_read8_alt);
        print_speed(245, 88, unit, mem_read16_alt);
        print_speed(335, 88, unit, mem_read32_alt);

        dprint(6, 102, C_BLACK, "CPU write seq:");
        print_speed(155, 102, unit, mem_write8);
        print_speed(245, 102, unit, mem_write16);
        print_speed(335, 102, unit, mem_write32);

        dprint(6, 116, C_BLACK, "CPU write alt:");
        print_speed(155, 116, unit, mem_write8_alt);
        print_speed(245, 116, unit, mem_write16_alt);
        print_speed(335, 116, unit, mem_write32_alt);

        dprint(6, 130, C_BLACK, "DSP read seq:");
        print_speed(245, 130, unit, mem_dsps_read16);
        print_speed(335, 130, unit, mem_dsps_read32);

        dprint(6, 158, C_BLACK, "DSP write seq:");
        print_speed(245, 158, unit, mem_dsps_write16);
        print_speed(335, 158, unit, mem_dsps_write32);

        dprint(6, 186, C_BLACK, "dma_memset:");
        print_speed(155, 186, unit, dma_memset);

        if(selection > 0)
            dprint(DWIDTH-72, row_y(1), C_BLACK, "<");
        if(selection < REGIONS_COUNT - 1)
            dprint(DWIDTH-12, row_y(1), C_BLACK, ">");
        fkey_button(1, "UNIT");
        fkey_button(6, "RUN ALL");
        dupdate();

        key = getkey().key;
        if(key == KEY_LEFT && selection > 0)
            selection--;
        if(key == KEY_RIGHT && selection < REGIONS_COUNT-1)
            selection++;
        if(key == KEY_F1)
            unit = !unit;
        if(key == KEY_F6) {
            for(int i = 0; i < REGIONS_COUNT; i++)
                benchmark(REGIONS[i], &info[i]);
        }
    }

    free(info);
}

#endif
