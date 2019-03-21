#include <gint/display.h>

/* Drawing functions:
   * ...
   Keyboard tests:
   * ...
   Timer tests:
   * Do more in-depth things than previous application
   * Stress testing for number of interrupts
   * ...
   Boot log:
   * Displays boot log on full screen
   * F1 for details (get live explanation and detect problems)
   * F2 to save to file */

void locate(int x, int y, const char *str)
{
#ifdef FX9860G
	dtext(6*(x-1), 7*(y-1), str);
#else
	PrintXY(x, y, str - 2, 0, 0);
#endif
}
