#include <gint/std/stdio.h>
#include <gint/dma.h>
#include <gint/mpu/dma.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

#ifdef FXCG50

#define DMA SH7305_DMA
#define dprint(x, y, ...) dprint(x, y, C_BLACK, C_NONE, __VA_ARGS__)

void show_dma(int x, int y, int channel)
{
	sh7305_dma_channel_t *addr[6] = {
		&DMA.DMA0, &DMA.DMA1, &DMA.DMA2,
		&DMA.DMA3, &DMA.DMA4, &DMA.DMA5,
	};
	sh7305_dma_channel_t *dma = addr[channel];

	#ifdef FX9860G
	dma->SAR = 0x12345678;
	dma->DAR = 0x9abcdef0;
	dma->TCR = 0x12481248;

	int dx=60, dy=8;
	dprint(x,    y,      "DMA%d:", channel);
	dprint(x,    y+1*dy, "%08X", (uint32_t)dma);
	dprint(x+dx, y+1*dy, "%08X", dma->DAR);
	dprint(x,    y+2*dy, "%08X", dma->TCR);
	dprint(x+dx, y+2*dy, "%08X", dma->CHCR);
	#endif

	#ifdef FXCG50
	int dx=45, dy=14;
	dprint(x,    y,      "DMA%d:", channel);
	dprint(x,    y+1*dy, "SAR");
	dprint(x+dx, y+1*dy, "%08X", dma->SAR);
	dprint(x,    y+2*dy, "DAR");
	dprint(x+dx, y+2*dy, "%08X", dma->DAR);
	dprint(x,    y+3*dy, "TCR");
	dprint(x+dx, y+3*dy, "%08X", dma->TCR);
	dprint(x,    y+4*dy, "CHCR");
	dprint(x+dx, y+4*dy, "%08X", dma->CHCR);
	#endif
}

/* gintctl_gint_dma(): Test the Direct Access Memory Controller */
void gintctl_gint_dma(void)
{
	/* Here we'll display the DMA status at "full speed", only limited by
	   the dupdate() time. */
	int key = 0, timeout = 1;

	/* Test channel, interrupts, and source */
	int channel = 0, interrupts = 0, source = 0;
	/* Number of successful attempts */
	int successes = 0;

	#ifdef FX9860G
	/* Currently visible channel */
	int view = 0;
	#endif

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		show_dma(1, 1, view);

		dprint(1, 32, "Channel     %d", channel);
		dprint(1, 40, "Interrupts  %s", interrupts ? "Yes" : "No");
		dprint(1, 48, "Source      %s", source ? "IL" : "RAM");
		dprint(103, 40, "%d", successes);

		extern image_t img_opt_gint_dma;
		dimage(0, 56, &img_opt_gint_dma);
		#endif

		#ifdef FXCG50
		row_title("Direct Memory Access status");

		show_dma(6,   24, 0);
		show_dma(138, 24, 1);
		show_dma(270, 24, 2);

		dprint(6, 102, "DMAOR: %08X", DMA.OR);

		dprint(6,  130, "Channel");
		dprint(96, 130, "%d", channel);
		dprint(6,  144, "Interrupt");
		dprint(96, 144, "%s", interrupts ? "Yes" : "No");
		dprint(6,  158, "Source");
		dprint(96, 158, "%s", source ? "IL" : "RAM");

		fkey_action(1, "RUN");

		fkey_button(4, "CHANNEL");
		fkey_button(5, "INT");
		fkey_button(6, "SOURCE");
		#endif

		dupdate();
		key = getkey_opt(GETKEY_DEFAULT, &timeout).key;

		/* On F1, start a DMA transfer and see what happens */
		if(key == KEY_F1)
		{
			void *src = (void *)(source ? 0xe5200000 : 0x88000000);
			void *dst = gint_vram;
			int blocks = 256;

			if(interrupts)
			{
				dma_transfer(channel, DMA_4B, blocks,
					src, DMA_INC, dst, DMA_INC);
				dma_transfer_wait(channel);
			}
			else dma_transfer_noint(channel, DMA_4B, blocks,
				src, DMA_INC, dst, DMA_INC);

			successes++;
		}

		#ifdef FX9860G
		/* On F2, switch the visible channel */
		if(key == KEY_F2) view = (view + 1) % 6;
		#endif

		/* On F4, F5 and F6, change parameters */
		if(key == KEY_F4) channel = (channel + 1) % 6;
		if(key == KEY_F5) interrupts = !interrupts;
		if(key == KEY_F6) source = !source;
	}
}

#endif /* FXCG50 */
