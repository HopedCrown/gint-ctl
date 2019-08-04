//---
//	gintctl:gint - gint feature tests
//---

#ifndef GINTCTL_GINT
#define GINTCTL_GINT

/* gintctl_gint_hardware(): Detected hardware configuration */
void gintctl_gint_hardware(void);

/* gintctl_gint_timer(): Show the timer status in real-time */
void gintctl_gint_timer(void);

/* gintctl_gint_bopti(): Test image rendering */
void gintctl_gint_bopti(void);

#ifdef FX9860G

/* gintctl_gint_gray(): Gray engine tuning */
void gintctl_gint_gray(void);

/* gintctl_gint_grayrender(): Gray rendering functions */
void gintctl_gint_grayrender(void);

#endif /* FX9860G */

#endif /* GINTCTL_GINT */
