#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>
#include <gint/mpu/usb.h>

#include <gintctl/gint.h>
#include <gintctl/widgets/gscreen.h>
#include <justui/jpainted.h>
#include <justui/jwidget.h>
#include <justui/jscrolledlist.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef FXCG50

#define USB SH7305_USB

//=== Command and trace models ===

enum {
    COMMAND_OPEN = 0,
    COMMAND_OPEN_WAIT,
    COMMAND_CLOSE,
    /* Async writes */
    COMMAND_WRITE_ASYNC_TEXT_HEADER,
    COMMAND_WRITE_ASYNC_IMAGE_HEADER,
    COMMAND_WRITE_ASYNC_SHORT_TEXT,
    COMMAND_WRITE_ASYNC_VRAM,
    /* Sync writes */
    COMMAND_WRITE_SYNC_TEXT_HEADER,
    COMMAND_WRITE_SYNC_IMAGE_HEADER,
    COMMAND_WRITE_SYNC_SHORT_TEXT,
    COMMAND_WRITE_SYNC_VRAM,
    /* Control */
    COMMAND_COMMIT_ASYNC,
    COMMAND_COMMIT_SYNC,
    COMMAND_WORLD_SWITCH,

    COMMAND__TOTAL,
};

struct trace {
    char const *message;
    uint16_t SYSCFG, SYSSTS, DVSTCTR;
    uint16_t CFIFOSEL, CFIFOCTR;
    uint16_t D0FIFOSEL, D0FIFOCTR;
    uint16_t D1FIFOSEL, D1FIFOCTR;
    uint16_t INTENB0, BRDYENB, NRDYENB, BEMPENB;
    uint16_t INTSTS0, BRDYSTS, NRDYSTS, BEMPSTS;
    uint16_t PIPESEL, PIPECFG;
    uint16_t PIPECTR[9];
};

#define MAX_COMMANDS 64
#define MAX_TRACES 256 /* ~14 kB */

static int *commands = NULL;
static int commands_len = 0;

static struct trace *traces = NULL;
static int traces_len = 0;

//=== Model interface ===

static void commands_clear(void)
{
    commands_len = 0;
}

static void commands_add(int c)
{
    if(commands_len >= MAX_COMMANDS)
        return;
    commands[commands_len++] = c;
}

static int commands_move_by(int index, int diff)
{
    if(index < 0 || index >= commands_len)
        return index;
    if(index + diff < 0 || index + diff >= commands_len || diff == 0)
        return index;

    int c = commands[index + diff];
    commands[index + diff] = commands[index];
    commands[index] = c;
    return index + diff;
}

static void traces_clear(void)
{
    traces_len = 0;
}

static struct trace *traces_add(void)
{
    if(traces_len >= MAX_TRACES)
        return NULL;
    return &traces[traces_len++];
}

//=== Painters ===

static char const * const command_texts[] = {
    "usb_open({&usb_ff_bulk, NULL})",
    "usb_open_wait()",
    "usb_close()",

    "usb_write_async() [text header]",
    "usb_write_async() [image header]",
    "usb_write_async() [short text]",
    "usb_write_async() [VRAM]",

    "usb_write_sync() [text header]",
    "usb_write_sync() [image header]",
    "usb_write_sync() [short text]",
    "usb_write_sync() [VRAM]",

    "usb_commit_async()",
    "usb_commit_sync()",
    "World switch",
};

static void paint_command(int x, int y, int w, int h, GUNUSED jlist *l,
    int index, bool selected)
{
    if(index < commands_len) {
        dprint(x+6, y+3, C_RGB(16, 16, 16), "[%02d]", index);
        dtext(x+38, y+3, C_BLACK, command_texts[commands[index]]);

        if(selected) {
            /* Up/Down arrows */
            int ah = 4;
            int spacing = 3;
            for(int i = 0; i < 2 * ah; i++) {
                int aw = (i < ah) ? i : 2*ah-i-1;
                int dy = (i < ah) ? i+spacing : h-ah-spacing+(i-ah);
                int dx = w - 10;
                dline(x+dx-aw, y+dy, x+dx+aw, y+dy, C_BLACK);
            }
        }
    }
    else {
        dtext(x+8, y+3, C_BLACK, "<Add new command...>");
    }

    if(selected)
        drect(x, y, x+w-1, y+h-1, C_INVERT);
}

static void info_command(GUNUSED jlist *l, GUNUSED int index,
    jlist_item_info *info)
{
    info->delegate = NULL;
    info->selectable = true;
    info->triggerable = true;
    info->natural_width = 116;
    info->natural_height = 15;
}

static void paint_commandoption(int x, int y, int w, int h, GUNUSED jlist *l,
    int index, bool selected)
{
    dtext(x+8, y+3, C_BLACK, command_texts[index]);

    if(selected)
        drect(x, y, x+w-1, y+h-1, C_INVERT);
}

static void info_commandoption(GUNUSED jlist *l, GUNUSED int index,
    jlist_item_info *info)
{
    info->delegate = NULL;
    info->selectable = true;
    info->triggerable = true;
    info->natural_width = 116;
    info->natural_height = 15;
}

static void paint_trace(int x, int y, int w, int h, GUNUSED jlist *l,
    int index, bool selected)
{
    dprint(x+8, y+3, C_BLACK, "<> %s", traces[index].message);

    if(selected)
        drect(x, y, x+w-1, y+h-1, C_INVERT);
}

static void info_trace(GUNUSED jlist *l, GUNUSED int index,
    jlist_item_info *info)
{
    info->delegate = NULL;
    info->selectable = true;
    info->triggerable = true;
    info->natural_width = 116;
    info->natural_height = 15;
}

//=== Command execution infrastructure ===//

// TODO: Collect logs

static void usbtrace_trace(char const *message)
{
    struct trace *t = traces_add();
    if(!t)
        return;

    t->message = message;
    t->SYSCFG = USB.SYSCFG.word;
    t->SYSSTS = USB.SYSSTS.word;
    t->DVSTCTR = USB.DVSTCTR.word;
    t->CFIFOSEL = USB.CFIFOSEL.word;
    t->CFIFOCTR = USB.CFIFOCTR.word;
    t->D0FIFOSEL = USB.D0FIFOSEL.word;
    t->D0FIFOCTR = USB.D0FIFOCTR.word;
    t->D1FIFOSEL = USB.D1FIFOSEL.word;
    t->D1FIFOCTR = USB.D1FIFOCTR.word;
    t->INTENB0 = USB.INTENB0.word;
    t->BRDYENB = USB.BRDYENB;
    t->NRDYENB = USB.NRDYENB;
    t->BEMPENB = USB.BEMPENB;
    t->INTSTS0 = USB.INTSTS0.word;
    t->BRDYSTS = USB.BRDYSTS;
    t->NRDYSTS = USB.NRDYSTS;
    t->BEMPSTS = USB.BEMPSTS;
    t->PIPESEL = USB.PIPESEL.word;
    t->PIPECFG = USB.PIPECFG.word;

    for(int i = 0; i < 9; i++)
        t->PIPECTR[i] = USB.PIPECTR[i].word;
}

static void execute_tracer(void)
{
    usb_set_trace(usbtrace_trace);
    usb_interface_t const *intf[] = { &usb_ff_bulk, NULL };

    char const *text_fmt = "Hello from USB tracer! Message %03d.\n";
    char text[64];
    int text_count = 0;
    int text_size = 36;

    void *image = gint_vram;
    int image_size = DWIDTH * DHEIGHT * 2;

    for(int i = 0; i < commands_len; i++) {
        USB_TRACE(command_texts[commands[i]]);


        switch(commands[i]) {
        case COMMAND_OPEN:
            usb_open(intf, GINT_CALL_NULL);
            break;
        case COMMAND_OPEN_WAIT:
            usb_open_wait();
            break;
        case COMMAND_CLOSE:
            usb_close();
            break;

        case COMMAND_WRITE_ASYNC_TEXT_HEADER:{
            usb_fxlink_header_t h;
            usb_fxlink_fill_header(&h, "fxlink", "text", text_size);
            usb_write_async(usb_ff_bulk_output(), &h, sizeof h, 1, false,
                GINT_CALL_NULL);
            break;}

        case COMMAND_WRITE_ASYNC_IMAGE_HEADER:{
            usb_fxlink_header_t h;
            usb_fxlink_image_t sh;
            usb_fxlink_fill_header(&h, "fxlink", "image",
                sizeof sh + image_size);
            sh.width = htole32(DWIDTH);
            sh.height = htole32(DHEIGHT);
            sh.pixel_format = htole32(USB_FXLINK_IMAGE_RGB565);
            usb_write_async(usb_ff_bulk_output(), &h, sizeof h, 4, false,
                GINT_CALL_NULL);
            usb_write_async(usb_ff_bulk_output(), &sh, sizeof sh, 4, false,
                GINT_CALL_NULL);
            break;}

        case COMMAND_WRITE_ASYNC_SHORT_TEXT:
            sprintf(text, text_fmt, ++text_count);
            usb_write_async(usb_ff_bulk_output(), text, text_size, 1, false,
                GINT_CALL_NULL);
            break;

        case COMMAND_WRITE_ASYNC_VRAM:
            usb_write_async(usb_ff_bulk_output(), image, image_size, 4, false,
                GINT_CALL_NULL);
            break;

        case COMMAND_WRITE_SYNC_TEXT_HEADER:{
            usb_fxlink_header_t h;
            usb_fxlink_fill_header(&h, "fxlink", "text", text_size);
            usb_write_sync(usb_ff_bulk_output(), &h, sizeof h, 1, false);
            break;}

        case COMMAND_WRITE_SYNC_IMAGE_HEADER:{
            usb_fxlink_header_t h;
            usb_fxlink_image_t sh;
            usb_fxlink_fill_header(&h, "fxlink", "image",
                sizeof sh + image_size);
            sh.width = htole32(DWIDTH);
            sh.height = htole32(DHEIGHT);
            sh.pixel_format = htole32(USB_FXLINK_IMAGE_RGB565);
            usb_write_sync(usb_ff_bulk_output(), &h, sizeof h, 4, false);
            usb_write_sync(usb_ff_bulk_output(), &sh, sizeof sh, 4, false);
            break;}

        case COMMAND_WRITE_SYNC_SHORT_TEXT:
            sprintf(text, text_fmt, ++text_count);
            usb_write_sync(usb_ff_bulk_output(), text, text_size, 1, false);
            break;

        case COMMAND_WRITE_SYNC_VRAM:
            usb_write_sync(usb_ff_bulk_output(), image, image_size, 4, false);
            break;

        case COMMAND_COMMIT_ASYNC:
            usb_commit_async(usb_ff_bulk_output(), GINT_CALL_NULL);
            break;
        case COMMAND_COMMIT_SYNC:
            usb_commit_sync(usb_ff_bulk_output());
            break;
        case COMMAND_WORLD_SWITCH:
            gint_world_switch(GINT_CALL_NULL);
            break;
        }
    }

    usb_set_trace(NULL);
}

static void val(int order, char const *name, uint32_t value, uint32_t old)
{
    int y = 12 * (order / 3) + 20;
    int x = 6 + 128 * (order % 3);

    int fg = (value != old) ? C_RGB(31, 8, 8) : C_WHITE;

    dprint(x, y, C_WHITE, "%s", name);
    dprint(x+_(40,88), y, fg, "%04X", value);
}

#define val(order, name, t1, t2, value) \
    val(order, name, t1->value, t2 ? t2->value : t1->value)

static void draw_registers(struct trace const *t, struct trace const *prev)
{
    val( 0, "SYSCFG",    t, prev, SYSCFG);
    val( 1, "SYSSTS",    t, prev, SYSSTS);
    val( 2, "DVSTCTR",   t, prev, DVSTCTR);

    val( 3, "CFIFOSEL",  t, prev, CFIFOSEL);
    val( 4, "D0FIFOSEL", t, prev, D0FIFOSEL);
    val( 5, "D1FIFOSEL", t, prev, D1FIFOSEL);
    val( 6, "CFIFOCTR",  t, prev, CFIFOCTR);
    val( 7, "D0FIFOCTR", t, prev, D0FIFOCTR);
    val( 8, "D1FIFOCTR", t, prev, D1FIFOCTR);

    val( 9, "INTENB0",   t, prev, INTENB0);
    val(10, "INTSTS0",   t, prev, INTSTS0);

    val(12, "BRDYENB",   t, prev, BRDYENB);
    val(13, "NRDYENB",   t, prev, NRDYENB);
    val(14, "BEMPENB",   t, prev, BEMPENB);
    val(15, "BRDYSTS",   t, prev, BRDYSTS);
    val(16, "NRDYSTS",   t, prev, NRDYSTS);
    val(17, "BEMPSTS",   t, prev, BEMPSTS);

    val(18, "PIPESEL",   t, prev, PIPESEL);
    val(19, "PIPECFG",   t, prev, PIPECFG);

    val(21, "PIPE1CTR",  t, prev, PIPECTR[0]);
    val(22, "PIPE2CTR",  t, prev, PIPECTR[1]);
    val(23, "PIPE3CTR",  t, prev, PIPECTR[2]);
    val(24, "PIPE4CTR",  t, prev, PIPECTR[3]);
    val(25, "PIPE5CTR",  t, prev, PIPECTR[4]);
    val(26, "PIPE6CTR",  t, prev, PIPECTR[5]);
    val(27, "PIPE7CTR",  t, prev, PIPECTR[6]);
    val(28, "PIPE8CTR",  t, prev, PIPECTR[7]);
    val(29, "PIPE9CTR",  t, prev, PIPECTR[8]);
}

static int browse_traces(int cursor)
{
    if(cursor < 0 || cursor >= traces_len)
        return cursor;

    while(1) {
        struct trace *t = &traces[cursor];

        dclear(0x5555);
        if(cursor > 0)
            dtext_opt(4, 4, C_WHITE, C_NONE, DTEXT_LEFT, DTEXT_TOP, "<");
        if(cursor < traces_len+1)
            dtext_opt(DWIDTH-5, 4, C_WHITE, C_NONE, DTEXT_RIGHT, DTEXT_TOP,
                ">");
        dprint(20, 4, C_WHITE, "[%3d/%3d] %s", cursor+1, traces_len,
            t->message);
        draw_registers(t, cursor > 0 ? &traces[cursor - 1] : NULL);
        dupdate();

        key_event_t ev = getkey();
        int k = ev.key;
        if(k == KEY_LEFT && !ev.shift && cursor > 0)
            cursor--;
        if(k == KEY_RIGHT && !ev.shift && cursor < traces_len - 1)
            cursor++;
        if(k == KEY_LEFT && ev.shift)
            cursor = 0;
        if(k == KEY_RIGHT && ev.shift)
            cursor = traces_len - 1;
        if(k == KEY_EXIT)
            return cursor;
    }
}

//=== Main function ===

void gintctl_gint_usbtrace(void)
{
    if(commands == NULL)
        commands = malloc(MAX_COMMANDS * sizeof *commands);
    if(traces == NULL)
        traces = malloc(MAX_TRACES * sizeof *traces);

    if(commands == NULL || traces == NULL) {
        dclear(C_WHITE);
        dprint(1, 1, C_BLACK, "Alloc failure!");
        dupdate();
        getkey();
        return;
    }

    commands_clear();
    traces_clear();

    gscreen *scr = gscreen_create("Live USB state tracing",
        "/PROG;/TRACES;;;;#RUN|/PROG;/TRACES;;;#CLEAR;");

    // Command composition tab

    jscrolledlist *tab1 = jscrolledlist_create(NULL, info_command,
        paint_command);
    jlist *commands_list = tab1->list;
    jlist_update_model(commands_list, 1);

    // Trace browsing tab

    jscrolledlist *tab2 = jscrolledlist_create(NULL, info_trace,
        paint_trace);
    jlist *traces_list = tab2->list;
    jlist_update_model(traces_list, 0);

    // New command selection tab

    jscrolledlist *tab3 = jscrolledlist_create(NULL, info_commandoption,
        paint_commandoption);
    jlist *commandoptions_list = tab3->list;
    jlist_update_model(commandoptions_list, COMMAND__TOTAL);

    // Scene setup

    gscreen_add_tab(scr, tab1, commands_list);
    gscreen_set_tab_fkeys_level(scr, 0, 0);
    gscreen_add_tab(scr, tab2, traces_list);
    gscreen_set_tab_fkeys_level(scr, 1, 1);
    gscreen_add_tab(scr, tab3, commandoptions_list);
    gscreen_set_tab_fkeys_visible(scr, 2, false);

    gscreen_show_tab(scr, 0);
    jscene_set_focused_widget(scr->scene, commands_list);

    commands_add(COMMAND_OPEN);
    commands_add(COMMAND_OPEN_WAIT);
    commands_add(COMMAND_WRITE_SYNC_TEXT_HEADER);
    commands_add(COMMAND_WRITE_SYNC_SHORT_TEXT);
    commands_add(COMMAND_COMMIT_SYNC);
    commands_add(COMMAND_WORLD_SWITCH);
    commands_add(COMMAND_OPEN_WAIT);
    commands_add(COMMAND_WRITE_SYNC_TEXT_HEADER);
    commands_add(COMMAND_WRITE_SYNC_SHORT_TEXT);
    commands_add(COMMAND_COMMIT_SYNC);
    commands_add(COMMAND_WRITE_SYNC_TEXT_HEADER);
    commands_add(COMMAND_WRITE_SYNC_SHORT_TEXT);
    commands_add(COMMAND_COMMIT_SYNC);
    jlist_update_model(commands_list, commands_len+1);

    while(1) {
        jevent e = jscene_run(scr->scene);
        void *focus = jscene_focused_widget(scr->scene);
        int key = 0;
        if(e.type == JSCENE_KEY && e.key.type == KEYEV_DOWN)
            key = e.key.key;

        if(e.type == JSCENE_PAINT) {
            dclear(C_WHITE);
            jscene_render(scr->scene);
            dupdate();
        }

        if(e.type == JLIST_ITEM_TRIGGERED && e.source == commands_list) {
            if(e.data >= commands_len)
                gscreen_show_tab(scr, 2);
        }
        if(e.type == JLIST_ITEM_TRIGGERED && e.source == commandoptions_list) {
            commands_add(e.data);
            jlist_update_model(commands_list, commands_len + 1);
            gscreen_show_tab(scr, 0);
        }
        if(e.type == JLIST_ITEM_TRIGGERED && e.source == traces_list) {
            int cursor = jlist_selected_item(traces_list);
            cursor = browse_traces(cursor);
            jlist_select(traces_list, cursor);
        }

        if(key == KEY_F5 && gscreen_in(scr, 1)) {
            traces_clear();
            jlist_update_model(traces_list, traces_len);
        }

        if(key == KEY_F6 && gscreen_in(scr, 0)) {
            execute_tracer();
            jlist_update_model(traces_list, traces_len);
            gscreen_show_tab(scr, 1);
        }

        //---

        if(key == KEY_F1 && !gscreen_in(scr, 2)) {
            gscreen_show_tab(scr, 0);
        }
        if(key == KEY_F2 && !gscreen_in(scr, 2)) {
            gscreen_show_tab(scr, 1);
        }

        if(key == KEY_UP && e.key.alpha && focus == commands_list) {
            int s = commands_move_by(jlist_selected_item(commands_list), -1);
            // TODO: Factor out these list model updates
            jlist_update_model(commands_list, commands_len + 1);
            jlist_select(commands_list, s);
        }
        if(key == KEY_DOWN && e.key.alpha && focus == commands_list) {
            int s = commands_move_by(jlist_selected_item(commands_list), +1);
            jlist_update_model(commands_list, commands_len + 1);
            jlist_select(commands_list, s);
        }

        if(key == KEY_EXIT && (gscreen_in(scr, 0) || gscreen_in(scr, 1)))
            break;
        if((key == KEY_EXIT || key == KEY_F6) && gscreen_in(scr, 2))
            gscreen_show_tab(scr, 0);
    }

    gscreen_destroy(scr);
}

#endif
