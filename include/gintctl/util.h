//---
//	gintctl:util - Utility functions
//---

#ifndef GINTCTL_UTIL
#define GINTCTL_UTIL

#include <stdarg.h>

//---
//	Platform disambiguation functions
//---

#ifdef FX9860G
#define _(fx,cg) fx
#endif

#ifdef FXCG50
#define _(fx,cg) cg
#endif

//---
//	Row manipulation functions
//---

/* row_title(): Render the main title */
void row_title(char const *format, ...);

/* row_print(): Formatted printing in a predefined row */
void row_print(int row, int x, char const *format, ...);

/* row_highlight(): Invert a row's pixels to highlight it */
void row_highlight(int row);

/* row_right(): Print at the last column of a row */
void row_right(int row, char const *character);

/* scrollbar(): Show a scrollbar */
void scrollbar(int offset, int length, int top, int bottom);

/* row_count(): Number of rows available to row_print() */
int row_count(void);

//---
//	General (x,y) printing
//---

/* print(): Formatted printing shorthand */
void print(int x, int y, char const *format, ...);

//---
//	F-key rendering
//---

#ifdef FXCG50

/* fkey_action(): A black-on-white F-key */
void fkey_action(int position, char const *text);

/* fkey_button(): A rectangular F-key */
void fkey_button(int position, char const *text);

/* fkey_menu(): A rectangular F-key with the bottom right corner removed */
void fkey_menu(int position, char const *text);

#endif /* FXCG50 */

#endif /* GINTCTL_UTIL */
