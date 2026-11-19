// Portraits have their own setting. Their native controls remain untouched;
// both animation phases share the same affine transform and screen limit.
static int government_screen(const void *loop)
{
    const Wide *name = loop ? loop_name(loop) : 0;
    return same_text(name, L"Gov") || same_text(name, L"RuinsTalk");
}

static int government_scaling_active(const void *loop)
{
    return government_screen(loop) && hardware_ui_enabled();
}

static int government_percent(void)
{
    if (!cached_government_percent && *(void **)game_address(build->cfg))
        cached_government_percent = read_government_scale();
    return cached_government_percent ? cached_government_percent : 100;
}

static int hidden_government_table(Control *control)
{
    if (!control || !government_screen(control->loop))
        return 0;
    for (Control *p = control; p && p->loop == control->loop; p = p->parent) {
        const Wide *name = control_name(p);
        if (same_text(name, L"Table") || same_text(name, L"Table2"))
            return 1;
    }
    return 0;
}

static int government_animation_pair(const Wide *name, const Wide **idle, const Wide **talk)
{
    if (same_text(name, L"Gov_Anim0") || same_text(name, L"Gov_Anim1")) {
        *idle = L"Gov_Anim0"; *talk = L"Gov_Anim1";
    } else if (same_text(name, L"GovHD_Anim0") || same_text(name, L"GovHD_Anim1")) {
        *idle = L"GovHD_Anim0"; *talk = L"GovHD_Anim1";
    } else if (same_text(name, L"Panel_Anim0") || same_text(name, L"Panel_Anim1")) {
        *idle = L"Panel_Anim0"; *talk = L"Panel_Anim1";
    } else if (same_text(name, L"PanelHD_Anim0") || same_text(name, L"PanelHD_Anim1")) {
        *idle = L"PanelHD_Anim0"; *talk = L"PanelHD_Anim1";
    } else {
        return 0;
    }
    return 1;
}

static int government_portrait(Control *control)
{
    if (!control || !government_scaling_active(control->loop))
        return 0;
    for (Control *p = control; p && p->loop == control->loop; p = p->parent) {
        const Wide *idle, *talk;
        if (government_animation_pair(control_name(p), &idle, &talk))
            return 1;
    }
    return 0;
}

static Rect government_native_bounds(const Control *piece)
{
    // The engine updates AbsolutePosition/HitTestBounds only for active
    // children. An inactive phase may still contain last planet's bounds.
    // Government animation parents are plain panels; use their current
    // absolute origin and both phases' live local positions instead.
    Point parent = piece->parent ? *(const Point *)((const Byte *)piece->parent + 0x70) : (Point){0, 0};
    Point origin = *(const Point *)((const Byte *)piece + 0x30);
    int x = parent.x + piece->position.x - origin.x;
    int y = parent.y + piece->position.y - origin.y;
    return (Rect){x, y, x + piece->size.x, y + piece->size.y};
}

static int government_animation_transform(Control *piece, const Wide *name,
                                          Point view, UiTransform *out)
{
    const Wide *idle, *talk;
    if (!government_animation_pair(name, &idle, &talk))
        return 0;
    int requested = government_percent();
    // At 100% the native game owns both size and position, including any
    // intentional edge cropping. Never fit or shift the original portrait.
    if (requested == 100) {
        *out = ui_identity;
        return 1;
    }
    Rect bounds = government_native_bounds(piece);
    if (piece->parent) {
        // Include the inactive phase too, so speaking never changes the pivot.
        for (Control *p = piece->parent->first; p; p = p->next) {
            const Wide *other = control_name(p);
            if (p->loop != piece->loop || (!same_text(other, idle) && !same_text(other, talk)))
                continue;
            Rect r = government_native_bounds(p);
            if (r.right <= r.left || r.bottom <= r.top)
                continue;
            bounds.left = ui_min(bounds.left, r.left);
            bounds.top = ui_min(bounds.top, r.top);
            bounds.right = ui_max(bounds.right, r.right);
            bounds.bottom = ui_max(bounds.bottom, r.bottom);
        }
    }
    int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
    if (width <= 0 || height <= 0 || width > 65536 || height > 65536)
        return 0;
    int scale = ui_min(requested, (view.x - 16) * 100 / width);
    scale = ui_min(scale, (view.y - 16) * 100 / height);
    if (scale < 1)
        return 0;
    // Enlarge around the native bottom centre, then move only as far as is
    // necessary to fit the entire pair inside a constant eight-pixel margin.
    int x = (100 - scale) * (bounds.left + width / 2);
    int y = (100 - scale) * bounds.bottom;
    x = ui_max(800 - bounds.left * scale,
               ui_min((view.x - 8) * 100 - bounds.right * scale, x));
    y = ui_max(800 - bounds.top * scale,
               ui_min((view.y - 8) * 100 - bounds.bottom * scale, y));
    *out = (UiTransform){scale, x, y};
    return 1;
}
