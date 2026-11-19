// The engine keeps GUI positions in its own coordinates. Only the draw clip,
// Direct3D vertices and the corresponding input points cross into screen space.
typedef struct {
    U32 hardware_flag_ref, queue_rect;
    U32 label_draw, custom_loop_vmt;
    U32 intersect, clip_child, clip_queued;
    U32 draw_child, draw_queued;
    U32 contains, cursor, invalidate, image_hit, zone_hit;
    U32 mouse[20], primitive[11];
} UiBuild;

static const UiBuild ui_builds[] = {
    {
        .hardware_flag_ref = 0x8130f0, .queue_rect = 0x4c326c,
        .label_draw = 0x49f71c, .custom_loop_vmt = 0x6046d8,
        .intersect = 0x45ed6c, .clip_child = 0x4c1b51, .clip_queued = 0x4c1bf0,
        .draw_child = 0x4c1b62, .draw_queued = 0x4c1c7e,
        .contains = 0x4c182c, .cursor = 0x4c5ecc, .invalidate = 0x4c1954, .image_hit = 0x4999a4,
        .zone_hit = 0x4b22f0,
        .mouse = {0x4c135a, 0x4c1574, 0x4c15d4, 0x4c1635, 0x4c16ac, 0x4c170c, 0x4c176c,
                  0x4c49a0, 0x4c49e5, 0x4c4b23, 0x4c4b6f, 0x4c4b9d, 0x4c4bf9, 0x4c4d6f,
                  0x4c4ddf, 0x4c4e1c, 0x4c4e99, 0x4c4ed6, 0x4c52f5, 0x4c676a},
        .primitive = {0x4b8202, 0x4b97ca, 0x4b9f36, 0x4caaaa, 0x4caba4, 0x4cb754,
                      0x4cb8c6, 0x4cb8df, 0x4cbc30, 0x4cbe19, 0x4cc08d},
    },
    {
        .hardware_flag_ref = 0x8820f0, .queue_rect = 0x4cc654,
        .label_draw = 0x491b64, .custom_loop_vmt = 0x621ed0,
        .intersect = 0x45ee1c, .clip_child = 0x4cac93, .clip_queued = 0x4cad3c,
        .draw_child = 0x4caca4, .draw_queued = 0x4caddc,
        .contains = 0x4ca84c, .cursor = 0x4cf83c, .invalidate = 0x4caa04, .image_hit = 0x48a87c,
        .zone_hit = 0x4a7bdc,
        .mouse = {0x4ca0fa, 0x4ca424, 0x4ca4ac, 0x4ca534, 0x4ca5e0, 0x4ca668, 0x4ca6f0,
                  0x4cdf45, 0x4cdf90, 0x4ce0db, 0x4ce12d, 0x4ce15b, 0x4ce1ba, 0x4ce342,
                  0x4ce3b7, 0x4ce3f6, 0x4ce47a, 0x4ce4b9, 0x4ce92d, 0x4d034d},
        .primitive = {0x4aeb90, 0x4b049a, 0x4b0c64, 0x7ea4dd, 0x7ea5da, 0x7eb364,
                      0x7eb4f4, 0x7eb511, 0x7eb902, 0x7ebb0c, 0x7ebdbd},
    }
};

static const UiBuild *ui_build;
static Byte (DELPHI *native_intersect)(Rect *, const Rect *, const Rect *);
static void (DELPHI *native_queue_rect)(void *, const Rect *);
static U32 native_invalidate_tail __attribute__((used));
static U32 native_image_hit_tail __attribute__((used));

static int hardware_ui_enabled(void)
{
    if (!ui_build)
        return 0;
    const Byte *hardware = *(const Byte **)game_address(ui_build->hardware_flag_ref);
    return hardware && *hardware;
}

static int general_ui_percent(void)
{
    if (!hardware_ui_enabled())
        return 100;
    if (!cached_ui_percent && *(void **)game_address(build->cfg))
        cached_ui_percent = read_hud_scale();
    return cached_ui_percent > 100 ? cached_ui_percent : 100;
}

static Rect control_bounds(const Control *control)
{
    return *(const Rect *)((const Byte *)control + 0x60);
}

static Control *loop_control(const void *loop, int offset)
{
    return *(Control *const *)((const Byte *)loop + offset);
}

static int native_screen_layout(const void *loop, Point view)
{
    const Wide *name = loop_name(loop);
    if (same_text(name, L"MainForm") ||
        same_text(name, L"GameSettings") ||
        same_text(name, L"Introduction") ||
        *(void *const *)loop == game_address(ui_build->custom_loop_vmt))
        return 1;
    if (!*(void *const *)((const Byte *)loop + 0x08))
        return 0;
    // Native modal loops present and restore only their original viewport.
    const Rect *r = (const Rect *)((const Byte *)loop + 0x48);
    return r->right > r->left && r->bottom > r->top &&
           (r->left > 0 || r->top > 0 || r->right < view.x || r->bottom < view.y);
}

static int has_prefix(const Wide *name, const Wide *prefix)
{
    if (!name)
        return 0;
    while (*prefix && *name == *prefix) {
        ++name;
        ++prefix;
    }
    return !*prefix;
}

#include "government_scale.inc"

static int is_map_layer(Control *control)
{
    // In the flight screen, world-space objects are children of a scrolling
    // map panel. Scrollable inventories also use World for their item rows.
    if (!control->parent || !*((Byte *)control + 0x40))
        return 0;
    const Wide *parent_name = control_name(control->parent);
    return same_text(parent_name, L"MainPanel") &&
           *((Byte *)control->loop + 0x9d) &&
           control->parent->size.x >= screen_width() - 8 &&
           control->parent->size.y >= screen_height() - 8;
}

static int fixed_layer(Control *control, void *loop, const Wide *name)
{
    if (control == loop_control(loop, 0x20) || control == loop_control(loop, 0x28) ||
        is_map_layer(control))
        return 1;
    // Government geometry is independent of the general interface scale.
    const Wide *screen = loop_name(loop);
    if ((same_text(screen, L"Gov") || same_text(screen, L"RuinsTalk")) &&
        (has_prefix(name, L"Gov_Anim") || has_prefix(name, L"GovHD_Anim") ||
         has_prefix(name, L"Panel_Anim") || has_prefix(name, L"PanelHD_Anim") ||
         same_text(name, L"Table") || same_text(name, L"Table2")))
        return 1;
    // Keep the native special layouts in charge of their geometry and artwork.
    if (same_text(name, L"PanelTalk")) {
        if (same_text(screen, L"Gov") || same_text(screen, L"RuinsTalk"))
            return 1;
        const MinimapLayout *talk = &minimaps[radar_conversation];
        if (talk->loop == loop && talk->parent && talk->parent == control->parent)
            return 1;
        // The portrait also identifies the pilot panel before radar setup.
        for (Control *child = control->first; child; child = child->next) {
            if (same_text(control_name(child), pilot_portrait_name.text))
                return 1;
        }
    }
    if (same_text(name, L"SpaceImg") || has_prefix(name, L"StarField") ||
        has_prefix(name, L"GovHD_Anim") || has_prefix(name, L"Gov_Anim") ||
        same_text(name, L"BGBuf") || same_text(name, L"CircleActionShr") ||
        same_text(name, L"CircleActionColor") || has_prefix(name, L"CircleActionWeaponColor"))
        return 1;
    if (same_text(name, L"BGCity") || same_text(name, L"BGCity2") || same_text(name, L"BGShrLight") ||
        same_text(name, L"HangarBackground") || same_text(name, L"MarketBackground") ||
        same_text(name, L"EquipmentBackground")) {
        const Wide *screen = loop_name(loop);
        if (same_text(screen, L"Hangar") || same_text(screen, L"GoodsShop") ||
            same_text(screen, L"EquipmentShop"))
            return 1;
    }
    if (minimaps[radar_flight].loop == loop) {
        Rect edge = control_bounds(control);
        int w = edge.right - edge.left, h = edge.bottom - edge.top;
        // The narrow rail can move past the left edge or into the screen.
        if (edge.left >= -96 && edge.left <= 64 && edge.right <= 128 && w > 0 && w <= 96 &&
            h >= 160 && h >= w * 3 && edge.bottom > 260 && edge.top < screen_height() - 280)
            return 1;
    }
    for (unsigned i = 0; i < COUNT_OF(minimaps); ++i) {
        const MinimapLayout *layout = &minimaps[i];
        if (layout->loop != loop)
            continue;
        if ((layout->parent && control->parent == layout->parent &&
             (control == layout->map || control == layout->blind ||
              control == layout->center || control == layout->border ||
              (i == radar_flight && same_text(name, radar_hp_name.text)))) ||
            control == layout->jamming)
            return 1;
    }
    return 0;
}

static int anchor_axis(int low, int high, int limit, int original_extent, float native_anchor)
{
    if (native_anchor >= 0.0f && native_anchor <= 1.0f)
        return (int)(native_anchor * limit + 0.5f);
    int leading = low;
    int trailing = limit - high;
    int edge = original_extent / 7;
    if (leading <= edge && leading < trailing)
        return 0;
    if (trailing <= edge && trailing < leading)
        return limit;
    return limit / 2;
}

static Point control_pivot(Control *control, Rect bounds, Point view)
{
    const float *anchor = (const float *)((Byte *)control + 0x4c);
    int auto_enabled = *((Byte *)control + 0x48);
    float x = auto_enabled ? anchor[0] : -1.0f;
    float y = auto_enabled ? anchor[1] : -1.0f;
    Control *parent = control->parent;
    if (auto_enabled && parent && same_text(control_name(parent), L"MainPanel")) {
        Rect parent_bounds = control_bounds(parent);
        if (parent_bounds.left == 0 && parent_bounds.top == 0 &&
            parent_bounds.right == view.x && parent_bounds.bottom == view.y) {
            // In flight the native MainPanel uses its center as the local origin.
            const Point *origin = (const Point *)((const Byte *)parent + 0x30);
            if (x >= 0.0f && x <= 0.5f && origin->x != 0 && origin->x == view.x / 2)
                x += 0.5f;
            if (y >= 0.0f && y <= 0.5f && origin->y != 0 && origin->y == view.y / 2)
                y += 0.5f;
        }
    }
    return (Point){
        anchor_axis(bounds.left, bounds.right, view.x, 1024, x),
        anchor_axis(bounds.top, bounds.bottom, view.y, 768, y)
    };
}

static int is_bottom_row(Rect bounds, Point view)
{
    return bounds.top >= view.y - 110 && bounds.bottom > view.y - 82 &&
           bounds.right > bounds.left;
}

static Point bottom_row_pivot(Control *control, Rect bounds, Point view)
{
    Point pivot = control_pivot(control, bounds, view);
    if (!*((Byte *)control + 0x48))
        pivot.x = bounds.left + bounds.right < view.x ? 0 : view.x;
    pivot.y = view.y;
    return pivot;
}

static int left_hud_corner(Rect bounds, Point view)
{
    if (bounds.left < -64 || bounds.left > 96 || bounds.right > 220 ||
        bounds.right <= bounds.left || bounds.bottom <= bounds.top ||
        bounds.bottom - bounds.top > 260)
        return 0;
    if (bounds.top >= view.y - 280 && bounds.bottom <= view.y + 32)
        return -1;
    if (bounds.top >= -32 && bounds.bottom <= 260)
        return 1;
    return 0;
}

static UiTransform bottom_bar_transform(Control *branch, Control *child, Point view, int percent)
{
    if (!branch || percent <= 100)
        return ui_identity;
    Rect bounds = control_bounds(branch);
    const Wide *name = control_name(branch);
    if (branch->size.x == view.x) {
        for (Control *p = branch->first; p; p = p->next) {
            if (!same_text(control_name(p), nav_background.text))
                continue;
            if (!child || is_native_hud_art(control_name(child)) ||
                branch->size.y != fhd_scale_px(82, percent))
                return ui_identity;
            return nav_child_transform(bounds, control_bounds(child), percent);
        }
    }
    if (same_text(name, L"PM_PanelMsg")) {
        Point pivot = {0, view.y};
        return ui_place(percent, pivot, pivot);
    }
    if (same_text(name, L"PM_WinMsg"))
        return ui_group_transform(percent, bounds, view, (Point){0, view.y});
    if (left_hud_corner(bounds, view) < 0) {
        Point pivot = {0, view.y};
        return ui_place(percent, pivot, pivot);
    }
    Point pivot = is_bottom_row(bounds, view) ? bottom_row_pivot(branch, bounds, view) :
                  control_pivot(branch, bounds, view);
    return ui_group_transform(percent, bounds, view, pivot);
}

static void *item_tip_loop;
static UiTransform item_tip_transform;

static UiTransform station_bottom_transform(Control *panel, Point view, int percent)
{
    if (percent <= 100)
        return ui_identity;
    // PanelRuins is a full 1024x768 native container; the actual toolbar
    // is only the 297x43 stock artwork inside it. Fit that artwork, not
    // the screen-sized parent, and share one transform across all descendants.
    Control *background = 0;
    for (Control *child = panel->first; child; child = child->next) {
        if (is_native_type(child, build->image_set_size) &&
            same_text(image_path(child), planet_image_path.text)) {
            background = child;
            break;
        }
    }
    if (!background)
        return ui_identity;
    Rect group = control_bounds(background);
    if (group.right - group.left < 220 || group.right - group.left > 520 ||
        group.bottom - group.top < 30 || group.bottom - group.top > 90)
        return ui_identity;
    Point place = {group.left, group.top};
    Point target = {view.x - fhd_scale_px(view.x - place.x, percent),
                    view.y - fhd_scale_px(view.y - place.y, percent)};
    UiTransform result = ui_place(percent, place, target);
    Control *station = 0;
    for (Control *child = panel->first; child; child = child->next) {
        if (same_text(control_name(child), ruins_gov.text)) {
            station = child;
            break;
        }
    }
    Control *ship = native_find(panel->loop, nav_ship.text);
    int deferred = ship && ship->parent && ship->parent->size.y == fhd_scale_px(82, percent);
    Point delta = dock_row_offset(panel->loop, panel, station, ship,
                                 percent, deferred, group, result);
    result.x100 += delta.x * 100;
    result.y100 += delta.y * 100;
    return result;
}

static int ship_item_screen(const void *loop)
{
    const Wide *name = loop_name(loop);
    return same_text(name, L"Ship") || same_text(name, L"Scaner");
}

static UiTransform item_window_transform(Control *window, Point view)
{
    Rect bounds = control_bounds(window);
    int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
    if (width <= 0 || height <= 0)
        return ui_identity;
    UiTransform source = item_tip_transform;
    int scale = ui_min(source.percent, ui_min(view.x * 100 / width, view.y * 100 / height));
    Point corner = ui_screen_point(source, (Point){bounds.left, bounds.top});
    UiTransform result = scale == source.percent ? source :
        ui_place(scale, (Point){bounds.left, bounds.top}, corner);
    // Native tips are positioned against the slot's unscaled bounds. Keep
    // that attachment, then bring the entire card back inside the viewport.
    result.x100 = ui_max(-bounds.left * scale,
                        ui_min(view.x * 100 - bounds.right * scale, result.x100));
    result.y100 = ui_max(-bounds.top * scale,
                        ui_min(view.y * 100 - bounds.bottom * scale, result.y100));
    return result;
}

static UiTransform control_transform(Control *control)
{
    int percent = general_ui_percent();
    if (!control || !control->loop)
        return ui_identity;
    void *loop = control->loop;
    int government = government_scaling_active(loop);
    if (percent == 100 && !government)
        return ui_identity;
    Point view = {screen_width(), screen_height()};
    if (view.x < 800 || view.y < 600 || view.x > 16384 || view.y > 16384)
        return ui_identity;
    if (native_screen_layout(loop, view))
        return ui_identity;
    percent = screen_ui_percent(loop, percent);
    Control *group = 0, *branch = 0, *child = 0;
    for (Control *p = control; p && p->loop == loop; p = p->parent) {
        const Wide *name = control_name(p);
        UiTransform portrait;
        if (government && government_animation_transform(p, name, view, &portrait))
            return portrait;
        if (loop == item_tip_loop && ship_item_screen(loop) &&
            (same_text(name, L"PII") || same_text(name, L"InfoHull")) &&
            (p->position.x != 10 || p->position.y != 10))
            return item_window_transform(p, view);
        if (fixed_layer(p, loop, name))
            return ui_identity;
        if (same_text(name, L"PanelMain"))
            return bottom_bar_transform(branch, child, view, fhd_hud_percent(view.x, percent));
        if (same_text(name, L"PanelSpace") && p->parent &&
            same_text(control_name(p->parent), L"MainPanel")) {
            Point pivot = view;
            return ui_place(fhd_hud_percent(view.x, percent), pivot, pivot);
        }
        if (same_text(name, L"PanelPlanet") && p->size.x >= 220 && p->size.x <= 520 &&
            p->size.y >= 30 && p->size.y <= 90) {
            Rect bounds = control_bounds(p);
            Point pivot = {bounds.left, bounds.top};
            return ui_place(fhd_hud_percent(view.x, percent), pivot, pivot);
        }
        if (same_text(name, ruins_panel_name.text))
            return station_bottom_transform(p, view, fhd_hud_percent(view.x, percent));
        child = branch;
        branch = p;
        if (p == loop_control(loop, 0x18) || p == loop_control(loop, 0x1c) ||
            p == loop_control(loop, 0x24))
            continue;
        Rect r = control_bounds(p);
        if (r.left < -32768 || r.top < -32768 || r.right > 32768 || r.bottom > 32768)
            return ui_identity;
        if (r.right > r.left && r.bottom > r.top &&
            r.right - r.left < view.x - 8 && r.bottom - r.top < view.y - 8)
            group = p;
    }
    if (!group)
        return ui_identity;
    Rect bounds = control_bounds(group);
    Point pivot = control_pivot(group, bounds, view);
    const MinimapLayout *flight = &minimaps[radar_flight];
    if (flight->loop == loop && group != flight->find_quest) {
        int corner = left_hud_corner(bounds, view);
        if (corner) {
            // All pieces of the edge controls share a pivot, including art
            // with origins a few pixels outside the screen.
            Point edge = {0, corner < 0 ? view.y : 0};
            return ui_place(fhd_hud_percent(view.x, percent), edge, edge);
        }
    }
    if (flight->loop == loop && group == flight->find_quest)
        pivot = (Point){bounds.right, bounds.top};
    else if (flight->loop == loop && is_bottom_row(bounds, view)) {
        pivot = bottom_row_pivot(group, bounds, view);
        percent = fhd_hud_percent(view.x, percent);
    }
    if (same_text(control_name(group), L"PanelTalk"))
        view.y = dialogue_screen_height();
    else if (percent == 200 && same_text(loop_name(loop), L"Info") &&
             find_image(group, info_frame_path.text)) {
        view.y = dialogue_screen_height();
        int fit = view.y * 100 / (bounds.bottom - bounds.top);
        if (fit <= 100 && bounds.bottom > view.y) {
            // Some ultrawide modes leave less room than the original frame needs.
            fit = ui_min(fit, 99);
            return (UiTransform){fit,
                ui_axis_offset(fit, bounds.left, bounds.right, view.x, pivot.x),
                ui_axis_offset(fit, bounds.top, bounds.bottom, view.y, pivot.y)};
        }
    }
    return ui_group_transform(percent, bounds, view, pivot);
}

static Rect physical_bounds(Control *control)
{
    return ui_screen_rect(control_transform(control), control_bounds(control));
}

typedef struct {
    UiTransform transform;
    Rect clip;
    U32 frame;
    int pixel_text;
    int portrait;
} DrawContext;

enum { sampler_mag_filter = 5, sampler_min_filter = 6,
       texture_point_filter = 1 };

static DrawContext draw_context;
static UiTransform input_transform = {100, 0, 0};
static void *input_loop;
static Point input_screen, input_logical;
static U32 input_frame;

static int within_native_call(U32 frame)
{
    U32 stack;
    __asm__ volatile("movl %%esp, %0" : "=r"(stack));
    return frame && stack < frame;
}

static Byte DELPHI clip_scaled_child(Rect *out, const Rect *clip, const Rect *bounds)
{
    Control *child = (Control *)((Byte *)bounds - 0x60);
    if (hidden_government_table(child))
        return 0;
    if (general_ui_percent() == 100 && !government_scaling_active(child->loop))
        return native_intersect(out, clip, bounds);
    Rect screen_clip = *clip;
    if (within_native_call(draw_context.frame)) {
        screen_clip = ui_screen_rect(draw_context.transform, screen_clip);
        if (!ui_intersection(&screen_clip, screen_clip, draw_context.clip))
            return 0;
    }
    Rect screen_bounds = physical_bounds(child);
    return native_intersect(out, &screen_clip, &screen_bounds);
}

static Byte DELPHI clip_scaled_queued(Rect *out, const Rect *clip, const Rect *bounds)
{
    Control *child = (Control *)((Byte *)bounds - 0x60);
    if (hidden_government_table(child))
        return 0;
    if (general_ui_percent() == 100 && !government_scaling_active(child->loop))
        return native_intersect(out, clip, bounds);
    Rect screen_bounds = physical_bounds(child);
    return native_intersect(out, clip, &screen_bounds);
}

static void DELPHI draw_scaled_control(Control *control, const Rect *screen_clip)
{
    if (hidden_government_table(control))
        return;
    void (DELPHI *draw)(Control *, const Rect *) = control->methods[0xa0 / 4];
    if (general_ui_percent() == 100 && !government_scaling_active(control->loop)) {
        draw(control, screen_clip);
        return;
    }
    UiTransform t = control_transform(control);
    DrawContext saved = draw_context;
    U32 frame;
    __asm__ volatile("movl %%esp, %0" : "=r"(frame));
    int pixel_text = t.percent > 100 && control->methods[0xa0 / 4] == game_address(ui_build->label_draw);
    draw_context = (DrawContext){t, *screen_clip, frame, pixel_text, government_portrait(control)};
    Rect logical = ui_logical_clip(t, *screen_clip);
    draw(control, &logical);
    draw_context = saved;
}

static Point screen_input_point(const void *loop, Point point)
{
    if (input_loop != loop || !within_native_call(input_frame))
        return point;
    if (point.x == input_logical.x && point.y == input_logical.y)
        return input_screen;
    return ui_screen_point(input_transform, point);
}

static int visible_item_hit(Control *control, Point screen)
{
    Rect visible = physical_bounds(control);
    if (*((const Byte *)control + 0x42) == 1)
        return 0;
    for (Control *p = control; p && p->loop == control->loop; p = p->parent) {
        if (!*((const Byte *)p + 0x41) ||
            !ui_intersection(&visible, visible, physical_bounds(p)))
            return 0;
    }
    return screen.x >= visible.left && screen.x < visible.right &&
           screen.y >= visible.top && screen.y < visible.bottom;
}

BSTR(item_tip_text, L"InfoText");
BSTR(item_tip_title, L"InfoName");
BSTR(item_hull_tip_text, L"InfoHullText");
BSTR(item_hull_tip_title, L"InfoHullName");
BSTR(item_tip_font, L"Font.Verdana9");
BSTR(item_tip_bold_font, L"Font.Verdana9bold");

static void prepare_item_tip_fonts(void *loop)
{
    if (general_ui_percent() <= 100 || !native_find || !native_label_font)
        return;
    const Wide *names[] = {item_tip_text.text, item_tip_title.text,
                          item_hull_tip_text.text, item_hull_tip_title.text};
    for (unsigned i = 0; i < COUNT_OF(names); ++i) {
        Control *label = native_find(loop, names[i]);
        if (!label || label->methods[0xa0 / 4] != game_address(ui_build->label_draw) ||
            !label->parent || (!same_text(control_name(label->parent), L"PII") &&
                               !same_text(control_name(label->parent), L"InfoHull")))
            continue;
        const Byte *cache = *(const Byte **)((const Byte *)label + 0x120);
        const Wide *key = cache ? *(const Wide *const *)(cache + 0x10) : 0;
        int normal = same_text(key, L"Font.2Small") || same_text(key, L"Font.1Small") ||
                     same_text(key, L"Font.Verdana8") || same_text(key, style_v8.text) ||
                     same_text(key, style_c10.text);
        int bold = same_text(key, L"Font.2SmallBold") || same_text(key, L"Font.1SmallBold") ||
                   same_text(key, L"Font.Verdana8bold") || same_text(key, style_v8b.text);
        if (normal || bold)
            native_label_font(label, bold ? item_tip_bold_font.text : item_tip_font.text);
    }
}

static Byte DELPHI contains_scaled_point(Control *control, const Point *point)
{
    if (hidden_government_table(control))
        return 0;
    if (!*((Byte *)control + 0x41) || *((Byte *)control + 0x42) == 1)
        return 0;
    Point screen = screen_input_point(control->loop, *point);
    Rect bounds = physical_bounds(control);
    Byte hit = screen.x >= bounds.left && screen.x < bounds.right &&
               screen.y >= bounds.top && screen.y < bounds.bottom;
    const Wide *name = control_name(control);
    if (hit && ship_item_screen(control->loop) &&
        (has_prefix(name, L"RHItem") || has_prefix(name, L"Storage_"))) {
        if (!visible_item_hit(control, screen))
            return 0;
        item_tip_loop = control->loop;
        item_tip_transform = control_transform(control);
        prepare_item_tip_fonts(control->loop);
    }
    return hit;
}

static Byte DELPHI zone_hit_scaled(Control *control, const Point *point)
{
    if (hidden_government_table(control))
        return 0;
    int ship = ship_item_screen(control->loop);
    if (ship && !visible_item_hit(control, screen_input_point(control->loop, *point)))
        return 0;
    Byte kind = *((const Byte *)control + 0x120);
    Byte hit = 0;
    if (!kind)
        hit = contains_scaled_point(control, point);
    else if (kind == 1) {
        // Ship slots also obey panel visibility. Other circular controls keep
        // their native activation semantics.
        UiTransform t = control_transform(control);
        Point screen = screen_input_point(control->loop, *point);
        Rect r = control_bounds(control);
        int diameter = ui_min(r.right - r.left, r.bottom - r.top);
        double x = (double)screen.x * 100 - t.x100 - ((r.left + r.right) / 2) * t.percent;
        double y = (double)screen.y * 100 - t.y100 - ((r.top + r.bottom) / 2) * t.percent;
        double radius = (double)diameter * t.percent / 2;
        hit = x * x + y * y <= radius * radius;
    }
    if (hit && ship) {
        item_tip_loop = control->loop;
        item_tip_transform = control_transform(control);
        // ShowItemInfo performs the native card measurement after this hit.
        prepare_item_tip_fonts(control->loop);
    }
    return hit;
}

static void DELPHI get_scaled_cursor(void *loop, Point *point)
{
    *point = loop_control(loop, 0x28)->position;
    if (input_loop == loop && within_native_call(input_frame))
        *point = ui_logical_point(input_transform, *point);
}

typedef Byte (DELPHI *ImageHit)(Control *, const Point *);
static ImageHit image_hit_original;

static Byte DELPHI image_hit_original_new(Control *, const Point *) __attribute__((naked, noinline));
static Byte DELPHI image_hit_original_new(Control *c __attribute__((unused)), const Point *p __attribute__((unused)))
{
    __asm__("pushl %esi\n" "pushl %edi\n" "subl $8, %esp\n"
            "jmp *native_image_hit_tail");
}

static Byte DELPHI image_hit_original_old(Control *, const Point *) __attribute__((naked, noinline));
static Byte DELPHI image_hit_original_old(Control *c __attribute__((unused)), const Point *p __attribute__((unused)))
{
    __asm__("pushl %ebp\n" "movl %esp, %ebp\n" "subl $16, %esp\n"
            "jmp *native_image_hit_tail");
}

static Byte DELPHI image_hit_scaled(Control *control, const Point *point)
{
    if (hidden_government_table(control))
        return 0;
    UiTransform t = control_transform(control);
    Point logical = ui_logical_point(t, screen_input_point(control->loop, *point));
    return image_hit_original(control, &logical);
}

static void dispatch_scaled_mouse(Control *control, U32 keys, const Point *point, int slot)
{
    if (hidden_government_table(control))
        return;
    void (DELPHI *dispatch)(Control *, U32, const Point *) = control->methods[slot / 4];
    if (general_ui_percent() == 100 && !government_scaling_active(control->loop)) {
        dispatch(control, keys, point);
        return;
    }

    UiTransform transform = control_transform(control);
    Point screen = screen_input_point(control->loop, *point);
    if (input_loop == control->loop && within_native_call(input_frame) &&
        ui_same_transform(transform, input_transform)) {
        dispatch(control, keys, point);
        return;
    }
    void *saved_loop = input_loop;
    U32 saved_frame = input_frame;
    UiTransform saved_transform = input_transform;
    Point saved_screen = input_screen, saved_logical = input_logical;
    U32 frame;
    __asm__ volatile("movl %%esp, %0" : "=r"(frame));
    input_transform = transform;
    input_loop = control->loop;
    input_frame = frame;
    input_screen = screen;
    input_logical = ui_logical_point(transform, screen);
    Point logical = input_logical;
    dispatch(control, keys, &logical);
    input_loop = saved_loop;
    input_frame = saved_frame;
    input_transform = saved_transform;
    input_screen = saved_screen;
    input_logical = saved_logical;
}

#define UI_EVENT(name, slot) \
    static CALLED_FROM_ASM void DELPHI event_##name(Control *c, U32 keys, const Point *p) \
    { dispatch_scaled_mouse(c, keys, p, slot); }
UI_EVENT(move, 0x38)
UI_EVENT(down, 0x54)
UI_EVENT(up, 0x58)
UI_EVENT(right_down, 0x5c)
UI_EVENT(right_up, 0x60)
UI_EVENT(double_left, 0x64)
UI_EVENT(double_right, 0x68)

// Replay the original VMT load into the same callee-saved register.
#define UI_MOUSE_SHIM(name, reg) \
    static ASSEMBLY void mouse_##name##_##reg(void) \
    { __asm__("movl (%eax), %" #reg "\n" "jmp event_" #name); }
UI_MOUSE_SHIM(move, ebp)
UI_MOUSE_SHIM(move, ebx)
UI_MOUSE_SHIM(down, edi)
UI_MOUSE_SHIM(down, ebx)
UI_MOUSE_SHIM(down, esi)
UI_MOUSE_SHIM(up, edi)
UI_MOUSE_SHIM(up, ebx)
UI_MOUSE_SHIM(right_down, ebp)
UI_MOUSE_SHIM(right_down, ebx)
UI_MOUSE_SHIM(right_down, esi)
UI_MOUSE_SHIM(right_up, edi)
UI_MOUSE_SHIM(right_up, ebx)
UI_MOUSE_SHIM(double_left, edi)
UI_MOUSE_SHIM(double_left, ebx)
UI_MOUSE_SHIM(double_left, esi)
UI_MOUSE_SHIM(double_right, edi)
UI_MOUSE_SHIM(double_right, ebx)
UI_MOUSE_SHIM(double_right, esi)

typedef void (DELPHI *InvalidateRect)(Control *, const Rect *);
static void DELPHI invalidate_original_new(Control *, const Rect *) __attribute__((naked, noinline));
static void DELPHI invalidate_original_new(Control *c __attribute__((unused)), const Rect *r __attribute__((unused)))
{
    __asm__("pushl %ebx\n" "pushl %esi\n" "pushl %edi\n" "subl $80, %esp\n"
            "jmp *native_invalidate_tail");
}

static void DELPHI invalidate_original_old(Control *, const Rect *) __attribute__((naked, noinline));
static void DELPHI invalidate_original_old(Control *c __attribute__((unused)), const Rect *r __attribute__((unused)))
{
    __asm__("pushl %ebp\n" "movl %esp, %ebp\n" "subl $84, %esp\n"
            "jmp *native_invalidate_tail");
}

static InvalidateRect invalidate_original;

static void DELPHI invalidate_scaled_rect(Control *control, const Rect *rect)
{
    // A scale change can leave old enlarged pixels outside the next animated
    // frame's damage rectangle. Queue a full repaint before that frame draws.
    if (government_redraw_pending && government_screen(control->loop) &&
        native_queue_rect && *((Byte *)control->loop + 0x58)) {
        Rect viewport = {0, 0, screen_width(), screen_height()};
        native_queue_rect(control->loop, &viewport);
        government_redraw_pending = 0;
    }
    UiTransform t = control_transform(control);
    if (ui_is_identity(t) || !*((Byte *)control + 0x41) || !control->parent) {
        invalidate_original(control, rect);
        return;
    }
    Point absolute = *(Point *)((Byte *)control + 0x70);
    Point offset = {absolute.x - control->position.x, absolute.y - control->position.y};
    Rect absolute_rect = {rect->left + offset.x, rect->top + offset.y,
                          rect->right + offset.x, rect->bottom + offset.y};
    Rect dirty = ui_screen_rect(t, absolute_rect);
    for (Control *p = control; p; p = p->parent) {
        if (!*((Byte *)p + 0x41) || !ui_intersection(&dirty, dirty, physical_bounds(p)))
            return;
    }
    native_queue_rect(control->loop, &dirty);
}

typedef struct {
    float x, y, z, rhw;
    U32 color;
    float u, v;
} UiVertex;

typedef int (WINAPI *DrawPrimitiveUP)(void *, U32, U32, const void *, U32);
typedef int (WINAPI *ScissorMethod)(void *, Rect *);
typedef int (WINAPI *GetSamplerState)(void *, U32, U32, U32 *);
typedef int (WINAPI *SetSamplerState)(void *, U32, U32, U32);

static float pixel_edge(float coordinate)
{
    // A D3D9 screen vertex is half a pixel before its texel boundary.
    int pixel = (int)coordinate;
    return pixel + (coordinate > pixel) - 0.5f;
}

#include "text_profiles.inc"

static int draw_portrait_vertices(void *device, U32 type, U32 count, UiVertex *data, U32 stride)
{
    enum { address_u = 1, address_v = 2, texture_clamp = 3 };
    void **methods = *(void ***)device;
    DrawPrimitiveUP draw = methods[83];
    GetSamplerState get = methods[68];
    SetSamplerState set = methods[69];
    U32 u, v;
    // Enlarged linear-filtered textures must not wrap their opaque bottom
    // row onto the transparent top edge. Keep filtering itself unchanged.
    if (!get || !set || get(device, 0, address_u, &u) < 0 ||
        get(device, 0, address_v, &v) < 0 ||
        set(device, 0, address_u, texture_clamp) < 0)
        return draw(device, type, count, data, stride);
    if (set(device, 0, address_v, texture_clamp) < 0) {
        set(device, 0, address_u, u);
        return draw(device, type, count, data, stride);
    }
    int result = draw(device, type, count, data, stride);
    set(device, 0, address_v, v);
    set(device, 0, address_u, u);
    return result;
}

static int draw_ui_vertices(void *device, U32 type, U32 count, UiVertex *data, U32 stride)
{
    if (draw_context.portrait)
        return draw_portrait_vertices(device, type, count, data, stride);
    void **methods = *(void ***)device;
    DrawPrimitiveUP draw = methods[83];
    GetSamplerState get = methods[68];
    SetSamplerState set = methods[69];
    U32 mag, min;
    if (!draw_context.pixel_text || !get || !set ||
        get(device, 0, sampler_mag_filter, &mag) < 0 ||
        get(device, 0, sampler_min_filter, &min) < 0 ||
        set(device, 0, sampler_mag_filter, texture_point_filter) < 0)
        return draw(device, type, count, data, stride);
    if (set(device, 0, sampler_min_filter, texture_point_filter) < 0) {
        set(device, 0, sampler_mag_filter, mag);
        return draw(device, type, count, data, stride);
    }
    int sharpened = begin_text_profile(device);
    int result = draw(device, type, count, data, stride);
    if (sharpened)
        ((ShaderState)methods[107])(device, 0);
    set(device, 0, sampler_min_filter, min);
    set(device, 0, sampler_mag_filter, mag);
    return result;
}

static unsigned primitive_vertices(U32 type, U32 count)
{
    switch (type) {
    case 1: return count;
    case 2: return count * 2;
    case 3: return count + 1;
    case 4: return count * 3;
    case 5:
    case 6: return count + 2;
    default: return 0;
    }
}

static int WINAPI draw_scaled_vertices(void *device, U32 type, U32 count, const void *data, U32 stride)
{
    void **methods = *(void ***)device;
    DrawPrimitiveUP draw = methods[83];
    if (!within_native_call(draw_context.frame) || ui_is_identity(draw_context.transform) ||
        !data || stride != sizeof(UiVertex) || count > 4096)
        return draw(device, type, count, data, stride);

    unsigned vertices = primitive_vertices(type, count);
    if (!vertices)
        return draw(device, type, count, data, stride);
    // Direct3D reads the vertex list during the call. The larger buffer covers
    // batched point lists without rewriting the game's original vertices.
    UiVertex small[16];
    static UiVertex large[12288];
    UiVertex *output = vertices <= COUNT_OF(small) ? small : large;
    const UiVertex *source = data;
    UiTransform transform = draw_context.transform;
    float scale = transform.percent / 100.0f;
    float dx = transform.x100 / 100.0f;
    float dy = transform.y100 / 100.0f;
    for (unsigned i = 0; i < vertices; ++i) {
        output[i] = source[i];
        output[i].x = (source[i].x + 0.5f) * scale + dx - 0.5f;
        output[i].y = (source[i].y + 0.5f) * scale + dy - 0.5f;
        if (draw_context.pixel_text) {
            output[i].x = pixel_edge(output[i].x);
            output[i].y = pixel_edge(output[i].y);
        }
    }

    ScissorMethod get_scissor = methods[76], set_scissor = methods[75];
    Rect prior;
    if (get_scissor(device, &prior) < 0)
        return draw_ui_vertices(device, type, count, output, stride);
    Rect viewport = {0, 0, screen_width(), screen_height()};
    Rect clip = draw_context.clip;
    if (!fhd_equal_rect(prior, viewport) &&
        !ui_intersection(&clip, clip, ui_screen_rect(transform, prior)))
        return 0;
    if (!ui_intersection(&clip, clip, viewport))
        return 0;
    if (set_scissor(device, &clip) < 0)
        return draw_ui_vertices(device, type, count, output, stride);
    int result = draw_ui_vertices(device, type, count, output, stride);
    set_scissor(device, &prior);
    return result;
}

typedef struct {
    Byte mov, slot;
    void (*hook)(void);
} MouseSite;

#define MOUSE_SITE(name, reg, mov, slot) {(mov), (slot), mouse_##name##_##reg}
static const MouseSite mouse_sites[2][20] = {
    {
        MOUSE_SITE(move, ebp, 0x28, 0x38), MOUSE_SITE(down, edi, 0x38, 0x54),
        MOUSE_SITE(up, edi, 0x38, 0x58), MOUSE_SITE(right_down, ebp, 0x28, 0x5c),
        MOUSE_SITE(right_up, edi, 0x38, 0x60), MOUSE_SITE(double_left, edi, 0x38, 0x64),
        MOUSE_SITE(double_right, edi, 0x38, 0x68),
        MOUSE_SITE(move, ebx, 0x18, 0x38), MOUSE_SITE(move, ebx, 0x18, 0x38),
        MOUSE_SITE(down, ebx, 0x18, 0x54), MOUSE_SITE(up, ebx, 0x18, 0x58),
        MOUSE_SITE(up, ebx, 0x18, 0x58), MOUSE_SITE(right_down, ebx, 0x18, 0x5c),
        MOUSE_SITE(right_up, ebx, 0x18, 0x60), MOUSE_SITE(down, esi, 0x30, 0x54),
        MOUSE_SITE(double_left, esi, 0x30, 0x64), MOUSE_SITE(right_down, esi, 0x30, 0x5c),
        MOUSE_SITE(double_right, esi, 0x30, 0x68),
        MOUSE_SITE(move, ebx, 0x18, 0x38), MOUSE_SITE(move, ebx, 0x18, 0x38),
    },
    {
        MOUSE_SITE(move, ebx, 0x18, 0x38), MOUSE_SITE(down, ebx, 0x18, 0x54),
        MOUSE_SITE(up, ebx, 0x18, 0x58), MOUSE_SITE(right_down, ebx, 0x18, 0x5c),
        MOUSE_SITE(right_up, ebx, 0x18, 0x60), MOUSE_SITE(double_left, ebx, 0x18, 0x64),
        MOUSE_SITE(double_right, ebx, 0x18, 0x68),
        MOUSE_SITE(move, ebx, 0x18, 0x38), MOUSE_SITE(move, ebx, 0x18, 0x38),
        MOUSE_SITE(down, ebx, 0x18, 0x54), MOUSE_SITE(up, ebx, 0x18, 0x58),
        MOUSE_SITE(up, ebx, 0x18, 0x58), MOUSE_SITE(right_down, ebx, 0x18, 0x5c),
        MOUSE_SITE(right_up, ebx, 0x18, 0x60), MOUSE_SITE(down, ebx, 0x18, 0x54),
        MOUSE_SITE(double_left, ebx, 0x18, 0x64), MOUSE_SITE(right_down, ebx, 0x18, 0x5c),
        MOUSE_SITE(double_right, ebx, 0x18, 0x68),
        MOUSE_SITE(move, ebx, 0x18, 0x38), MOUSE_SITE(move, ebx, 0x18, 0x38),
    },
};
#undef MOUSE_SITE

static int install_global_ui(void)
{
    int old = build->timestamp == 0x68eccc46;
    const UiBuild *locations = &ui_builds[old];
    Patch patches[40] = {
        call_patch(locations->clip_child, locations->intersect, (void (*)(void))clip_scaled_child),
        call_patch(locations->clip_queued, locations->intersect, (void (*)(void))clip_scaled_queued),
        {game_address(locations->draw_child), (void (*)(void))draw_scaled_control, {0xff, 0x91, 0xa0, 0, 0, 0}, 6, 0},
        {game_address(locations->draw_queued), (void (*)(void))draw_scaled_control, {0xff, 0x91, 0xa0, 0, 0, 0}, 6, 0},
        {game_address(locations->contains), (void (*)(void))contains_scaled_point, {0x56, 0x57, 0x83, 0xc4, 0xf8}, 5, 1},
        {game_address(locations->cursor), (void (*)(void))get_scaled_cursor, {0x8b, 0x40, 0x28, 0x8b, 0x48, 0x20}, 6, 1},
        {game_address(locations->invalidate), (void (*)(void))invalidate_scaled_rect, {0x53, 0x56, 0x57, 0x83, 0xc4, 0xb0}, 6, 1},
        {game_address(locations->image_hit), (void (*)(void))image_hit_scaled, {0x56, 0x57, 0x83, 0xc4, 0xf8}, 5, 1},
    };
    if (old) {
        const Byte contains[] = {0x55, 0x8b, 0xec, 0x83, 0xc4, 0xf0};
        const Byte cursor[] = {0x55, 0x8b, 0xec, 0x83, 0xc4, 0xf8};
        const Byte invalidate[] = {0x55, 0x8b, 0xec, 0x83, 0xc4, 0xac};
        copy_code(patches[4].expected, contains, 6);
        patches[4].length = 6;
        copy_code(patches[5].expected, cursor, 6);
        copy_code(patches[6].expected, invalidate, 6);
        copy_code(patches[7].expected, contains, 6);
        patches[7].length = 6;
    }
    int index = 8;
    for (unsigned i = 0; i < COUNT_OF(locations->mouse); ++i) {
        MouseSite site = mouse_sites[old][i];
        patches[index++] = (Patch){game_address(locations->mouse[i]), site.hook,
            {0x8b, site.mov, 0xff, (Byte)(0x50 | ((site.mov >> 3) & 7)), site.slot}, 5, 0};
    }
    for (unsigned i = 0; i < COUNT_OF(locations->primitive); ++i) {
        patches[index++] = (Patch){game_address(locations->primitive[i]), (void (*)(void))draw_scaled_vertices,
            {0xff, 0x90, 0x4c, 0x01, 0x00, 0x00}, 6, 0};
    }
    patches[index++] = (Patch){game_address(locations->zone_hit), (void (*)(void))zone_hit_scaled,
        {0x53, 0x56, 0x57, 0x55, 0x83}, 5, 1};
    if (old) {
        const Byte prefix[] = {0x55, 0x8b, 0xec, 0x83, 0xc4, 0xd4};
        copy_code(patches[index - 1].expected, prefix, 6);
        patches[index - 1].length = 6;
    }
    native_intersect = game_address(locations->intersect);
    native_queue_rect = game_address(locations->queue_rect);
    native_invalidate_tail = (U32)game_address(locations->invalidate + 6);
    invalidate_original = old ? invalidate_original_old : invalidate_original_new;
    native_image_hit_tail = (U32)game_address(locations->image_hit + patches[7].length);
    image_hit_original = old ? image_hit_original_old : image_hit_original_new;
    if (!apply_patches(patches, index))
        return 0;
    ui_build = locations;
    return 1;
}
