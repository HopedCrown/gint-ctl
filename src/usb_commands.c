#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <endian.h>
#include <ctype.h>

#include <gintctl/config.h>

#if GINTCTL_ENABLE_USB

static void drop(usb_fxlink_header_t const *h)
{
    USB_LOG("[gintctl] dropping %.16s.%.16s\n", h->application, h->type);
    usb_fxlink_drop_transaction();
    USB_LOG("[gintctl] done dropping\n");
}

void gintctl_handle_usb_command(usb_fxlink_header_t const *h)
{
    if(strcmp(h->application, "gintctl") != 0)
        return drop(h);

    if(!strcmp(h->type, "echo-bounds")) {
        int buffer[128], first = 0, last = 0, total = 0;
        int block = USB_READ_BLOCK;

        while(1) {
            timeout_t tm = timeout_make_ms(1000);
            int rc = usb_read_async(usb_ff_bulk_input(), buffer, sizeof buffer,
                USB_READ_WAIT | block, NULL, &tm, GINT_CALL_NULL);
            if(rc < 0)
                break;
            block = 0;

            if(total == 0)
                first = le32toh(buffer[0]);
            if(rc >= 4)
                last = le32toh(buffer[rc / 4 - 1]);
            total += rc;

            if(rc != sizeof buffer)
                break;
        }

        char str[64];
        sprintf(str, "first=%08x last=%08x total=%d B\n", first, last, total);
        usb_fxlink_text(str, 0);
        return;
    }

    if(!strcmp(h->type, "garbage")) {
        /* Purposefully don't read the transaction's contents so they get
           interpreted as garbage headers; this is to stress test the function
           that reads & interprets headers. */
        return;
    }

    if(!strcmp(h->type, "read-unaligned")) {
        /* Read by little bits depending on given mode */
        char mode;
        usb_read_sync(usb_ff_bulk_input(), &mode, 1, false);

        char *buf = malloc(h->size);
        if(!buf) {
            usb_fxlink_drop_transaction();
            return;
        }

        USB_LOG("[gintctl] Read unaligned size %d mode '%c'\n",
            h->size-1, mode);
        if(mode == 'r')
            srand(0xc0ffee);

        size_t i = 0;
        int round_number = 0;

        while(i < h->size - 1) {
            size_t round = 1;

            /* Determine the size of this next round */
            if(isdigit(mode)) {
                round = mode - '0';
            }
            else if(mode == 'i') {
                round = (round_number & 3) + 1;
            }
            else if(mode == 'r') {
                round = rand() & 15;
            }

            if(round > (h->size - 1) - i)
                round = h->size - 1 - i;
            usb_read_sync(usb_ff_bulk_input(), buf + i, round, false);
            i += round;
            round_number++;
        }

        /* Echo the result */
        buf[h->size - 1] = 0;
        USB_LOG("Final result: %s\n", buf);
        usb_fxlink_text(buf, h->size - 1);
        free(buf);
        return;
    }

    drop(h);
}

#endif /* GINTCTL_ENABLE_USB */
