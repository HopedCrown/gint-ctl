#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/drivers/r61523.h>
#include <gintctl/gint.h>
#include <gintctl/util.h>

#if GINT_HW_CP

void gintctl_gint_r61523(void)
{
	int key = 0;
    int select = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);
		
        row_title("R61523 display driver control");
        
        int level, PWM_div;
        bool EN2, dimming;
        static const char *EN2_string[] = {"LOW", "HIGH"};
        static const char *dimming_string[] = {"OFF", "ON"};
        r61523_get_backlight(&EN2, &level, &PWM_div, &dimming);
        row_print(1, 1, "EN2:");
        row_print(2, 1, "Level:");
        row_print(3, 1, "PWM Div:");
        row_print(4, 1, "Dimming:");
        row_print(1, 20, EN2_string[EN2]);
        row_print(2, 20, "%d", level);
        row_print(3, 20, "0x%02x", PWM_div);
        row_print(4, 20, dimming_string[dimming]);

        int CPL, BP, FP;
        bool waveform;
        bool idle = (r61523_get_power_mode() >> 6) & 1;
        static const char *mode_string[] = {"Normal", "Idle"};
        static const char *waveform_string[] = {"Frame", "Line"};
        r61523_get_display_timing(idle, &waveform, &CPL, &BP, &FP);
        row_print(5, 1, "Enter %s Mode [EXE]", mode_string[!idle]);
        row_print(6, 1, "BC (%s):", mode_string[idle]);
        row_print(7, 1, "CPL (%s):", mode_string[idle]);
        row_print(8, 1, "BP (%s):", mode_string[idle]);
        row_print(9, 1, "FP (%s):", mode_string[idle]);
        row_print(6, 20, waveform_string[waveform]);
        row_print(7, 20, "%d", CPL);
        row_print(8, 20, "%d", BP);
        row_print(9, 20, "%d", FP);

        row_highlight(select + 1);

		dupdate();

		key = getkey().key;
		
        switch(key)
        {
            case KEY_UP:
                if(select)
                    select--;
                break;
            case KEY_DOWN:
                if(select < 8)
                    select++;
                break;
            case KEY_LEFT:
                if(select == 0)
                    EN2 = !EN2;
                else if(select == 1 && level)
                    level--;
                else if(select == 2 && PWM_div)
                    PWM_div--;
                else if(select == 3)
                    dimming = !dimming;
                else if(select == 5)
                    waveform = !waveform;
                else if(select == 6 && CPL > 18)
                    CPL--;
                else if(select == 7 && BP > 4)
                    BP--;
                else if(select == 8 && FP > 4)
                    FP--;
                break;
            case KEY_RIGHT:
                if(select == 0)
                    EN2 = !EN2;
                else if(select == 1 && level < 255)
                    level++;
                else if(select == 2 && PWM_div < 255)
                    PWM_div++;
                else if(select == 3)
                    dimming = !dimming;
                else if(select == 5)
                    waveform = !waveform;
                else if(select == 6 && CPL < 63)
                    CPL++;
                else if(select == 7 && BP < 128)
                    BP++;
                else if(select == 8 && FP < 128)
                    FP++;
                break;
        }

        r61523_set_backlight(EN2, level, PWM_div, dimming);
        r61523_set_display_timing(idle, waveform, CPL, BP, FP);

        if(select == 4 && key == KEY_EXE)
        {
            if(idle)
                r61523_exit_idle_mode();
            else
                r61523_enter_idle_mode();
        }
	}
}

#endif /* GINT_HW_CP */
