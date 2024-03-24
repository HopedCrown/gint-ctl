#include <gint/dma.h>
#include <gint/mpu/dma.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/mmu.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

#include <stdio.h>

#define DMA SH7305_DMA
#define dprint(x, y, ...) dprint(x, y, C_BLACK, __VA_ARGS__)

void show_dma(int x, int y, GUNUSED int channel, sh7305_dma_channel_t *dma)
{
	#if GINT_RENDER_MONO
	int dx=60, dy=8;
	dprint(x, y,      "SAR:");
	dprint(x, y+1*dy, "DAR:");
	dprint(x, y+2*dy, "TCR:");
	dprint(x, y+3*dy, "CHCR:");
	dprint(x+dx, y,      "%08X", dma->SAR);
	dprint(x+dx, y+1*dy, "%08X", dma->DAR);
	dprint(x+dx, y+2*dy, "%08X", dma->TCR);
	dprint(x+dx, y+3*dy, "%08X", dma->CHCR);
	#endif

	#if GINT_RENDER_RGB
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
	/* We'll display the DMA status at "full speed", without sleeping. */
	int key=0, timeout=1;
	/* Test channel, interrupts, and source; successful attempts */
	int channel=0, interrupts=0, source=0, successes=0;

	/* Get the physical VRAM address */
	void *vram_address = gint_vram;
	#ifdef FX9860G
	uint32_t virt_page = (uint32_t)vram_address & 0xfffff000;
	uint32_t phys_page = 0x80000000 + mmu_translate(virt_page, NULL);
	vram_address = (void *)phys_page + (vram_address - (void *)virt_page);
	#endif

	sh7305_dma_channel_t *addr[6] = {
		&DMA.DMA0, &DMA.DMA1, &DMA.DMA2, &DMA.DMA3, &DMA.DMA4, &DMA.DMA5,
	};

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#if GINT_RENDER_MONO
		show_dma(1, 0, channel, addr[channel]);
		dprint(1, 32, "Channel     DMA%d", channel);
		dprint(1, 40, "Interrupts  %s", interrupts ? "Yes" : "No");
		dprint(1, 48, "Source      %s", source ? "IL" : "RAM");
		dprint(103, 40, "%d", successes);

		extern bopti_image_t img_opt_gint_dma;
		dimage(0, 56, &img_opt_gint_dma);
		#endif

		#if GINT_RENDER_RGB
		row_title("Direct Memory Access status");

		show_dma(6,   24, 0, addr[0]);
		show_dma(138, 24, 1, addr[1]);
		show_dma(270, 24, 2, addr[2]);

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

		/* On F1, start a 1024-byte DMA transfer and see what happens */
		if(key == KEY_F1)
		{
			void *src = (void *)(source ? 0xe5200000 : 0x88000000);
			void *dst = vram_address;
			int blocks = 256;

			if(interrupts) dma_transfer_sync(channel, DMA_4B,
				blocks, src, DMA_INC, dst, DMA_INC);
			else dma_transfer_atomic(channel, DMA_4B, blocks, src,
				DMA_INC, dst, DMA_INC);

			successes++;
		}

		if(key == KEY_F4) channel = (channel + 1) % 6;
		if(key == KEY_F5) interrupts = !interrupts;
		if(key == KEY_F6) source = !source;
	}
}
