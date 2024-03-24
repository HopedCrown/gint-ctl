#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#if GINT_HW_CG && GINT_RENDER_RGB

extern void dsp_ldrc(void);
extern int dsp_padd(int x, int y);
extern int dsp_cpumul(int x, int y);
extern int dsp_loop(int x);

/* gintctl_gint_dsp(): DSP initialization and configuration */
void gintctl_gint_dsp(void)
{
	int key = 0;

	int padd_x=27, padd_y=15;
	int padd_done = 0;
	int padd_result = 0;

	int mul_x=12, mul_y=14;
	int mul_done = 0;
	int mul_result = 0;

	int loop_x = 16;
	int loop_done = 0;
	int loop_result = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);
		row_title("DSP initialization and basic instructions");

		row_print(1, 1, "F1: LDRC instruction");
		row_print(2, 1, "F2: PADD instruction");
		if(!padd_done)
		{
			row_print(3, 3, "%d + %d = (not tested yet)", padd_x,
				padd_y);
		}
		else
		{
			row_print(3, 3, "%d + %d = %d", padd_x, padd_y,
				padd_result);
		}
		row_print(4, 1, "F3: Multiplication by CPU");
		if(!mul_done)
		{
			row_print(5, 3, "%d * %d = (not tested yet)", mul_x,
				mul_y);
		}
		else
		{
			row_print(5, 3, "%d * %d = %d", mul_x, mul_y,
				mul_result);
		}
		row_print(6, 1, "F4: Repeat loop");
		if(!loop_done)
		{
			row_print(7, 3, "%d times 1 = (not tested yet)",
				loop_x);
		}
		else
		{
			row_print(7, 3, "%d times 1 = %d", loop_x,
				loop_result);
		}

		fkey_button(1, "LDRC");
		fkey_button(2, "PADD");
		fkey_button(3, "PMULS");
		fkey_button(4, "LOOP");

		dupdate();
		key = getkey().key;

		if(key == KEY_F1)
		{
			dsp_ldrc();
		}
		if(key == KEY_F2)
		{
			padd_result = dsp_padd(padd_x, padd_y);
			padd_done = 1;
		}
		if(key == KEY_F3)
		{
			mul_result = dsp_cpumul(mul_x, mul_y);
			mul_done = 1;
		}
		if(key == KEY_F4)
		{
			loop_result = dsp_loop(loop_x);
			loop_done = 1;
		}
	}
}

#endif
