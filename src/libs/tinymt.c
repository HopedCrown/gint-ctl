#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/std/stdlib.h>

#include <gintctl/libs.h>
#include <gintctl/util.h>

void gintctl_libs_tinymt(void)
{
	int key = 0;

	uint32_t seed = 0xdeadbeef;
	srand(seed);
	uint32_t values[32];
	for(int i = 0; i < 32; i++) values[i] = rand();

	dclear(C_WHITE);

	#ifdef FX9860G
	row_print(1, 1, "TinyMT random");

	row_print(2, 1, "Seed: %08X", seed);
	for(int i = 0; i < 12; i++)
		row_print(3+(i >> 1), (i&1)?13:2, "%08X", values[i]);
	#endif

	#ifdef FXCG50
	row_title("TinyMT random number generation");
	row_print(1, 1, "Seed: %08X", seed);
	row_print(3, 1, "First values:");

	for(int i = 0; i < 32; i++)
		row_print(4+(i >> 2), 2+12*(i&3), "%08X", values[i]);
	#endif

	dupdate();

	while(key != KEY_EXIT) key = getkey().key;
}
