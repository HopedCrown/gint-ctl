#include <gint/mpu/intc.h>
#include <gint/mpu/rtc.h>
#include <gint/hardware.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/util.h>

void gintctl_regs(void)
{
	dclear(C_WHITE);
	row_title("Register browser");

	if(isSH3())
	{
		#define IPR(X) (*SH7705_INTC._.IPR##X).word
		row_print(2,1, "A:%04x B:%04x C:%04x", IPR(A), IPR(B), IPR(C));
		row_print(3,1, "D:%04x E:%04x F:%04x", IPR(D), IPR(E), IPR(F));
		row_print(4,1, "G:%04x H:%04x",        IPR(G), IPR(H));

		row_print(6, 1, "RCR1:%02x RCR2:%02x",
			SH7705_RTC.RCR1.byte,
			SH7705_RTC.RCR2.byte
		);
		#undef IPR
	}
	else
	{
		#define IPR(X) SH7305_INTC._->IPR##X.word
		row_print(2,1, "A:%04x B:%04x C:%04x", IPR(A), IPR(B), IPR(C));
		row_print(3,1, "D:%04x E:%04x F:%04x", IPR(D), IPR(E), IPR(F));
		row_print(4,1, "G:%04x H:%04x I:%04x", IPR(G), IPR(H), IPR(I));
		row_print(5,1, "J:%04x K:%04x L:%04x", IPR(J), IPR(K), IPR(L));
		#undef IPR
	}

	dupdate();
	getkey();

	if(isSH3()) return;

	#define IMR(X) SH7305_INTC.MSK->IMR##X

	dclear(C_WHITE);
	row_title("Register browser");

	row_print(2, 1, "0:%02x 1:%02x 2:%02x 3:%02x",
		IMR(0), IMR(1), IMR(2), IMR(3));
	row_print(3, 1, "4:%02x 5:%02x 6:%02x 7:%02x",
		IMR(4), IMR(5), IMR(6), IMR(7));
	row_print(4, 1, "8:%02x 9:%02x A:%02x B:%02x",
		IMR(8), IMR(9), IMR(10), IMR(11));
	row_print(5, 1, "C:%02x", IMR(12));

	dupdate();
	getkey();

	dclear(C_WHITE);
	row_title("Register browser");

	row_print(2, 1, "RAMCR: %08x", *(uint32_t *)0xff000074);
	row_print(3, 1, "SAR0:  %08x", *(uint32_t *)0xfe008020);
	row_print(4, 1, "CHCR0: %08x", *(uint32_t *)0xfe00802c);

	dupdate();
	getkey();
}
