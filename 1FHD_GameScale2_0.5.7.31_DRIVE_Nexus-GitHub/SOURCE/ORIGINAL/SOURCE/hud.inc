static void (DELPHI *native_alloc_rgba)(GraphBuffer *, int, int);
static void *(DELPHI *native_get_pixels)(GraphBuffer *);
static void (DELPHI *native_label_font)(Control *, const Wide *);
typedef struct {
    Control *map, *parent, *border, *blind, *center, *jamming;
    void *loop;
    Control *find_quest;
} MinimapLayout;

enum { radar_flight, radar_film, radar_conversation, radar_layout_count };
static MinimapLayout minimaps[radar_layout_count];

BSTR(nav_background, L"PM_ImageBG");
BSTR(nav_background_path, L"GI,Bm.PanelMain2.2BG");
BSTR(planet_panel_name, L"PanelPlanet");
BSTR(ruins_panel_name, L"PanelRuins");
BSTR(nav_ship, L"PM_Ship");
BSTR(nav_menu, L"PM_Logo");
BSTR(planet_gov, L"PP_Gov");
BSTR(ruins_gov, L"PR_Gov");
BSTR(planet_image_path, L"GI,Bm.PanelMain2.2Planet");
BSTR(radar_center, L"CenterShip");
BSTR(radar_center_player, L"CenterPlayer");
BSTR(radar_blind, L"MapPanelA");
BSTR(radar_partner, L"MapPartnerDuty");
BSTR(radar_border_path, L"GI,Bm.PanelSpace2.2RadarF");
BSTR(radar_jamming_name, L"RadarJamming");
BSTR(radar_hp_name, L"HPbar_panel");
BSTR(find_quest_panel, L"FHDFindQuestControl");
BSTR(find_quest_button, L"FHDFindQuestButton");
BSTR(radar_frame_150, L"Alpha,Bm.FHDGameScale2.Radar150.Frame");
BSTR(radar_frame_200, L"Alpha,Bm.FHDGameScale2.Radar200.Frame");
BSTR(radar_frame_250, L"Alpha,Bm.FHDGameScale2.Radar250.Frame");
BSTR(radar_blind_150, L"Alpha,Bm.FHDGameScale2.Radar150.Blind");
BSTR(radar_blind_200, L"Alpha,Bm.FHDGameScale2.Radar200.Blind");
BSTR(radar_blind_250, L"Alpha,Bm.FHDGameScale2.Radar250.Blind");
BSTR(radar_bg_150, L"Bm.FHDGameScale2.Radar150.Background");
BSTR(radar_bg_200, L"Bm.FHDGameScale2.Radar200.Background");
BSTR(radar_bg_250, L"Bm.FHDGameScale2.Radar250.Background");
BSTR(radar_jam_150, L"GAI,Bm.FHDGameScale2.Radar150.Jamming");
BSTR(radar_jam_200, L"GAI,Bm.FHDGameScale2.Radar200.Jamming");
BSTR(radar_jam_250, L"GAI,Bm.FHDGameScale2.Radar250.Jamming");
BSTR(radar_talk_150, L"GI,Bm.FHDGameScale2.Radar150.CommPanel");
BSTR(radar_talk_200, L"GI,Bm.FHDGameScale2.Radar200.CommPanel");
BSTR(radar_talk_250, L"GI,Bm.FHDGameScale2.Radar250.CommPanel");
BSTR(radar_close_normal, L"GI,Bm.FHDGameScale2.CommCloseN");
BSTR(radar_close_hover, L"GI,Bm.FHDGameScale2.CommCloseA");
BSTR(radar_close_pressed, L"GI,Bm.FHDGameScale2.CommCloseD");
BSTR(radar_close_disabled, L"GI,Bm.FHDGameScale2.CommCloseH");
BSTR(pilot_stars_left, L"GI,Bm.FHDGameScale2.PilotStarsLeft");
BSTR(pilot_stars_right, L"GI,Bm.FHDGameScale2.PilotStarsRight");
BSTR(hud_font_mid, L"Font.Verdana11");
BSTR(hud_font_large, L"Font.Verdana13");

static int screen_width(void)
{
    return *(int *)game_address(build->screen_width);
}

static int screen_height(void)
{
    return *(int *)game_address(build->screen_height);
}

static const Wide *loop_name(const void *loop)
{
    return loop ? *(const Wide *const *)((const Byte *)loop + 0x04) : 0;
}

static int screen_ui_percent(const void *loop, int percent)
{
    return same_text(loop_name(loop), L"Galaxy") ? ui_min(percent, 125) : percent;
}

static int hud_percent(const void *loop)
{
    return fhd_hud_percent(screen_width(), screen_ui_percent(loop, read_hud_scale()));
}

static Point scaled_point(Point p, int percent)
{
    return (Point){fhd_scale_px(p.x, percent), fhd_scale_px(p.y, percent)};
}

static void set_origin(Control *control, Point value)
{
    ((void (DELPHI *)(Control *, const Point *))control->methods[0x1c / 4])(control, &value);
}

static void mark_decoration(Control *control)
{
    ((void (DELPHI *)(Control *, Byte))control->methods[0x2c / 4])(control, 1);
}

static GraphBuffer *graph_buffer(Control *control)
{
    return *(GraphBuffer **)((Byte *)control + 0x120);
}

static int is_native_type(Control *control, U32 set_size_address)
{
    return control && control->methods[0x18 / 4] == game_address(set_size_address);
}

static const Wide *image_path(Control *image_control)
{
    return *(const Wide **)((Byte *)image_control + 0x13c);
}

static Control *load_gi_art(Control *image_control, const Wide *path, Point size)
{
    if (size.x < 1 || size.y < 1)
        return 0;
    struct { U32 bytes; Wide key[160]; } value;
    unsigned i = 0;
    while (i + 1 < COUNT_OF(value.key) && path[i + 3]) {
        value.key[i] = path[i + 3];
        ++i;
    }
    if (!i || path[i + 3])
        return 0;
    value.key[i] = 0;
    value.bytes = i * sizeof(Wide);
    Control *layer = native_create(graph_buffer_name.text, image_control);
    if (!layer)
        return 0;
    set_size(layer, size);
    mark_decoration(layer);
    native_load_gi(layer, value.key);
    GraphBuffer *pixels = graph_buffer(layer);
    if (!pixels || pixels->width <= 0 || pixels->height <= 0) {
        native_free_child(image_control, layer);
        return 0;
    }
    return layer;
}

static Control *load_ui_image(Control *image_control)
{
    return is_gi_art(image_control) ?
        load_gi_art(image_control, image_path(image_control), image_control->size) : 0;
}

static Control *resize_art(Control *image_control, Control *layer, Point target)
{
    if (!layer)
        return 0;
    GraphBuffer *pixels = graph_buffer(layer);
    native_rescale(pixels, target.x, target.y);
    if (pixels->width != target.x || pixels->height != target.y) {
        native_free_child(image_control, layer);
        return 0;
    }
    set_size(layer, target);
    return layer;
}

static Control *prepare_art(Control *image_control, Point target)
{
    return resize_art(image_control, load_ui_image(image_control), target);
}

static Control *prepare_talk_art(Control *background, TalkRadarArt art, Point target, DialogueRows rows)
{
    Control *source = load_gi_art(background, art.path, art.size);
    if (!source || (art.size.x == target.x && art.size.y == target.y))
        return source;
    GraphBuffer *from = graph_buffer(source);
    Control *layer = 0;
    if (from->width == art.size.x && from->height == art.size.y && from->bytes_per_pixel == 4)
        layer = native_create(graph_buffer_name.text, background);
    if (layer) {
        GraphBuffer *to = graph_buffer(layer);
        native_alloc_rgba(to, target.x, target.y);
        const Byte *input = native_get_pixels(from);
        Byte *output = native_get_pixels(to);
        if (!input || !output) {
            native_free_child(background, layer);
            layer = 0;
        } else {
            int added = target.x - art.size.x;
            int seam = talk_radar_seam + added;
            int source_y = 0;
            for (int y = 0; y < target.y; ++y) {
                while (source_y + 1 < from->height && dialogue_y(source_y + 1 - rows.offset, rows) <= y)
                    ++source_y;
                const U32 *in = (const U32 *)(input + source_y * from->pitch);
                U32 *out = (U32 *)(output + y * to->pitch);
                for (int x = 0; x < seam; ++x)
                    out[x] = in[x * talk_radar_seam / seam];
                for (int x = seam; x < target.x; ++x)
                    out[x] = in[x - added];
            }
            *((Byte *)layer + 0x127) = 1;
            set_size(layer, target);
            mark_decoration(layer);
        }
    }
    native_free_child(background, source);
    return layer;
}

static int fill_portrait_strips(Control *background, Control *layer, const Rect sides[2])
{
    GraphBuffer *target = graph_buffer(layer);
    if (!target || target->bytes_per_pixel != 4)
        return 0;
    int filled = 0;
    const Wide *paths[] = {pilot_stars_left.text, pilot_stars_right.text};
    Rect bounds = {0, 0, target->width, target->height};
    for (unsigned side = 0; side < COUNT_OF(paths); ++side) {
        Rect strip;
        if (!ui_intersection(&strip, sides[side], bounds))
            continue;
        int w = strip.right - strip.left, h = strip.bottom - strip.top;
        Control *texture = load_gi_art(background, paths[side], (Point){320, 224});
        if (!texture)
            continue;
        GraphBuffer *source = graph_buffer(texture);
        const Byte *input = native_get_pixels(source);
        Byte *output = native_get_pixels(target);
        if (input && output && source->bytes_per_pixel == 4) {
            int sample_width = ui_min(source->width, ui_max(1, source->height * w / h));
            int sample_left = (source->width - sample_width) / 2;
            for (int y = 0; y < h; ++y) {
                const U32 *in = (const U32 *)(input + (y * source->height / h) * source->pitch);
                U32 *out = (U32 *)(output + (strip.top + y) * target->pitch);
                for (int x = 0; x < w; ++x) {
                    if (!(out[strip.left + x] >> 24))
                        out[strip.left + x] = in[sample_left + x * sample_width / w] | 0xff000000u;
                }
            }
            ++filled;
        }
        native_free_child(background, texture);
    }
    return filled;
}

static void fill_pilot_portrait(Control *background, Control *layer, Control *portrait, Rect opening)
{
    const Rect sides[] = {
        {opening.left, opening.top, portrait->position.x, opening.bottom},
        {portrait->position.x + portrait->size.x, opening.top, opening.right, opening.bottom},
    };
    fill_portrait_strips(background, layer, sides);
}

static void finish_art(Control *image_control, Control *layer, Point size, int percent)
{
    Point origin = scaled_point(*(Point *)((Byte *)image_control + 0x30), percent);
    set_origin(image_control, origin);
    set_size(image_control, size);
    set_position(layer, (Point){-origin.x, -origin.y});
    set_active(*(Control **)((Byte *)image_control + 0x12c), 0);
}

static int scale_art(Control *image_control, int percent)
{
    Point target = scaled_point(image_control->size, percent);
    Control *layer = prepare_art(image_control, target);
    if (!layer)
        return 0;
    finish_art(image_control, layer, target, percent);
    return 1;
}

static int scale_button_axes(Control *button, int horizontal, int vertical, Point target)
{
    Control **states = (Control **)((Byte *)button + 0x140);
    Point *offsets = (Point *)((Byte *)button + 0x160);
    Control *layers[7] = {0};
    Point sizes[7];
    if (!states[0])
        return 0;
    for (int i = 0; i < 7; ++i) {
        if (!states[i])
            continue;
        sizes[i] = (Point){fhd_scale_px(states[i]->size.x, horizontal), fhd_scale_px(states[i]->size.y, vertical)};
        layers[i] = prepare_art(states[i], sizes[i]);
        if (!layers[i]) {
            for (int j = 0; j < i; ++j) {
                if (layers[j])
                    native_free_child(states[j], layers[j]);
            }
            return 0;
        }
    }
    for (int i = 0; i < 7; ++i) {
        if (layers[i]) {
            offsets[i] = (Point){fhd_scale_px(offsets[i].x, horizontal), fhd_scale_px(offsets[i].y, vertical)};
            Point origin = *(Point *)((Byte *)states[i] + 0x30);
            set_origin(states[i], (Point){fhd_scale_px(origin.x, horizontal), fhd_scale_px(origin.y, vertical)});
            finish_art(states[i], layers[i], sizes[i], 100);
        }
    }
    *((Byte *)button + 0x121) = 0; // Native rectangle hit test uses the enlarged button.
    Point origin = *(Point *)((Byte *)button + 0x30);
    set_origin(button, (Point){fhd_scale_px(origin.x, horizontal), fhd_scale_px(origin.y, vertical)});
    set_size(button, target);
    return 1;
}

static int scale_button(Control *button, int percent)
{
    return scale_button_axes(button, percent, percent, scaled_point(button->size, percent));
}

static Control *prepare_nav_background(Control *background, int percent)
{
    Control *source = load_ui_image(background);
    if (!source)
        return 0;
    GraphBuffer *source_graph = graph_buffer(source);
    int width = background->size.x;
    int height = fhd_scale_px(background->size.y, percent);
    Control *layer = 0;
    if (source_graph->width == width && source_graph->bytes_per_pixel == 4)
        layer = native_create(graph_buffer_name.text, background);
    if (layer) {
        GraphBuffer *target = graph_buffer(layer);
        native_alloc_rgba(target, width, height);
        Byte *from = native_get_pixels(source_graph);
        Byte *to = native_get_pixels(target);
        if (!from || !to) {
            native_free_child(background, layer);
            layer = 0;
        } else {
            for (int y = 0; y < height; ++y) {
                const U32 *input = (const U32 *)(from + (y * source_graph->height / height) * source_graph->pitch);
                U32 *output = (U32 *)(to + y * target->pitch);
                for (int x = 0; x < width; ++x)
                    output[x] = input[fhd_nav_source_x(x, width, percent)];
            }
            *((Byte *)layer + 0x127) = 1;
            set_size(layer, (Point){width, height});
            mark_decoration(layer);
        }
    }
    native_free_child(background, source);
    return layer;
}

static const Wide *control_name(Control *control)
{
    return *(const Wide **)((Byte *)control + 0x78);
}

static void resize_status_label(Control *label, Point size, int percent)
{
    set_size(label, size);
    native_label_font(label, percent >= 175 ? hud_font_large.text : hud_font_mid.text);
}

static void scale_date_panel(Control *panel, int percent)
{
    set_size(panel, scaled_point(panel->size, percent));
    for (Control *child = panel->first; child; child = child->next) {
        set_position(child, scaled_point(child->position, percent));
        if (is_native_type(child, build->image_set_size))
            scale_art(child, percent);
        else if (same_text(control_name(child), L"PM_Date") || same_text(control_name(child), L"PM_DateNew"))
            resize_status_label(child, scaled_point(child->size, percent), percent);
    }
}

static int is_status_label(const Wide *name)
{
    return same_text(name, L"PM_Money") || same_text(name, L"PM_FreeSpace") ||
           same_text(name, L"PM_Help");
}

static int is_native_hud_art(const Wide *name)
{
    return same_text(name, nav_background.text) || same_text(name, L"PM_PanelDate") ||
           is_status_label(name);
}

static UiTransform nav_child_transform(Rect bar, Rect child, int percent)
{
    int left = child.left - bar.left;
    int width = bar.right - bar.left;
    if (left <= 430 || left >= width - 540) {
        Point pivot = {left <= 430 ? bar.left : bar.right, bar.top};
        return ui_place(percent, pivot, pivot);
    }
    return ui_place(percent, (Point){child.left, bar.top},
                    (Point){bar.left + fhd_nav_x(left, width, percent), bar.top});
}

static Control *DELPHI main_hud_hook(void *form, const Wide *key)
{
    Control *messages = native_get(form, key);
    int percent = hud_percent(form);
    if (!messages || percent == 100)
        return messages;
    Control *background = native_find(form, nav_background.text);
    if (!stock_image(background, nav_background_path.text) || !background->parent ||
        background->size.x != screen_width() || background->parent->size.x != screen_width() ||
        background->size.y < 70 || background->size.y > 100 || background->parent->size.y != 82)
        return messages;

    Control *layer = prepare_nav_background(background, percent);
    if (!layer)
        return messages;
    int deferred = general_ui_percent() > 100;
    Control *bar = background->parent;
    int width = bar->size.x;
    int height = fhd_scale_px(bar->size.y, percent);
    set_position(bar, (Point){bar->position.x, bar->position.y + bar->size.y - height});
    set_size(bar, (Point){width, height});
    finish_art(background, layer, layer->size, 100);

    for (Control *child = bar->first; child; child = child->next) {
        if (child == background)
            continue;
        const Wide *name = control_name(child);
        if (deferred && !is_native_hud_art(name))
            continue;
        int old_x = child->position.x;
        int x = fhd_nav_x(old_x, width, percent);
        Point size = {fhd_nav_x(old_x + child->size.x, width, percent) - x,
                      fhd_scale_px(child->size.y, percent)};
        if (is_native_type(child, build->button_set_size)) {
            scale_button(child, percent);
        } else if (is_native_type(child, build->image_set_size)) {
            scale_art(child, percent);
        } else if (same_text(name, L"PM_PanelDate")) {
            scale_date_panel(child, percent);
        } else if (is_status_label(name)) {
            resize_status_label(child, size, percent);
        } else {
            continue;
        }
        set_position(child, (Point){x, fhd_scale_px(child->position.y, percent)});
    }
    if (deferred) {
        int left = messages->position.x;
        int span = fhd_nav_x(left + messages->size.x, width, percent) -
                   fhd_nav_x(left, width, percent);
        // RebuildMessageButtons uses this width when choosing how many to show.
        set_size(messages, (Point){ui_max(0, span) * 100 / percent, messages->size.y});
    } else {
        int bottom = screen_height();
        set_position(messages, (Point){fhd_nav_x(messages->position.x, width, percent),
                                       bottom - fhd_scale_px(bottom - messages->position.y, percent)});
    }
    return messages;
}

static Rect hud_rect(Control *control)
{
    Point position = control->position;
    for (Control *p = control->parent; p; p = p->parent) {
        position.x += p->position.x;
        position.y += p->position.y;
    }
    Point origin = *(const Point *)((const Byte *)control + 0x30);
    int left = position.x - origin.x;
    int top = position.y - origin.y;
    return (Rect){left, top, left + control->size.x, top + control->size.y};
}

static Rect hud_button_rect(Control *button)
{
    Rect bounds = hud_rect(button);
    Control **states = (Control **)((Byte *)button + 0x140);
    for (int i = 0; i < 6; ++i) {
        if (!states[i])
            continue;
        Rect art = hud_rect(states[i]);
        bounds.left = ui_min(bounds.left, art.left);
        bounds.top = ui_min(bounds.top, art.top);
        bounds.right = ui_max(bounds.right, art.right);
        bounds.bottom = ui_max(bounds.bottom, art.bottom);
    }
    return bounds;
}

static Point dock_row_offset(void *form, Control *panel, Control *station, Control *ship,
                             int percent, int deferred, Rect group, UiTransform placement)
{
    if (!is_native_type(ship, build->button_set_size) || !ship->parent ||
        !is_native_type(station, build->button_set_size) || station->parent != panel)
        return (Point){0, 0};

    Rect reference = hud_rect(ship);
    Rect first = hud_rect(station);
    Rect bar = hud_rect(ship->parent);
    if (deferred)
        reference = ui_screen_rect(nav_child_transform(bar, reference, percent), reference);
    first = ui_screen_rect(placement, first);
    group = ui_screen_rect(placement, group);
    int dy = (reference.top + reference.bottom - first.top - first.bottom) / 2;
    int row_top = group.top + dy;
    int row_bottom = group.bottom + dy;
    int right_limit = reference.left;
    for (Control *button = ship->parent->first; button; button = button->next) {
        int graph_button = is_native_type(button, build->button_set_size);
        if (!graph_button && (!deferred || is_native_hud_art(control_name(button))))
            continue;
        Rect bounds = graph_button ? hud_button_rect(button) : hud_rect(button);
        if (deferred)
            bounds = ui_screen_rect(nav_child_transform(bar, hud_rect(button), percent), bounds);
        if (bounds.left >= screen_width() / 2 && bounds.bottom > row_top && bounds.top < row_bottom)
            right_limit = ui_min(right_limit, bounds.left);
    }

    int gap = fhd_scale_px(6, percent);
    int dx = ui_min(0, right_limit - gap - group.right);
    Control *menu = native_find(form, nav_menu.text);
    int left_limit = gap;
    if (is_native_type(menu, build->button_set_size)) {
        Rect bounds = hud_button_rect(menu);
        if (deferred)
            bounds = ui_screen_rect(nav_child_transform(bar, hud_rect(menu), percent), bounds);
        if (bounds.bottom > row_top && bounds.top < row_bottom)
            left_limit = bounds.right + gap;
    }
    if (group.left + dx < left_limit) {
        dy = ui_min(dy, bar.top - gap - group.bottom);
        dx = ui_max(gap - group.left, ui_min(0, screen_width() - gap - group.right));
    }
    return (Point){dx, dy};
}

static Point dock_button_offset(void *form, Control *panel, int percent, int deferred)
{
    Rect group = hud_rect(panel);
    Point place = {group.left, group.top};
    UiTransform placement = deferred ? ui_place(percent, place, place) : ui_identity;
    return dock_row_offset(form, panel, native_find(form, planet_gov.text),
                           native_find(form, nav_ship.text), percent, deferred, group, placement);
}

static Control *DELPHI planet_hud_hook(void *form, const Wide *key)
{
    Control *result = native_get(form, key);
    int percent = hud_percent(form);
    if (percent == 100)
        return result;
    Control *panel = native_find(form, planet_panel_name.text);
    if (!panel || panel->size.x < 220 || panel->size.x > 520 || panel->size.y < 30 || panel->size.y > 90)
        return result;
    int deferred = general_ui_percent() > 100;
    if (deferred) {
        int right = screen_width();
        int bottom = screen_height();
        set_position(panel, (Point){right - fhd_scale_px(right - panel->position.x, percent),
                                   bottom - fhd_scale_px(bottom - panel->position.y, percent)});
        Control *ship = native_find(form, nav_ship.text);
        int scaled_bar = ship && ship->parent && ship->parent->size.y == fhd_scale_px(82, percent);
        if (scaled_bar) {
            Point delta = dock_button_offset(form, panel, percent, 1);
            set_position(panel, (Point){panel->position.x + delta.x, panel->position.y + delta.y});
        }
        return result;
    }
    Control *background = find_image(panel, planet_image_path.text);
    if (background && scale_art(background, percent)) {
        for (Control *child = panel->first; child; child = child->next) {
            if (!is_native_type(child, build->button_set_size))
                continue;
            Point pos = scaled_point(child->position, percent);
            Point size = child->size;
            if (!scale_button(child, percent)) {
                pos.x += (fhd_scale_px(size.x, percent) - size.x) / 2;
                pos.y += (fhd_scale_px(size.y, percent) - size.y) / 2;
            }
            set_position(child, pos);
        }
        int right = screen_width();
        int bottom = screen_height();
        set_position(panel, (Point){right - fhd_scale_px(right - panel->position.x, percent),
                                   bottom - fhd_scale_px(bottom - panel->position.y, percent)});
        set_size(panel, scaled_point(panel->size, percent));
    }
    Point delta = dock_button_offset(form, panel, percent, 0);
    set_position(panel, (Point){panel->position.x + delta.x, panel->position.y + delta.y});
    return result;
}

typedef struct {
    int percent, map_size, buffer_size;
    const Wide *frame, *blind, *background, *jamming;
    const Wide *talk;
    int talk_width;
} RadarPreset;

static const RadarPreset radar_presets[] = {
    {150, 223, 234, radar_frame_150.text, radar_blind_150.text, radar_bg_150.text, radar_jam_150.text,
     radar_talk_150.text, 482},
    {200, 297, 304, radar_frame_200.text, radar_blind_200.text, radar_bg_200.text, radar_jam_200.text,
     radar_talk_200.text, 565},
    {250, 371, 378, radar_frame_250.text, radar_blind_250.text, radar_bg_250.text, radar_jam_250.text,
     radar_talk_250.text, 639},
};
static const RadarPreset *active_radar;

static TalkRadarArt talk_radar_art(void *form)
{
    const MinimapLayout *layout = &minimaps[radar_conversation];
    if (!active_radar || !layout->map || layout->loop != form ||
        layout->map->size.x != active_radar->map_size)
        return (TalkRadarArt){0};
    int extra = active_radar->map_size - 149;
    return (TalkRadarArt){active_radar->talk, {active_radar->talk_width, 540 + extra}, extra};
}

static void DELPHI allocate_radar_buffer(GraphBuffer *buffer, int width, int height)
{
    active_radar = 0;
    int percent = read_radar_scale();
    for (unsigned i = 0; i < COUNT_OF(radar_presets); ++i) {
        const RadarPreset *preset = &radar_presets[i];
        if (preset->percent == percent) {
            active_radar = preset;
            width = height = preset->buffer_size;
            break;
        }
    }
    for (unsigned i = 0; i < COUNT_OF(minimaps); ++i)
        minimaps[i] = (MinimapLayout){0};
    ((void (DELPHI *)(GraphBuffer *, int, int))game_address(build->graph_alloc))(buffer, width, height);
}

static void DELPHI set_radar_background(Control *background, const Wide *stock)
{
    const Wide *path = active_radar ? active_radar->background : stock;
    ((void (DELPHI *)(Control *, const Wide *))game_address(build->gi_set_path))(background, path);
}

static void set_radar_image(Control *control, const Wide *path)
{
    ((void (DELPHI *)(Control *, const Wide *))game_address(build->image_set_path))(control, path);
}

static void set_talk_art(Control *background, Control *close, TalkRadarArt art)
{
    set_radar_image(background, art.path);
    if (!is_native_type(close, build->button_set_size))
        return;
    const Wide *paths[] = {radar_close_normal.text, radar_close_hover.text, radar_close_pressed.text,
                           0, radar_close_disabled.text};
    Control **states = (Control **)((Byte *)close + 0x140);
    for (unsigned i = 0; i < COUNT_OF(paths); ++i) {
        if (paths[i] && is_native_type(states[i], build->image_set_size))
            set_radar_image(states[i], paths[i]);
    }
}

static void set_radar_square(Control *control, Point center, int size)
{
    set_position(control, center);
    set_size(control, (Point){size, size});
    set_origin(control, (Point){size / 2, size / 2});
}

static void place_find_quest(void *form, MinimapLayout *layout)
{
    layout->find_quest = 0;
    Control *panel = native_find(form, find_quest_panel.text);
    if (!panel || panel->loop != layout->loop)
        return;
    Control *button = native_find(form, find_quest_button.text);
    if (!is_native_type(button, build->button_set_size) || button->parent != panel)
        return;

    layout->find_quest = panel;
    // FindQuest is attached to the screen content panel. The radar lives
    // under MainPanel, so compare the two edges in screen coordinates.
    Rect radar = hud_rect(layout->map);
    Rect quest = hud_rect(button);
    int shift = radar.left - 6 - quest.right;
    if (shift)
        set_position(panel, (Point){panel->position.x + shift, panel->position.y});
}

static Control *layout_minimap(void *form, const Wide *key, unsigned slot)
{
    Control *map = native_get(form, key);
    MinimapLayout *layout = &minimaps[slot];
    if (map && layout->map == map && layout->parent == map->parent && layout->loop == map->loop)
        return map;
    *layout = (MinimapLayout){0};
    if (!is_native_type(map, build->graph_set_size) || !map->parent)
        return map;

    int conversation = slot == radar_conversation;
    Control *blind = conversation ? 0 : native_find(form, radar_blind.text);
    if (!is_native_type(blind, build->image_set_size) || blind->parent != map->parent)
        blind = 0;
    Control *border = find_image(map->parent, radar_border_path.text);
    if (!border)
        border = conversation ? map->next : blind ? blind->next : 0;
    if (!is_native_type(border, build->image_set_size) || border->parent != map->parent)
        border = 0;
    Control *center = native_find(form, conversation ? radar_center_player.text : radar_center.text);
    if (!is_native_type(center, build->button_set_size) || center->parent != map->parent)
        center = 0;
    Control *jamming = native_find(form, radar_jamming_name.text);
    if (!is_native_type(jamming, build->image_set_size))
        jamming = 0;
    *layout = (MinimapLayout){map, map->parent, border, blind, center, jamming, map->loop, 0};

    if (!active_radar)
        return map;
    int size = active_radar->map_size;
    int extra = size - 149;
    Point map_pos = map->position;
    Point frame_pos = border ? border->position : (Point){map_pos.x - 78, map_pos.y - 85};
    Point enlarged_center = {map_pos.x - extra / 2, map_pos.y + extra / 2};

    set_radar_square(map, enlarged_center, size);
    if (border) {
        set_radar_image(border, active_radar->frame);
        set_position(border, (Point){frame_pos.x - extra, frame_pos.y});
        set_size(border, (Point){164 + extra, 204 + extra});
    }
    if (blind) {
        set_radar_image(blind, active_radar->blind);
        set_radar_square(blind, enlarged_center, size);
    }
    if (center)
        set_position(center, (Point){center->position.x - conversation, center->position.y + extra});
    if (jamming) {
        const Wide *path = image_path(jamming);
        if (path && path[0] == 'G' && path[1] == 'A' && path[2] == 'I' && path[3] == ',')
            set_radar_image(jamming, active_radar->jamming);
        Point pos = {enlarged_center.x - size / 2, enlarged_center.y - size / 2};
        for (Control *parent = map->parent; parent; parent = parent->parent) {
            pos.x += parent->position.x;
            pos.y += parent->position.y;
        }
        for (Control *parent = jamming->parent; parent; parent = parent->parent) {
            pos.x -= parent->position.x;
            pos.y -= parent->position.y;
        }
        set_position(jamming, pos);
        set_size(jamming, (Point){size, size});
        set_origin(jamming, (Point){0, 0});
    }
    if (slot == radar_flight) {
        Control *partner_icon = native_find(form, radar_partner.text);
        if (partner_icon && partner_icon->parent && partner_icon->parent->parent == map->parent) {
            Control *partners = partner_icon->parent;
            set_position(partners, (Point){partners->position.x, partners->position.y + extra});
        }
        Control *hp = native_find(form, radar_hp_name.text);
        if (hp && hp->parent == map->parent)
            set_position(hp, (Point){hp->position.x - extra, hp->position.y});
    }
    return map;
}

static Control *DELPHI radar_layout_hook(void *form, const Wide *key)
{
    Control *map = layout_minimap(form, key, radar_flight);
    MinimapLayout *layout = &minimaps[radar_flight];
    if (layout->map)
        place_find_quest(form, layout);
    return map;
}

static Control *DELPHI film_radar_layout_hook(void *form, const Wide *key)
{
    return layout_minimap(form, key, radar_film);
}

static Control *DELPHI talk_radar_layout_hook(void *form, const Wide *key)
{
    return layout_minimap(form, key, radar_conversation);
}
