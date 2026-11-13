typedef unsigned char Byte;
typedef unsigned int U32;
typedef __WCHAR_TYPE__ Wide;

#define WINAPI __attribute__((stdcall))
#define DELPHI __attribute__((regparm(3)))
#define ASSEMBLY __attribute__((naked, noinline))
#define CALLED_FROM_ASM __attribute__((used, noinline))

extern int WINAPI VirtualProtect(void *, U32, U32, U32 *);
extern int WINAPI FlushInstructionCache(void *, const void *, U32);
extern void *WINAPI GetCurrentProcess(void);
extern int WINAPI GetModuleHandleExW(U32, const Wide *, void **);

typedef struct {
    U32 size, api;
    void *game_module;
    const Wide *game_root, *exe_path, *plugin_root, *config_path;
    void (WINAPI *log)(const Wide *, const Wide *);
} Host;

typedef struct {
    U32 size, required_api;
    Wide id[64], version[32], description[160];
    unsigned long long exclusive_capabilities;
} PluginInfo;

#include "scale_math.h"
#include "ui_transform.h"
extern int WINAPI GetClipCursor(Rect *);
extern int WINAPI ClipCursor(const Rect *);
extern int WINAPI GetClientRect(void *, Rect *);
extern int WINAPI ClientToScreen(void *, Point *);
extern void *WINAPI GetForegroundWindow(void);

typedef struct {
    U32 timestamp, image_size;
    U32 open_choice_call, apply_choice_call;
    U32 add_label, add_choice, get_option, has_option;
    U32 read_param, set_param, language, cfg, restart_init;
    int restart_offset;
    U32 talk_call, get_control, find_control, create_control, free_child;
    U32 image_set_size, scroll_set_size, load_gi, rescale;
    U32 popup_call, pixel_scale, popup_text_site, screen_width;
    int popup_ratio_offset, popup_attempts_offset;
    U32 clip_call, focus_calls[2], focus, window_handle, present_params;
    U32 government_call, station_call, main_hud_call, planet_hud_call, radar_layout_call;
    U32 info_call, film_radar_call, talk_radar_call;
    U32 alloc_rgba, get_pixels, button_set_size, label_font;
    U32 screen_height, graph_set_size;
    U32 radar_alloc_call, graph_alloc, radar_bg_call, gi_set_path, image_set_path;
} GameBuild;

static const GameBuild builds[] = {
    {
        .timestamp = 0x6ac53b97, .image_size = 0x461000,
        .open_choice_call = 0x534fe1, .apply_choice_call = 0x540841,
        .add_label = 0x53a6b4, .add_choice = 0x53a93c,
        .get_option = 0x53b99c, .has_option = 0x53b868,
        .read_param = 0x7b24e4, .set_param = 0x7b2768, .language = 0x81cf48, .cfg = 0x81cfb8,
        .restart_init = 0x53ffc8, .restart_offset = -9,
        .talk_call = 0x5ea178, .get_control = 0x4c5940, .find_control = 0x4c5a08,
        .create_control = 0x4bcb40, .free_child = 0x4c0ca8,
        .image_set_size = 0x499830, .scroll_set_size = 0x4a4b20,
        .load_gi = 0x491038, .rescale = 0x7bb1b8,
        .popup_call = 0x4d74ee, .pixel_scale = 0x7cc2c0, .popup_text_site = 0x4d78b6,
        .screen_width = 0x81cf5c, .popup_ratio_offset = -0x38, .popup_attempts_offset = -0x40,
        .clip_call = 0x7cac2e, .focus_calls = {0x7c6f3f, 0x7ca4e6}, .focus = 0x7c508c,
        .window_handle = 0x81cfa8, .present_params = 0x81d208,
        .government_call = 0x51d27a, .station_call = 0x558537, .main_hud_call = 0x75a266, .planet_hud_call = 0x58af12,
        .radar_layout_call = 0x75f185,
        .info_call = 0x58c1a1, .film_radar_call = 0x52532d,
        .talk_radar_call = 0x5ea154,
        .alloc_rgba = 0x7b931c,
        .get_pixels = 0x7b90cc, .button_set_size = 0x4ad6f0, .label_font = 0x49dfe8,
        .screen_height = 0x81cf60, .graph_set_size = 0x4c1048,
        .radar_alloc_call = 0x7c9dec, .graph_alloc = 0x7b90e0,
        .radar_bg_call = 0x787050, .gi_set_path = 0x493bb4, .image_set_path = 0x499014,
    },
    {
        .timestamp = 0x68eccc46, .image_size = 0x4d1000,
        .open_choice_call = 0x53fe0d, .apply_choice_call = 0x54bfdd,
        .add_label = 0x5456bc, .add_choice = 0x5459c0,
        .get_option = 0x546bc0, .has_option = 0x546a60,
        .read_param = 0x8281c8, .set_param = 0x8284bc, .language = 0x888e70, .cfg = 0x888ee0,
        .restart_init = 0x54b6df, .restart_offset = -5,
        .talk_call = 0x605a53, .get_control = 0x4cefc8, .find_control = 0x4cf0a0,
        .create_control = 0x4b6840, .free_child = 0x4c9738,
        .image_set_size = 0x48a66c, .scroll_set_size = 0x497fdc,
        .load_gi = 0x48398c, .rescale = 0x833400,
        .popup_call = 0x4d5609, .pixel_scale = 0x4c6958, .popup_text_site = 0x4d5a24,
        .screen_width = 0x888e84, .popup_ratio_offset = -0x40, .popup_attempts_offset = -0x48,
        .clip_call = 0x4c4fe8, .focus_calls = {0x4c10fb, 0x4c483c}, .focus = 0x4bf1c8,
        .window_handle = 0x888ed0, .present_params = 0x889130,
        .government_call = 0x5259b8, .station_call = 0x566b3a, .main_hud_call = 0x7a169b, .planet_hud_call = 0x59bed6,
        .radar_layout_call = 0x7a71e4,
        .info_call = 0x59d36e, .film_radar_call = 0x52e415,
        .talk_radar_call = 0x605a28,
        .alloc_rgba = 0x830ec4,
        .get_pixels = 0x830bb8, .button_set_size = 0x4a255c, .label_font = 0x48ff80,
        .screen_height = 0x888e88, .graph_set_size = 0x4c9c9c,
        .radar_alloc_call = 0x4c4134, .graph_alloc = 0x830bdc,
        .radar_bg_call = 0x7d3e32, .gi_set_path = 0x486c38, .image_set_path = 0x489b4c,
    },
};

#define BSTR(name, literal) \
    static const struct { U32 length; Wide text[sizeof(literal) / sizeof(Wide)]; } name = { \
        sizeof(literal) - sizeof(Wide), literal \
    }

BSTR(scale_key, L"FHDDialogWidthPercent");
BSTR(scale_caption_ru, L"Ширина диалогов");
BSTR(scale_caption_en, L"Dialogue width");
BSTR(pct100, L"100%");
BSTR(pct115, L"115%");
BSTR(pct130, L"130%");
BSTR(pct150, L"150%");
BSTR(value100, L"100");
BSTR(value115, L"115");
BSTR(value130, L"130");
BSTR(value150, L"150");

BSTR(hud_key, L"FHDHudScalePercent");
BSTR(hud_caption_ru, L"Масштаб интерфейса");
BSTR(hud_caption_en, L"Interface scale");
BSTR(pct125, L"125%");
BSTR(pct175, L"175%");
BSTR(pct200, L"200%");
BSTR(value125, L"125");
BSTR(value175, L"175");
BSTR(value200, L"200");
BSTR(radar_key, L"FHDMinimapScalePercent");
BSTR(radar_caption_ru, L"Миникарта");
BSTR(radar_caption_en, L"Minimap");
BSTR(radar_normal_ru, L"Нормал");
BSTR(radar_normal_en, L"Normal");
BSTR(radar_150, L"×1.5");
BSTR(radar_200, L"×2");
BSTR(radar_250, L"×2.5");
BSTR(value250, L"250");
BSTR(value110, L"110");
BSTR(legacy_dialog_key, L"FHDGameScale2DialogWidth");

typedef struct {
    int percent;
    const Wide *label, *value;
} ScaleChoice;

static const ScaleChoice scale_choices[] = {
    {100, pct100.text, value100.text},
    {115, pct115.text, value115.text},
    {130, pct130.text, value130.text},
    {150, pct150.text, value150.text},
};
static const ScaleChoice hud_choices[] = {
    {100, pct100.text, value100.text},
    {125, pct125.text, value125.text},
    {150, pct150.text, value150.text},
    {175, pct175.text, value175.text},
    {200, pct200.text, value200.text},
};
static const ScaleChoice radar_choices[] = {
    {100, radar_normal_en.text, value100.text},
    {150, radar_150.text, value150.text},
    {200, radar_200.text, value200.text},
    {250, radar_250.text, value250.text},
};
#define COUNT_OF(array) (sizeof(array) / sizeof((array)[0]))

static const Wide plugin_id[] = L"FHD_GameScale2";
static const GameBuild *build;
static Byte *image;
static int installed;
static int cached_ui_percent;
static U32 native_label __attribute__((used));
static U32 native_choice __attribute__((used));
static U32 native_option __attribute__((used));
static Byte (DELPHI *native_has_option)(void *, const Wide *);
static void (DELPHI *native_read)(void *, const Wide *, const Wide **);
static void (DELPHI *native_write)(void *, const Wide *, const Wide *);
static void (DELPHI *native_clear)(const Wide **);

static void *game_address(U32 preferred)
{
    return image + (preferred - 0x400000);
}

static int same_text(const Wide *left, const Wide *right)
{
    if (!left || !right)
        return 0;
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static int read_percent(const Wide *key, const ScaleChoice *choices, unsigned count)
{
    void *config = *(void **)game_address(build->cfg);
    if (!config)
        return 0;
    const Wide *value = 0;
    native_read(config, key, &value);
    int percent = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (same_text(value, choices[i].value)) {
            percent = choices[i].percent;
            break;
        }
    }
    native_clear(&value);
    return percent;
}

static int read_scale(void)
{
    int percent = read_percent(scale_key.text, scale_choices, COUNT_OF(scale_choices));
    if (percent)
        return percent;
    static const ScaleChoice legacy_choices[] = {
        {100, pct100.text, value100.text},
        {115, pct115.text, value110.text},
        {130, pct130.text, value125.text},
        {150, pct150.text, value150.text},
        {150, pct175.text, value175.text},
    };
    percent = read_percent(legacy_dialog_key.text, legacy_choices, COUNT_OF(legacy_choices));
    return percent ? percent : 100;
}

static int read_hud_scale(void)
{
    int percent = read_percent(hud_key.text, hud_choices, COUNT_OF(hud_choices));
    return percent ? percent : 100;
}

static int read_radar_scale(void)
{
    int percent = read_percent(radar_key.text, radar_choices, COUNT_OF(radar_choices));
    return percent ? percent : 100;
}

static void native_add_label(void *form, const Wide *key, const Wide *caption)
    __attribute__((naked, noipa));
static void native_add_label(void *form __attribute__((unused)), const Wide *key __attribute__((unused)),
                             const Wide *caption __attribute__((unused)))
{
    __asm__("movl 4(%esp), %eax\n"
            "movl 8(%esp), %edx\n"
            "movl 12(%esp), %ecx\n"
            "pushl $1\n"
            "call *native_label\n"
            "ret");
}

static void native_add_choice(void *form, int value, const Wide *label, int selected)
    __attribute__((naked, noipa));
static void native_add_choice(void *form __attribute__((unused)), int value __attribute__((unused)),
                              const Wide *label __attribute__((unused)), int selected __attribute__((unused)))
{
    __asm__("movl 4(%esp), %eax\n"
            "movl 8(%esp), %edx\n"
            "movl 12(%esp), %ecx\n"
            "pushl 16(%esp)\n"
            "pushl $0\n"
            "call *native_choice\n"
            "ret");
}

static int native_get_option(void *form, const Wide *key) __attribute__((naked, noipa));
static int native_get_option(void *form __attribute__((unused)), const Wide *key __attribute__((unused)))
{
    __asm__("movl 4(%esp), %eax\n"
            "movl 8(%esp), %edx\n"
            "jmp *native_option");
}

static void add_choices(void *form, const Wide *key, const Wide *caption,
                        const ScaleChoice *choices, unsigned count, int selected)
{
    native_add_label(form, key, caption);
    for (unsigned i = 0; i < count; ++i)
        native_add_choice(form, choices[i].percent, choices[i].label, choices[i].percent == selected);
}

static CALLED_FROM_ASM void add_scale_settings(void *form)
{
    const Wide *language = *(const Wide **)game_address(build->language);
    int russian = same_text(language, L"russian");
    add_choices(form, hud_key.text, russian ? hud_caption_ru.text : hud_caption_en.text,
                hud_choices, COUNT_OF(hud_choices), read_hud_scale());
    add_choices(form, scale_key.text, russian ? scale_caption_ru.text : scale_caption_en.text,
                scale_choices, COUNT_OF(scale_choices), read_scale());
    int radar = read_radar_scale();
    native_add_label(form, radar_key.text, russian ? radar_caption_ru.text : radar_caption_en.text);
    native_add_choice(form, 100, russian ? radar_normal_ru.text : radar_normal_en.text, radar == 100);
    for (unsigned i = 1; i < COUNT_OF(radar_choices); ++i)
        native_add_choice(form, radar_choices[i].percent, radar_choices[i].label, radar == radar_choices[i].percent);
}

static void save_choice(void *form, Byte *frame, const Wide *key,
                        const ScaleChoice *choices, unsigned count, int current)
{
    if (!native_has_option(form, key))
        return;
    int selected = native_get_option(form, key);
    if (selected == current)
        return;
    void *config = *(void **)game_address(build->cfg);
    if (!config)
        return;
    for (unsigned i = 0; i < count; ++i) {
        if (selected == choices[i].percent) {
            native_write(config, key, choices[i].value);
            frame[build->restart_offset] = 1;
            return;
        }
    }
}

static CALLED_FROM_ASM void apply_scale_settings(void *form, Byte *caller_frame)
{
    save_choice(form, caller_frame, hud_key.text, hud_choices, COUNT_OF(hud_choices), read_hud_scale());
    save_choice(form, caller_frame, scale_key.text, scale_choices, COUNT_OF(scale_choices), read_scale());
    save_choice(form, caller_frame, radar_key.text, radar_choices, COUNT_OF(radar_choices), read_radar_scale());
    cached_ui_percent = 0;
}

static ASSEMBLY void settings_open_hook(void)
{
    __asm__("pushal\n"
            "pushl %eax\n"
            "call add_scale_settings\n"
            "addl $4, %esp\n"
            "popal\n"
            "jmp *native_label");
}

static ASSEMBLY void settings_apply_hook(void)
{
    __asm__("pushal\n"
            "pushl %ebp\n"
            "pushl %eax\n"
            "call apply_scale_settings\n"
            "addl $8, %esp\n"
            "popal\n"
            "jmp *native_option");
}

typedef struct Control Control;
struct Control {
    void **methods;
    Control *first, *last, *previous, *next, *parent;
    void *loop, *source;
    Point position, size;
};

typedef struct {
    void *methods;
    int width, height, pitch;
    void *pixels;
    int storage, bits_per_pixel, bytes_per_pixel;
    Byte use_texture;
} GraphBuffer;

static Control *(DELPHI *native_find)(void *, const Wide *);
static Control *(DELPHI *native_get)(void *, const Wide *);
static Control *(DELPHI *native_create)(const Wide *, Control *);
static void (DELPHI *native_free_child)(Control *, Control *);
static void (DELPHI *native_load_gi)(Control *, const Wide *);
static void (DELPHI *native_rescale)(GraphBuffer *, int, int);
static void (*native_focus_window)(void);
static U32 native_pixel_scale __attribute__((used));

BSTR(image_background_name, L"PanelTalk:ImageBG");
BSTR(image_choice_name, L"PanelTalk:ImageBGB");
BSTR(choice_scroll_name, L"PanelTalk:TalkPA");
BSTR(talk_text_name, L"PanelTalk:TalkText");
BSTR(full_dialog_image, L"GI,Bm.FormTalk2.2Bg");
BSTR(full_choice_image, L"GI,Bm.FormTalk2.2BgB");
BSTR(graph_buffer_name, L"GraphBuf");

static void set_position(Control *control, Point value)
{
    ((void (DELPHI *)(Control *, const Point *))control->methods[0x0c / 4])(control, &value);
}

static void set_size(Control *control, Point value)
{
    ((void (DELPHI *)(Control *, const Point *))control->methods[0x18 / 4])(control, &value);
}

static void set_active(Control *control, Byte active)
{
    ((void (DELPHI *)(Control *, Byte))control->methods[0x28 / 4])(control, active);
}

static int stock_image(Control *control, const Wide *expected)
{
    return control && control->methods[0x18 / 4] == game_address(build->image_set_size) &&
           same_text(*(const Wide **)((Byte *)control + 0x13c), expected) &&
           *(Control **)((Byte *)control + 0x12c);
}

static Control *prepare_art(Control *image_control, Point target);
static void finish_art(Control *image_control, Control *layer, Point size, int percent);
static int is_native_type(Control *control, U32 set_size_address);
static const Wide *image_path(Control *image_control);
static const Wide *control_name(Control *control);

typedef struct {
    int text_start, text_height, text_cut;
    int choices_start, choices_height, choices_cut;
} DialogueRows;

static int general_ui_percent(void);
static int hud_percent(void);
static int screen_height(void);

static int dialogue_screen_height(void)
{
    return screen_height() - fhd_scale_px(82, hud_percent()) - 8;
}

static int shrink_row(int y, int start, int height, int cut)
{
    if (!cut || y <= start)
        return 0;
    return ui_min(y - start, height) * cut / height;
}

static int dialogue_y(int y, DialogueRows rows)
{
    return y - shrink_row(y, rows.text_start, rows.text_height, rows.text_cut) -
           shrink_row(y, rows.choices_start, rows.choices_height, rows.choices_cut);
}

static DialogueRows fit_dialogue_rows(int wanted, Control *text, Control *choices)
{
    DialogueRows rows = {text->position.y, text->size.y, 0,
                         choices->position.y, choices->size.y, 0};
    int text_available = ui_max(0, rows.text_height - 96);
    int choices_available = ui_max(0, rows.choices_height - 72);
    if (wanted <= 0 || rows.text_start + rows.text_height > rows.choices_start)
        return rows;
    wanted = ui_min(wanted, text_available + choices_available);
    rows.text_cut = ui_min(wanted, ui_min(text_available, ui_max(wanted * 2 / 3, wanted - choices_available)));
    rows.choices_cut = wanted - rows.text_cut;
    return rows;
}

static void set_scroll_geometry(Control *scroll, int width, int original, int widened,
                                DialogueRows rows)
{
    Control *scrollbar = *(Control **)((Byte *)scroll + 0x144);
    Point saved_position = scrollbar ? scrollbar->position : (Point){0, 0};
    Point saved_size = scrollbar ? scrollbar->size : (Point){0, 0};
    Rect *bar = (Rect *)((Byte *)scroll + 0x15a);
    int bar_width = bar->right - bar->left;
    int center = fhd_wide_x(bar->left + bar_width / 2, original, widened);
    bar->left = center - bar_width / 2;
    bar->right = bar->left + bar_width;
    bar->top = dialogue_y(bar->top, rows);
    bar->bottom = dialogue_y(bar->bottom, rows);
    Point pos = scroll->position;
    int y = dialogue_y(pos.y, rows);
    int height = dialogue_y(pos.y + scroll->size.y, rows) - y;
    set_position(scroll, (Point){fhd_wide_x(pos.x, original, widened), y});
    set_size(scroll, (Point){width, height});
    if (scrollbar) {
        int bar_y = dialogue_y(saved_position.y, rows);
        set_position(scrollbar, (Point){bar->left, bar_y});
        set_size(scrollbar, (Point){saved_size.x, dialogue_y(saved_position.y + saved_size.y, rows) - bar_y});
    }
}

BSTR(full_gov_image, L"GI,Bm.FormGov2.2TWin");
BSTR(full_gov_choice, L"GI,Bm.FormGov2.2TWinB");

static Control *find_image(Control *parent, const Wide *path)
{
    for (Control *child = parent->first; child; child = child->next) {
        if (stock_image(child, path))
            return child;
    }
    return 0;
}

static int is_gi_art(Control *control)
{
    if (!is_native_type(control, build->image_set_size) ||
        !*(Control **)((Byte *)control + 0x12c))
        return 0;
    const Wide *path = image_path(control);
    return path && path[0] == 'G' && path[1] == 'I' && path[2] == ',';
}

static Control *find_dialog_art(Control *panel, Control *choices, int answer_frame)
{
    Control *stock = find_image(panel, answer_frame ? full_gov_choice.text : full_gov_image.text);
    if (stock)
        return stock;
    Control *match = 0;
    int best = -1;
    for (Control *image = panel->first; image; image = image->next) {
        if (!is_gi_art(image) || image->position.x < -16 || image->position.x > panel->size.x / 4 ||
            image->size.x < panel->size.x * 3 / 4 || image->size.x > panel->size.x + 64)
            continue;
        int bottom = image->position.y + image->size.y;
        if (answer_frame) {
            if (image->position.y < choices->position.y - 128 ||
                image->position.y > choices->position.y + 20 ||
                bottom < choices->position.y + choices->size.y - 48 ||
                bottom > panel->size.y + 64)
                continue;
        } else if (image->position.y < -16 || image->position.y > 24 ||
                   bottom < choices->position.y + choices->size.y - 32 ||
                   bottom > panel->size.y + 64) {
            continue;
        }
        if (image->size.x > best) {
            match = image;
            best = image->size.x;
        }
    }
    return match;
}

// This value lives on our GraphBuf control and disappears with the screen.
#define DIALOG_ART_MARKER 0x46484457u

static int has_dialog_art(Control *image)
{
    for (Control *child = image->first; child; child = child->next) {
        if (*(const U32 *)((const Byte *)child + 0x8c) == DIALOG_ART_MARKER)
            return 1;
    }
    return 0;
}

static Point dialog_art_size(Control *image, int original, int widened, DialogueRows rows)
{
    int y = image->position.y;
    return (Point){fhd_wide_x(image->size.x, original, widened),
        dialogue_y(y + image->size.y, rows) - dialogue_y(y, rows)};
}

static void widen_dialog(void *form, Control *government_label)
{
    int percent = read_scale();
    int ui_percent = general_ui_percent();
    if (percent == 100 && ui_percent == 100)
        return;
    int government = government_label != 0;
    Control *label = government ? government_label : native_find(form, talk_text_name.text);
    Control *text = label ? label->parent : 0;
    Control *panel = text ? text->parent : 0;
    Control *choices = 0;
    if (panel) {
        for (Control *child = panel->first; child; child = child->next) {
            if (same_text(control_name(child), L"TalkPA")) {
                choices = child;
                break;
            }
        }
    }
    if (!choices)
        choices = native_find(form, choice_scroll_name.text);
    if (!panel || !label || !is_native_type(text, build->scroll_set_size) ||
        !is_native_type(choices, build->scroll_set_size) ||
        text->parent != panel || choices->parent != panel || label->parent != text ||
        panel->size.x < 220 || panel->size.x > 1600 || panel->size.y < 260 || panel->size.y > 1800)
        return;
    Control *background = government ? find_dialog_art(panel, choices, 0) :
        native_find(form, image_background_name.text);
    Control *choice_background = government ? find_dialog_art(panel, choices, 1) :
        native_find(form, image_choice_name.text);
    if (!is_gi_art(background) || !is_gi_art(choice_background) || background == choice_background ||
        background->parent != panel || choice_background->parent != panel ||
        has_dialog_art(background) || has_dialog_art(choice_background))
        return;

    if (!government && (!stock_image(background, full_dialog_image.text) ||
        !stock_image(choice_background, full_choice_image.text) ||
        panel->size.x != 428 || background->size.x != 428 ||
        choice_background->size.x != 361 || background->size.y > 1024 ||
        choice_background->size.y > 300))
        return;

    int original = panel->size.x;
    int width = *(int *)game_address(build->screen_width);
    int left = panel->position.x;
    int widened = government ? fhd_gov_width(original, left, width, percent) :
        fhd_dialog_width(original, left, percent);
    int wanted_cut = ui_percent == 100 ? 0 : panel->size.y - dialogue_screen_height() * 100 / ui_percent;
    DialogueRows rows = fit_dialogue_rows(wanted_cut, text, choices);
    int height_cut = rows.text_cut + rows.choices_cut;
    if (widened == original && !height_cut)
        return;

    Point background_size = dialog_art_size(background, original, widened, rows);
    Point choice_size = dialog_art_size(choice_background, original, widened, rows);
    if (background_size.x < 1 || background_size.y < 1 || choice_size.x < 1 || choice_size.y < 1)
        return;
    Control *background_layer = prepare_art(background, background_size);
    if (!background_layer)
        return;
    Control *choice_layer = prepare_art(choice_background, choice_size);
    if (!choice_layer) {
        native_free_child(background, background_layer);
        return;
    }

    left -= government ? (widened - original) / 2 : widened - original;
    set_position(panel, (Point){left, panel->position.y});
    set_size(panel, (Point){widened, panel->size.y - height_cut});
    if (!government)
        *(int *)((Byte *)form + 0x124) = left;
    finish_art(background, background_layer, background_size, 100);
    finish_art(choice_background, choice_layer, choice_size, 100);
    *(U32 *)((Byte *)background_layer + 0x8c) = DIALOG_ART_MARKER;
    *(U32 *)((Byte *)choice_layer + 0x8c) = DIALOG_ART_MARKER;

    Control *text_bar = *(Control **)((Byte *)text + 0x144);
    Control *choices_bar = *(Control **)((Byte *)choices + 0x144);
    for (Control *child = panel->first; child; child = child->next) {
        if (child == text || child == choices || child == text_bar || child == choices_bar)
            continue;
        Point pos = child->position;
        if (child == background || child == choice_background) {
            pos.x = fhd_wide_x(pos.x, original, widened);
            pos.y = dialogue_y(pos.y, rows);
        } else {
            pos.x = fhd_wide_x(pos.x + child->size.x / 2, original, widened) - child->size.x / 2;
            pos.y = dialogue_y(pos.y + child->size.y / 2, rows) - child->size.y / 2;
        }
        set_position(child, pos);
    }

    set_scroll_geometry(text, fhd_wide_x(text->size.x, original, widened), original, widened, rows);
    set_scroll_geometry(choices, fhd_wide_x(choices->size.x, original, widened), original, widened, rows);
    set_size(label, (Point){text->size.x, label->size.y});
}

static Control *DELPHI talk_layout_hook(void *form, const Wide *key)
{
    Control *close = native_get(form, key);
    widen_dialog(form, 0);
    return close;
}

static Control *DELPHI landed_dialog_open_hook(void *form, const Wide *key)
{
    Control *label = native_get(form, key);
    widen_dialog(form, label);
    return label;
}

static Control *DELPHI station_dialog_open_hook(void *form, const Wide *key)
{
    Control *background = native_get(form, key);
    Control *label = native_find(form, talk_text_name.text);
    if (label)
        widen_dialog(form, label);
    return background;
}

#include "hud.inc"
#include "info.inc"

static CALLED_FROM_ASM void widen_popup_ratio(Byte *frame)
{
    int percent = read_scale();
    if (percent > 100) {
        // The stock popup chooses its text wrap and button placement from this ratio.
        float *ratio = (float *)(frame + build->popup_ratio_offset);
        *ratio = *ratio * percent / 100;
    }
}

static ASSEMBLY void popup_ratio_hook(void)
{
    __asm__("pushal\n"
            "pushl %ebp\n"
            "call widen_popup_ratio\n"
            "addl $4, %esp\n"
            "popal\n"
            "jmp *native_pixel_scale");
}

static CALLED_FROM_ASM void cap_popup_text(Point *size, Byte *frame)
{
    // Vanilla stores 1.6178011 as this single-precision value.
    if (*(U32 *)(frame + build->popup_ratio_offset) == 0x3fcf141b)
        return;
    int screen_width = *(int *)game_address(build->screen_width);
    if (screen_width < 800 || screen_width > 16384)
        return;
    int limit = screen_width * 3 / 4;
    if (size->x > limit) {
        size->x = limit;
        *(int *)(frame + build->popup_attempts_offset) = 1;
    }
}

static ASSEMBLY void popup_text_size_hook(void)
{
    __asm__("pushal\n"
            "pushl %ebp\n"
            "pushl %edx\n"
            "call cap_popup_text\n"
            "addl $8, %esp\n"
            "popal\n"
            "movl (%eax), %ecx\n"
            "jmp *24(%ecx)");
}

static int fullscreen_client_rect(void *window, Rect *bounds)
{
    Rect client;
    Point origin = {0, 0};
    if (!window || !GetClientRect(window, &client) ||
        !ClientToScreen(window, &origin) ||
        client.right <= client.left || client.bottom <= client.top)
        return 0;
    *bounds = (Rect){origin.x + client.left, origin.y + client.top,
                     origin.x + client.right, origin.y + client.bottom};
    return 1;
}

static Rect last_clip;
static int clip_owned;

static int WINAPI clip_active_client(const Rect *fallback)
{
    void *window = *(void **)game_address(build->window_handle);
    Rect bounds;
    if (!fullscreen_client_rect(window, &bounds))
        return ClipCursor(fallback);
    int result = ClipCursor(&bounds);
    if (result) {
        last_clip = bounds;
        clip_owned = 1;
    }
    return result;
}

static void focus_window_hook(void)
{
    native_focus_window();
    void *window = *(void **)game_address(build->window_handle);
    if (!window || GetForegroundWindow() != window)
        return;

    int windowed = *(int *)game_address(build->present_params + 0x20);
    if (!windowed) {
        Rect bounds;
        if (fullscreen_client_rect(window, &bounds) && ClipCursor(&bounds)) {
            last_clip = bounds;
            clip_owned = 1;
        }
    } else if (clip_owned) {
        Rect clip;
        if (GetClipCursor(&clip) && fhd_equal_rect(clip, last_clip))
            ClipCursor(0);
        clip_owned = 0;
    }
}

typedef struct {
    Byte *site;
    void (*hook)(void);
    Byte expected[6];
    int length;
    Byte jump;
} Patch;

static void relative_call_bytes(Byte out[5], const Byte *site, U32 target)
{
    U32 delta = target - ((U32)site + 5);
    out[0] = 0xe8;
    for (int i = 0; i < 4; ++i)
        out[i + 1] = (Byte)(delta >> (i * 8));
}

static Patch call_patch(U32 site_address, U32 old_target, void (*hook)(void))
{
    Patch patch = {.site = game_address(site_address), .hook = hook, .length = 5};
    relative_call_bytes(patch.expected, patch.site, (U32)game_address(old_target));
    return patch;
}

static int bytes_match(const Byte *left, const Byte *right, int size)
{
    for (int i = 0; i < size; ++i) {
        if (left[i] != right[i])
            return 0;
    }
    return 1;
}

static void copy_code(volatile Byte *destination, const Byte *source, int length)
{
    for (int i = 0; i < length; ++i)
        destination[i] = source[i];
}

static int apply_patches(Patch *patches, int count)
{
    U32 protection[48];
    if (count < 1 || count > (int)(sizeof(protection) / sizeof(protection[0])))
        return 0;
    for (int i = 0; i < count; ++i) {
        if (!bytes_match(patches[i].site, patches[i].expected, patches[i].length))
            return 0;
    }
    int unlocked = 0;
    while (unlocked < count) {
        if (!VirtualProtect(patches[unlocked].site, patches[unlocked].length, 0x40, &protection[unlocked]))
            break;
        ++unlocked;
    }

    int success = unlocked == count;
    void *process = GetCurrentProcess();
    if (success) {
        for (int i = 0; i < count; ++i) {
            Byte replacement[6] = {0, 0, 0, 0, 0, 0x90};
            relative_call_bytes(replacement, patches[i].site, (U32)patches[i].hook);
            if (patches[i].jump)
                replacement[0] = 0xe9;
            copy_code(patches[i].site, replacement, patches[i].length);
        }
        if (!FlushInstructionCache(process, 0, 0)) {
            for (int i = 0; i < count; ++i)
                copy_code(patches[i].site, patches[i].expected, patches[i].length);
            FlushInstructionCache(process, 0, 0);
            success = 0;
        }
    }

    while (unlocked > 0) {
        --unlocked;
        U32 unused;
        VirtualProtect(patches[unlocked].site, patches[unlocked].length, protection[unlocked], &unused);
    }
    return success;
}

#include "global_ui.inc"

static const GameBuild *identify_build(const Byte *module)
{
    if (*(const unsigned short *)module != 0x5a4d)
        return 0;
    U32 header = *(const U32 *)(module + 0x3c);
    if (header < 0x40 || header > 0xf00 || *(const U32 *)(module + header) != 0x4550 ||
        *(const unsigned short *)(module + header + 4) != 0x014c ||
        *(const unsigned short *)(module + header + 24) != 0x010b)
        return 0;
    U32 stamp = *(const U32 *)(module + header + 8);
    U32 image_size = *(const U32 *)(module + header + 80);
    for (U32 i = 0; i < sizeof(builds) / sizeof(builds[0]); ++i) {
        if (builds[i].timestamp == stamp && builds[i].image_size == image_size)
            return &builds[i];
    }
    return 0;
}

static void report(const Host *host, const Wide *message)
{
    if (host->log)
        host->log(plugin_id, message);
}

static void copy_wide(Wide *destination, const Wide *source)
{
    while ((*destination++ = *source++))
        ;
}

int WINAPI XenoPlugin_Query(PluginInfo *info)
{
    if (!info || info->size < 520)
        return 0;
    info->required_api = 1;
    copy_wide(info->id, plugin_id);
    copy_wide(info->version, L"0.5.7");
    copy_wide(info->description, L"Interface scaling, independent dialogue width and fullscreen cursor bounds");
    if (info->size >= 528)
        info->exclusive_capabilities = 0;
    return 1;
}

U32 WINAPI XenoPlugin_Initialize(const Host *host)
{
    if (!host || host->size < 32 || host->api != 1 || !host->game_module)
        return 87;
    if (installed)
        return 0;
    image = host->game_module;
    build = identify_build(image);
    if (!build) {
        report(host, L"GameScale2 inactive: unsupported Rangers.exe");
        return 0;
    }

    const Byte restart_guard[] = {0xc6, 0x45, (Byte)build->restart_offset, 0x00};
    const Byte clear_guard[] = {0x8b, 0x10, 0x85, 0xd2, 0x74, 0x0e};
    if (!bytes_match(game_address(build->restart_init), restart_guard, sizeof(restart_guard)) ||
        !bytes_match(game_address(0x405d18), clear_guard, sizeof(clear_guard))) {
        report(host, L"GameScale2 inactive: unexpected settings code");
        return 0;
    }

    native_label = (U32)game_address(build->add_label);
    native_choice = (U32)game_address(build->add_choice);
    native_option = (U32)game_address(build->get_option);
    native_has_option = game_address(build->has_option);
    native_read = game_address(build->read_param);
    native_write = game_address(build->set_param);
    native_clear = game_address(0x405d18);
    native_get = game_address(build->get_control);
    native_find = game_address(build->find_control);
    native_create = game_address(build->create_control);
    native_free_child = game_address(build->free_child);
    native_load_gi = game_address(build->load_gi);
    native_rescale = game_address(build->rescale);
    native_pixel_scale = (U32)game_address(build->pixel_scale);
    native_focus_window = game_address(build->focus);

    native_alloc_rgba = game_address(build->alloc_rgba);
    native_get_pixels = game_address(build->get_pixels);
    native_label_font = game_address(build->label_font);

    void *plugin_module = 0;
    if (!GetModuleHandleExW(5, (const Wide *)XenoPlugin_Initialize, &plugin_module)) {
        report(host, L"GameScale2 inactive: could not retain plugin");
        return 0;
    }
    Patch settings[] = {
        call_patch(build->open_choice_call, build->add_label, settings_open_hook),
        call_patch(build->apply_choice_call, build->get_option, settings_apply_hook),
    };
    if (!apply_patches(settings, COUNT_OF(settings))) {
        report(host, L"GameScale2 inactive: graphics settings call already modified");
        return 0;
    }
    installed = 1;
    report(host, L"GameScale2 0.5.3: interface and dialogue settings installed");

    Patch government[] = {
        call_patch(build->government_call, build->get_control, (void (*)(void))landed_dialog_open_hook),
    };
    if (!apply_patches(government, COUNT_OF(government)))
        report(host, L"GameScale2: government dialogue hook skipped");

    // Station OnOpen builds answer rows from TalkPA's current width.
    // Apply the layout before it starts building those rows.
    Patch station[] = {
        call_patch(build->station_call, build->get_control, (void (*)(void))station_dialog_open_hook),
    };
    if (!apply_patches(station, COUNT_OF(station)))
        report(host, L"GameScale2: station dialogue hook skipped");

    Patch talk[] = {
        call_patch(build->talk_call, build->get_control, (void (*)(void))talk_layout_hook),
    };
    if (!apply_patches(talk, COUNT_OF(talk)))
        report(host, L"GameScale2: ship dialogue hook skipped");

    Patch popup[] = {
        call_patch(build->popup_call, build->pixel_scale, popup_ratio_hook),
        {game_address(build->popup_text_site), popup_text_size_hook, {0x8b, 0x08, 0xff, 0x51, 0x18}, 5, 0},
    };
    if (!apply_patches(popup, COUNT_OF(popup)))
        report(host, L"GameScale2: standard popup hooks skipped");

    Patch info[] = {
        call_patch(build->info_call, build->get_control, (void (*)(void))info_layout_hook),
    };
    if (!apply_patches(info, COUNT_OF(info)))
        report(host, L"GameScale2: information center layout hook skipped");

    Patch bottom[] = {
        call_patch(build->main_hud_call, build->get_control, (void (*)(void))main_hud_hook),
        call_patch(build->planet_hud_call, build->get_control, (void (*)(void))planet_hud_hook),
    };
    if (!apply_patches(bottom, COUNT_OF(bottom)))
        report(host, L"GameScale2: bottom panel hooks skipped");

    Patch radar[] = {
        call_patch(build->radar_alloc_call, build->graph_alloc, (void (*)(void))allocate_radar_buffer),
        call_patch(build->radar_bg_call, build->gi_set_path, (void (*)(void))set_radar_background),
        call_patch(build->radar_layout_call, build->get_control, (void (*)(void))radar_layout_hook),
        call_patch(build->film_radar_call, build->get_control, (void (*)(void))film_radar_layout_hook),
        call_patch(build->talk_radar_call, build->get_control, (void (*)(void))talk_radar_layout_hook),
    };
    if (!apply_patches(radar, COUNT_OF(radar)))
        report(host, L"GameScale2: minimap layout hooks skipped");

    if (install_global_ui())
        report(host, L"GameScale2: general interface scaling installed for hardware rendering");
    else
        report(host, L"GameScale2: general interface scaling skipped; using the standard HUD path");

    Patch cursor[] = {
        call_patch(build->clip_call, 0x408354, (void (*)(void))clip_active_client),
        call_patch(build->focus_calls[0], build->focus, focus_window_hook),
        call_patch(build->focus_calls[1], build->focus, focus_window_hook),
    };
    if (!apply_patches(cursor, COUNT_OF(cursor)))
        report(host, L"GameScale2: fullscreen cursor hooks skipped");
    return 0;
}
