//---
//	gintctl:gint - gint feature tests
//---

#ifndef GINTCTL_GINT
#define GINTCTL_GINT

/* gintctl_gint_cpumem(): Detected CPU and memory configuration */
void gintctl_gint_cpumem(void);

/* gintctl_gint_ram(): Determine the size of some memory areas */
void gintctl_gint_ram(void);

/* gintctl_gint_dsp(): DSP initialization and configuration */
void gintctl_gint_dsp(void);

/* gintctl_gint_spuram(): SPU memory access, banking, and DMA */
void gintctl_gint_spuram(void);

/* gintctl_gint_dump(): Dump memory to filesystem */
void gintctl_gint_dump(void);

/* gintctl_gint_drivers(): Test the gint driver logic and world switch */
void gintctl_gint_drivers(void);

/* gintctl_gint_tlb(): TLB miss handler and TLB management */
void gintctl_gint_tlb(void);

/* gintctl_gint_overclock(): Clock speed detection and setting */
void gintctl_gint_overclock(void);

/* gintct_gint_keyboard: Real-time keyboard visualization */
void gintctl_gint_keyboard(void);

/* gintctl_gint_timer(): Show the timer status in real-time */
void gintctl_gint_timer(void);

/* gintctl_gint_timer_callbacks(): Stunts in the environment of callbacks */
void gintctl_gint_timer_callbacks(void);

/* gintctl_gint_dma(): Test the Direct Access Memory Controller */
void gintctl_gint_dma(void);

/* gintctl_gint_rtc(): Configure RTC and check timer speed */
void gintctl_gint_rtc(void);

/* gintctl_gint_render(): Test basic rendering functions */
void gintctl_gint_render(void);

/* gintctl_gint_image(): Test image rendering */
void gintctl_gint_image(void);

/* gintctl_gint_topti(): Test text rendering */
void gintctl_gint_topti(void);

/* gintctl_gint_kmalloc(): Dynamic memory allocator */
void gintctl_gint_kmalloc(void);

/* gintctl_gint_usb(): USB communication */
void gintctl_gint_usb(void);

#ifdef FX9860G

/* gintctl_gint_gray(): Gray engine tuning */
void gintctl_gint_gray(void);

/* gintctl_gint_grayrender(): Gray rendering functions */
void gintctl_gint_grayrender(void);

#endif /* FX9860G */

#endif /* GINTCTL_GINT */
