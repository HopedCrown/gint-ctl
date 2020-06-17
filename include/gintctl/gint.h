//---
//	gintctl:gint - gint feature tests
//---

#ifndef GINTCTL_GINT
#define GINTCTL_GINT

/* gintctl_gint_hardware(): Detected hardware configuration */
void gintctl_gint_hardware(void);

/* gintctl_gint_ram(): Determine the size of some memory areas */
void gintctl_gint_ram(void);

/* gintctl_gint_dump(): Dump memory to filesystem */
void gintctl_gint_dump(void);

/* gintctl_gint_switch(): Test the gint switch-in-out procedures */
void gintctl_gint_switch(void);

/* gintctl_gint_tlb(): TLB miss handler and TLB management */
void gintctl_gint_tlb(void);

/* gintct_gint_keybaord: Real-time keyboard visualization */
void gintctl_gint_keyboard(void);

/* gintctl_gint_timer(): Show the timer status in real-time */
void gintctl_gint_timer(void);

/* gintctl_gint_timer_callbacks(): Stunts in the environment of callbacks */
void gintctl_gint_timer_callbacks(void);

/* gintctl_gint_bopti(): Test image rendering */
void gintctl_gint_bopti(void);

#ifdef FXCG50

/* gintctl_gint_dma(): Test the Direct Access Memory Controller */
void gintctl_gint_dma(void);

#endif /* FXCG50 */

#ifdef FX9860G

/* gintctl_gint_gray(): Gray engine tuning */
void gintctl_gint_gray(void);

/* gintctl_gint_grayrender(): Gray rendering functions */
void gintctl_gint_grayrender(void);

#endif /* FX9860G */

/* gintctl_gint_printf(): printf() function */
void gintctl_gint_printf(void);

#endif /* GINTCTL_GINT */
