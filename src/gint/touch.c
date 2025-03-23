#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/gint.h>
#include <gint/touch.h>
#include <gint/drivers.h>
#include <gint/drivers/r61523.h>
#include <gint/drivers/states.h>
#include <gint/defs/util.h>
#include <libprof.h>
#include <string.h>

#if GINT_HW_CP

static bool touch_get_os_calibration(
    int *x_base, int *y_base, int *x_div, int *y_div)
{
    if(!memcmp((void *)0x80020020, "02.01.2000", 10) ||
       !memcmp((void *)0x80020020, "02.01.7002", 10)) {
        *x_base = *(int *)0x8c1bea50;
        *y_base = *(int *)0x8c1bea54;
        *x_div  = *(int *)0x8c1bea58;
        *y_div  = *(int *)0x8c1bea5c;
        return true;
    }
    return false;
}

//=== Copy of gint internals =================================================//

/* _touch_adraw - raw 0x84 register information */
struct _touch_adraw
{
    uint16_t x1;
    uint16_t y1;
    uint16_t z1;
    uint16_t gh;
    uint16_t x2;
    uint16_t y2;
    uint16_t z2;
    uint16_t dm;
};

/* _touch_adconv - post-conversion raw 0x84 register information */
struct _touch_adconv
{
    int x1;
    int y1;
    int x2;
    int y2;
    int z1;
    int z2;
    uint16_t gh;
    uint16_t dm;
};

/* _touch_addots_type - type of dots */
enum _touch_addots_type
{
    TS_DOTS_TYPE_OFF    = 0,
    TS_DOTS_TYPE_SINGLE = 1,
    TS_DOTS_TYPE_DUAL   = 2,
};

/* _touch_addots - touchscreen dots information */
struct _touch_addots
{
    enum _touch_addots_type type;
    int x1;
    int y1;
    int z1;
    int x2;
    int y2;
    int z2;
};

/* touch_adconv_get_raw() - read 0x84 register using I2C */
extern int touch_adconv_get_raw(struct _touch_adraw *adraw);

/* touch_adconv_get_conv() - perform the raw conversion */
extern int touch_adconv_get_conv(
    struct _touch_adconv *adconv,
    struct _touch_adraw *adraw,
    int type
);

/* touch_adconv_get_dots() - generate dots information */
extern int touch_adconv_get_dots(
    struct _touch_addots *dots,
    struct _touch_adconv *adconv,
    int type
);

// debug symbols

/* touch_get_dots() - get dots information */
extern int touch_get_dots(struct _touch_addots *dots);

/* i2c_reg_read() - register read operation */
extern int i2c_reg_read(int reg, void *buffer, size_t size);

/* touch_get_dots() - get dots information */
int touch_get_dots(struct _touch_addots *addots)
{
    struct _touch_adraw adraw;
    struct _touch_adconv adconv;
    int type;

    type = touch_adconv_get_raw(&adraw);
    type = touch_adconv_get_conv(&adconv, &adraw, type);
    type = touch_adconv_get_dots(addots, &adconv, type);
    return type;
}

//=== Headers ================================================================//

/* struct menu - menu information */
struct menu
{
    bool dirty;
    void (*display)(struct menu *);
    void (*keyboard)(void);
    int *prof_frame;
    int *prof_peak;
    struct {
        int adconv_frame;
        int adconv_peak;
        int paint_frame;
        int paint_peak;
        int dots_frame;
        int dots_peak;
        int world_frame;
        int world_peak;
        int event_frame;
        int event_peak;
    } prof;
};

#define _pxy(...) dprint(x * 10, (y++) * 14, C_BLACK, __VA_ARGS__)

extern void world_menu_init(void);
extern void world_menu_display(struct menu *menu);
extern void world_menu_keyboard(void);

extern void paint_menu_init(void);
extern void paint_menu_display(struct menu *menu);
extern void paint_menu_keyboard(void);

extern void event_menu_init(void);
extern void event_menu_display(struct menu *menu);
extern void event_menu_keyboard(void);

extern void dots_menu_init(void);
extern void dots_menu_display(struct menu *menu);
extern void dots_menu_keyboard(void);

extern void adconv_menu_init(void);
extern void adconv_menu_display(struct menu *menu);
extern void adconv_menu_keyboard(void);

//=== Menu #1: A/D Conv ======================================================//

/* _adconv_info - internal adconv info */
struct _adinfo {
    struct _touch_adraw adraw;
    struct _touch_adconv adconv;
    struct _touch_addots dots;
    int _type;
};

/* _adconv_disp_i2c() - disp raw I2C info */
static int _adconv_disp_i2c(int y, struct _adinfo *info)
{
    GAUTOTYPE adraw = &info->adraw;
    uint8_t test;
    int x;

    info->_type = touch_adconv_get_raw(adraw);
    i2c_reg_read(0x68, &test, 1);
    x = 0;
    _pxy("Raw ADCONV values:");
    y += 1;
    x += 1;
    _pxy("RX1 %04X", adraw->x1);
    _pxy("RX2 %04X", adraw->x2);
    _pxy("RGH %04X", adraw->gh);
    y -= 3;
    x += 9;
    _pxy("RY1 %04X", adraw->y1);
    _pxy("RY2 %04X", adraw->y2);
    _pxy("RDM %04X", adraw->dm);
    y -= 3;
    x += 9;
    _pxy("RZ1 %04X", adraw->z1);
    _pxy("RZ2 %04X", adraw->z2);
    _pxy("R68 %04X", test);
    y += 1;
    x = 1;
    _pxy("RX1 %d", adraw->x1);
    _pxy("RX2 %d", adraw->x2);
    _pxy("RGH %d", adraw->gh);
    y -= 3;
    x += 9;
    _pxy("RY1 %d", adraw->y1);
    _pxy("RY2 %d", adraw->y2);
    _pxy("RDM %d", adraw->dm);
    y -= 3;
    x += 9;
    _pxy("RZ1 %d", adraw->z1);
    _pxy("RZ2 %d", adraw->z2);
    _pxy("R68 %d", test);
    return y + 1;
}

/* _adconv_disp_adconv() - disp ad convertion */
static int _adconv_disp_adconv(int y, struct _adinfo *info)
{
    GAUTOTYPE adconv = &info->adconv;
    GAUTOTYPE adraw = &info->adraw;
    int x;

    info->_type = touch_adconv_get_conv(adconv, adraw, info->_type);
    x = 0;
    _pxy("ADCONV convert values:");
    y += 1;
    x += 1;
    _pxy("X1 %04X", adconv->x1);
    _pxy("X2 %04X", adconv->x2);
    _pxy("GH %04X", adconv->gh);
    y -= 3;
    x += 9;
    _pxy("Y1 %04X", adconv->y1);
    _pxy("Y2 %04X", adconv->y2);
    _pxy("DM %04X", adconv->dm);
    y -= 3;
    x += 9;
    _pxy("Z1 %04X", adconv->z1);
    _pxy("Z2 %04X", adconv->z2);
    y += 2;
    x = 1;
    _pxy("X1 %d", adconv->x1);
    _pxy("X2 %d", adconv->x2);
    _pxy("GH %d", adconv->gh);
    y -= 3;
    x += 9;
    _pxy("Y1 %d", adconv->y1);
    _pxy("Y2 %d", adconv->y2);
    _pxy("DM %d", adconv->dm);
    y -= 3;
    x += 9;
    _pxy("Z1 %d", adconv->z1);
    _pxy("Z2 %d", adconv->z2);
    return y + 2;
}

/* _adconv_disp_dots() - disp dots information */
static int _adconv_disp_dots(int y, struct _adinfo *info)
{
    static char const * const status[3] = {"Off", "Single", "Dual"};
    GAUTOTYPE adconv = info->adconv;
    GAUTOTYPE dots = info->dots;
    int x;

    touch_adconv_get_dots(&dots, &adconv, info->_type);
    x = 0;
    _pxy("DOTS CONVERSION");
    y += 1;
    x += 1;
    _pxy("P1(%04X %04X %04X)", dots.x1, dots.y1, dots.z1);
    _pxy("P2(%04X %04X %04X)", dots.x2, dots.y2, dots.z2);
    y += 1;
    _pxy("P1(%d %d %d)", dots.x1, dots.y1, dots.z1);
    _pxy("P2(%d %d %d)", dots.x2, dots.y2, dots.z2);
    y += 1;
    _pxy("STATUS %s", status[dots.type]);
    if (dots.type != 0)
        dpixel(dots.x1, dots.y1, C_BLACK);
    if (dots.type == 2)
        dpixel(dots.x2, dots.y2, C_RED);
    return y + 1;
}

/* _adconv_disp_prof() - disp all profiling information */
static int _adconv_disp_prof(int y, struct menu *menu)
{
    int x;

    x = 0;
    _pxy("PROFILING:");
    y += 1;
    x += 1;
    GAUTOTYPE prof = menu->prof;
    _pxy("ADCONV (%dus, %dus)", prof.adconv_frame, prof.adconv_peak);
    _pxy("PAINT  (%dus, %dus)", prof.paint_frame, prof.paint_peak);
    _pxy("DOTS   (%dus, %dus)", prof.dots_frame, prof.dots_peak);
    _pxy("WORLD  (%dus, %dus)", prof.world_frame, prof.world_peak);
    _pxy("EVENT  (%dus, %dus)", prof.event_frame, prof.event_peak);
    return y + 1;
}

/* adconv_menu_init() - init menu */
void adconv_menu_init(void)
{
    ;
}

/* adconv_menu_display() - display menu */
void adconv_menu_display(struct menu *menu)
{
    struct _adinfo info;
    int y;

    memset(&info, 0x00, sizeof(struct _adinfo));
    dclear(C_WHITE);
    y = _adconv_disp_i2c(0, &info);
    y = _adconv_disp_adconv(y, &info);
    y = _adconv_disp_dots(y, &info);
    y = _adconv_disp_prof(y, menu);
    dupdate();
}

/* adconv_menu_keyboard() - keyboard handling */
void adconv_menu_keyboard(void)
{
    ;
}

//=== Menu #2: Paint =========================================================//

struct {
    int x1;
    int y1;
    bool have_prev;
} _paint_info;

/* paint_menu_init() - init menu */
void paint_menu_init(void)
{
    memset(&_paint_info, 0x00, sizeof(_paint_info));
    _paint_info.have_prev = false;
}

/* paint_menu_display() - display menu */
void paint_menu_display(struct menu *menu)
{
    struct _touch_addots dots;

    if (menu->dirty) {
        dclear(C_WHITE);
        dupdate();
        menu->dirty = false;
    }
    touch_get_dots(&dots);
    if (dots.type == TS_DOTS_TYPE_OFF) {
        _paint_info.have_prev = false;
        return;
    }
    if (_paint_info.have_prev) {
        dline(
            _paint_info.x1,
            _paint_info.y1,
            dots.x1,
            dots.y1,
            (dots.type != TS_DOTS_TYPE_DUAL) ? C_BLACK : C_RED
        );
    } else {
        dpixel(dots.x1, dots.y1, C_BLACK);
    }
    _paint_info.x1 = dots.x1;
    _paint_info.y1 = dots.y1;
    _paint_info.have_prev = true;
    dupdate();
}

/* paint_menu_keyboard() - keyboard handling */
void paint_menu_keyboard(void)
{
    ;
}

//=== Menu #3: Dots ==========================================================//

/* dots_menu_init() - init menu */
void dots_menu_init(void)
{
    ;
}

/* dots_menu_display() - display menu */
void dots_menu_display(struct menu *menu)
{
    struct _touch_addots dots;
    int color;

    if (menu->dirty) {
        dclear(C_WHITE);
        dupdate();
        menu->dirty = false;
    }
    touch_get_dots(&dots);
    if (dots.type == TS_DOTS_TYPE_OFF)
        return;
    color = (dots.type != TS_DOTS_TYPE_DUAL) ? C_BLACK : C_RED;
    dpixel(dots.x1+0, dots.y1+0, color);
    dpixel(dots.x1+0, dots.y1+1, color);
    dpixel(dots.x1+1, dots.y1+0, color);
    dpixel(dots.x1+1, dots.y1+1, color);
    r61523_display_rect(
        gint_vram, dots.x1, dots.x1 + 1, dots.y1, dots.y1 + 1
    );
}

/* dots_menu_keyboard() - keyboard handling */
void dots_menu_keyboard(void)
{
    ;
}

//=== Menu #4: World =========================================================//

/* _world_info - information */
static struct {
    touch_state_t _gint;
    touch_state_t _casio;
    int switch_count;
    uintptr_t ptr;
} _world_info;

/* _world_disp() - display world information */
static int _world_disp(char *name, touch_state_t *state, int y)
{
    int x;

    x = 0;
    _pxy("%s:", name);
    y += 1;
    x += 1;
    _pxy("PRCR %04X", state->PRCR);
    _pxy("PJCR %04X", state->PJCR);
    y -= 2;
    x += 9;
    _pxy("ICCR %02X", state->ICCR);
    _pxy("ICIC %02X", state->ICIC);
    y -= 2;
    x += 9;
    _pxy("ICCH %02X", state->ICCH);
    _pxy("ICIL %02X", state->ICCL);
    return y + 1;
}

/* _world_switch() - display world switch info */
static int _world_switch(int switch_count, uintptr_t ptr, int y)
{
    int x;

    x = 0;
    _pxy("world switch:");
    y += 1;
    x += 1;
    _pxy("counter: %d", switch_count);
    _pxy("ptr: %p", ptr);
    return y + 1;
}

static int _world_calibration(int y)
{
    int x = 0;
    _pxy("OS calibration parameters:");
    y += 1;
    x += 1;

    int x_base, y_base, x_div, y_div;
    if(!touch_get_os_calibration(&x_base, &y_base, &x_div, &y_div)) {
        _pxy("not available");
        return y+1;
    }

    _pxy("x_base: %04X (%d)", x_base, x_base);
    _pxy("y_base: %04X (%d)", y_base, y_base);
    _pxy("x_div: %04X (%d)", x_div, x_div);
    _pxy("y_div: %04X (%d)", y_div, y_div);
    return y + 1;
}


/* _world_drv_sync() - sync driver information */
static void _world_drv_sync(void)
{
    extern gint_world_t gint_world_os;
    extern gint_world_t gint_world_addin;

    for (int i = 0 ; i < gint_driver_count() ; i++)
    {
        if (strcmp(gint_drivers[i].name, "TOUCH") != 0)
            continue;
        memcpy(
            &_world_info._gint,
            gint_world_addin[i],
            sizeof(touch_state_t)
        );
        memcpy(
            &_world_info._casio,
            gint_world_os[i],
            sizeof(touch_state_t)
        );
        break;
    }
}

/* world_menu_init() - init menu */
void world_menu_init(void)
{
    memset(&_world_info, 0x00, sizeof(_world_info));
    _world_drv_sync();
}

/* world_menu_display() - display menu */
void world_menu_display(struct menu *menu)
{
    int y;

    (void)menu;
    dclear(C_WHITE);
    y = _world_disp("Gint", &_world_info._gint, 0);
    y = _world_disp("Casio", &_world_info._casio, y);
    y = _world_switch(_world_info.switch_count, _world_info.ptr, y);
    y = _world_calibration(y);
    dupdate();
}

/* world_menu_keyboard() - keyboard */
void world_menu_keyboard(void)
{
    extern void *__malloc(size_t);
    extern void __free(void *);

    if (keypressed(KEY_7)) {
        if (_world_info.ptr == 0x00000000) {
            _world_info.ptr = gint_world_switch(GINT_CALL(__malloc, 667));
            _world_info.switch_count += 1;
            _world_drv_sync();
        }
    } else if (keypressed(KEY_9)) {
        if (_world_info.ptr != 0x00000000) {
            gint_world_switch(GINT_CALL(__free, _world_info.ptr));
            _world_info.switch_count += 1;
            _world_info.ptr = 0x00000000;
            _world_drv_sync();
        }
    }
}

//=== Menu #5: Events ========================================================//

/* _event_info - internal event information */
static struct {
    key_event_t buffer[20];
    int cursor;
} _event_info;

/* _event_table_sync() - fetch next touch-screen event */
static void _event_table_sync(void)
{
    key_event_t evt;

    evt = touch_next_event();
    if (evt.type == KEYEV_NONE)
        return;
    _event_info.buffer[_event_info.cursor].type = evt.type;
    _event_info.buffer[_event_info.cursor].x = evt.x;
    _event_info.buffer[_event_info.cursor].y = evt.y;
    _event_info.cursor += 1;
    if (_event_info.cursor >= 20)
        _event_info.cursor = 0;
}

/* _event_table_iter() - iterate over the event buffer */
static int _event_table_iter(key_event_t **evt)
{
    int cursor;
    bool found;

    if (evt == NULL)
        return -3;
    found = false;
    cursor = _event_info.cursor;
    while (true)
    {
        cursor += 1;
        if (cursor >= 20)
            cursor = 0;
        if (cursor == _event_info.cursor)
            return -1;
        if (_event_info.buffer[cursor].type == KEYEV_NONE)
            continue;
        if (*evt == NULL || found) {
            *evt = &(_event_info.buffer[cursor]);
            break;
        }
        if (*evt == &(_event_info.buffer[cursor]))
            found = true;
    }
    return 0;
}

/* _event_disp_event() - display event information */
static int _event_disp_event(key_event_t *evt, int y)
{
    char const *type;
    int x;

    if (evt == NULL)
        return y;
    x = 1;
    type = "UNKNOWN";
    if (evt->type == KEYEV_TOUCH_RELEASE)
        type = "RELEASE";
    if (evt->type == KEYEV_TOUCH_PRESSED)
        type = "PRESSED";
    if (evt->type == KEYEV_TOUCH_DRAG)
        type = "DRAG";
    _pxy("%s x:%d y:%d", type, evt->x, evt->y);
    return y;
}

/* event_menu_init() - init menu */
void event_menu_init(void)
{
    memset(&_event_info, 0x00, sizeof(_event_info));
    for(int i = 0 ; i < 20 ; i++)
        _event_info.buffer[i].type = KEYEV_NONE;
    _event_info.cursor = 0;
}

/* event_menu_display() - display menu */
void event_menu_display(struct menu *menu)
{
    key_event_t *evt;
    int y;
    int x;

    (void)menu;
    _event_table_sync();

    y = 0;
    x = 0;
    evt = NULL;

    dclear(C_WHITE);
    _pxy("Event list:");
    while (true) {
        if (_event_table_iter(&evt) != 0)
            break;
        y = _event_disp_event(evt, y);
    }
    if (evt != NULL) {
        if (evt->type != KEYEV_NONE && evt->type != KEYEV_TOUCH_RELEASE) {
            dpixel(evt->x + 0, evt->y + 0, C_BLACK);
            dpixel(evt->x + 0, evt->y + 1, C_BLACK);
            dpixel(evt->x + 1, evt->y + 0, C_BLACK);
            dpixel(evt->x + 1, evt->y + 1, C_BLACK);
        }
    }
    dupdate();
}

/* event_menu_keyboard() - keyboard */
void event_menu_keyboard(void)
{
    ;
}

//=== Main function ==========================================================//

static int _touch_select_tab(int tab, struct menu *menu)
{
    switch (tab)
    {
        case 0:
            menu->display    = &adconv_menu_display;
            menu->keyboard   = &adconv_menu_keyboard;
            menu->prof_frame = &(menu->prof.adconv_frame);
            menu->prof_peak  = &(menu->prof.adconv_peak);
            break;
        case 1:
            menu->display    = &paint_menu_display;
            menu->keyboard   = &paint_menu_keyboard;
            menu->prof_frame = &(menu->prof.paint_frame);
            menu->prof_peak  = &(menu->prof.paint_peak);
            break;
        case 2:
            menu->display    = &dots_menu_display;
            menu->keyboard   = &dots_menu_keyboard;
            menu->prof_frame = &(menu->prof.dots_frame);
            menu->prof_peak  = &(menu->prof.dots_peak);
            break;
        case 3:
            menu->display    = &world_menu_display;
            menu->keyboard   = &world_menu_keyboard;
            menu->prof_frame = &(menu->prof.world_frame);
            menu->prof_peak  = &(menu->prof.world_peak);
            break;
        case 4:
            menu->display    = &event_menu_display;
            menu->keyboard   = &event_menu_keyboard;
            menu->prof_frame = &(menu->prof.event_frame);
            menu->prof_peak  = &(menu->prof.event_peak);
            break;
        default:
            return -1;
    }
    menu->dirty = true;
    return tab;
}

void gintctl_gint_touch(void)
{
    struct menu menu;

    prof_init();

    adconv_menu_init();
    paint_menu_init();
    dots_menu_init();
    world_menu_init();
    event_menu_init();

    memset(&menu, 0x00, sizeof(struct menu));
    _touch_select_tab(0, &menu);
    while (true)
    {
        *(menu.prof_frame) = prof_exec({menu.display(&menu);});
        if (*(menu.prof_peak) < *(menu.prof_frame))
            *(menu.prof_peak) = *(menu.prof_frame);
        clearevents();
        if (keypressed(KEY_CLEAR)) {
            break;
        }else if (keypressed(KEY_1)) {
            _touch_select_tab(0, &menu);
        } else if (keypressed(KEY_2)) {
            _touch_select_tab(1, &menu);
        } else if (keypressed(KEY_3)) {
            _touch_select_tab(2, &menu);
        } else if (keypressed(KEY_4)) {
            _touch_select_tab(3, &menu);
        } else if (keypressed(KEY_5)) {
            _touch_select_tab(4, &menu);
        } else {
            menu.keyboard();
        }
    }
}

#endif /* GINT_HW_CP */
