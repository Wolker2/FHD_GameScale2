// The engine keeps GUI positions in its own coordinates. Only the draw clip,
// Direct3D vertices and the corresponding input points cross into screen space.
typedef struct {
    U32 hardware_flag_ref, queue_rect;
    U32 intersect, clip_child, clip_queued;
    U32 draw_child, draw_queued;
    U32 contains, cursor, invalidate, image_hit;
    U32 mouse[20], primitive[11];
} UiBuild;

static const UiBuild ui_builds[] = {
    {
        .hardware_flag_ref = 0x8130f0, .queue_rect = 0x4c326c,
        .intersect = 0x45ed6c, .clip_child = 0x4c1b51, .clip_queued = 0x4c1bf0,
        .draw_child = 0x4c1b62, .draw_queued = 0x4c1c7e,
        .contains = 0x4c182c, .cursor = 0x4c5ecc, .invalidate = 0x4c1954, .image_hit = 0x4999a4,
        .mouse = {0x4c135a, 0x4c1574, 0x4c15d4, 0x4c1635, 0x4c16ac, 0x4c170c, 0x4c176c,
                  0x4c49a0, 0x4c49e5, 0x4c4b23, 0x4c4b6f, 0x4c4b9d, 0x4c4bf9, 0x4c4d6f,
                  0x4c4ddf, 0x4c4e1c, 0x4c4e99, 0x4c4ed6, 0x4c52f5, 0x4c676a},
        .primitive = {0x4b8202, 0x4b97ca, 0x4b9f36, 0x4caaaa, 0x4caba4, 0x4cb754,
                      0x4cb8c6, 0x4cb8df, 0x4cbc30, 0x4cbe19, 0x4cc08d},
    },
    {
        .hardware_flag_ref = 0x8820f0, .queue_rect = 0x4cc654,
        .intersect = 0x45ee1c, .clip_child = 0x4cac93, .clip_queued = 0x4cad3c,
        .draw_child = 0x4caca4, .draw_queued = 0x4caddc,
        .contains = 0x4ca84c, .cursor = 0x4cf83c, .invalidate = 0x4caa04, .image_hit = 0x48a87c,
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

static int general_ui_percent(void)
{
    if (!ui_build)
        return 100;
    const Byte *hardware = *(const Byte **)game_address(ui_build->hardware_flag_ref);
    if (!hardware || !*hardware)
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
    // Keep the native special layouts in charge of their geometry and artwork.
    if (same_text(name, L"PanelMain") || same_text(name, L"PanelPlanet"))
        return 1;
    if (same_text(name, L"SpaceImg") || has_prefix(name, L"StarField") ||
        has_prefix(name, L"GovHD_Anim") || has_prefix(name, L"Gov_Anim") ||
        same_text(name, L"BGBuf") || same_text(name, L"CircleActionShr") ||
        same_text(name, L"CircleActionColor") || has_prefix(name, L"CircleActionWeaponColor"))
        return 1;
    for (unsigned i = 0; i < COUNT_OF(minimaps); ++i) {
        const MinimapLayout *layout = &minimaps[i];
        if (layout->loop != loop)
            continue;
        if ((layout->parent && control->parent == layout->parent &&
             (control == layout->map || control == layout->blind ||
              control == layout->center || control == layout->border)) ||
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

static UiTransform control_transform(Control *control)
{
    int percent = general_ui_percent();
    if (percent == 100 || !control || !control->loop)
        return ui_identity;
    void *loop = control->loop;
    Point view = {screen_width(), screen_height()};
    if (view.x < 800 || view.y < 600 || view.x > 16384 || view.y > 16384)
        return ui_identity;
    Control *group = 0;
    for (Control *p = control; p && p->loop == loop; p = p->parent) {
        const Wide *name = control_name(p);
        if (fixed_layer(p, loop, name))
            return ui_identity;
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
    const float *auto_anchor = (const float *)((Byte *)group + 0x4c);
    int auto_enabled = *((Byte *)group + 0x48);
    Point pivot = {
        anchor_axis(bounds.left, bounds.right, view.x, 1024, auto_enabled ? auto_anchor[0] : -1),
        anchor_axis(bounds.top, bounds.bottom, view.y, 768, auto_enabled ? auto_anchor[1] : -1)
    };
    const MinimapLayout *flight = &minimaps[radar_flight];
    if (flight->loop == loop && group == flight->find_quest)
        pivot = (Point){bounds.right, bounds.top};
    if (same_text(control_name(group), L"PanelTalk"))
        view.y = dialogue_screen_height();
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
} DrawContext;

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
    if (general_ui_percent() == 100)
        return native_intersect(out, clip, bounds);
    Control *child = (Control *)((Byte *)bounds - 0x60);
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
    if (general_ui_percent() == 100)
        return native_intersect(out, clip, bounds);
    Rect screen_bounds = physical_bounds((Control *)((Byte *)bounds - 0x60));
    return native_intersect(out, clip, &screen_bounds);
}

static void DELPHI draw_scaled_control(Control *control, const Rect *screen_clip)
{
    void (DELPHI *draw)(Control *, const Rect *) = control->methods[0xa0 / 4];
    if (general_ui_percent() == 100) {
        draw(control, screen_clip);
        return;
    }
    UiTransform t = control_transform(control);
    DrawContext saved = draw_context;
    U32 frame;
    __asm__ volatile("movl %%esp, %0" : "=r"(frame));
    draw_context = (DrawContext){t, *screen_clip, frame};
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

static Byte DELPHI contains_scaled_point(Control *control, const Point *point)
{
    if (!*((Byte *)control + 0x41) || *((Byte *)control + 0x42) == 1)
        return 0;
    Point screen = screen_input_point(control->loop, *point);
    Rect bounds = physical_bounds(control);
    return screen.x >= bounds.left && screen.x < bounds.right &&
           screen.y >= bounds.top && screen.y < bounds.bottom;
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
    UiTransform t = control_transform(control);
    Point logical = ui_logical_point(t, screen_input_point(control->loop, *point));
    return image_hit_original(control, &logical);
}

static void dispatch_scaled_mouse(Control *control, U32 keys, const Point *point, int slot)
{
    void (DELPHI *dispatch)(Control *, U32, const Point *) = control->methods[slot / 4];
    if (general_ui_percent() == 100) {
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
    UiTransform t = control_transform(control);
    if (t.percent == 100 || !*((Byte *)control + 0x41) || !control->parent) {
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
    if (!within_native_call(draw_context.frame) || draw_context.transform.percent <= 100 ||
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
    }

    ScissorMethod get_scissor = methods[76], set_scissor = methods[75];
    Rect prior;
    if (get_scissor(device, &prior) < 0)
        return draw(device, type, count, output, stride);
    Rect viewport = {0, 0, screen_width(), screen_height()};
    Rect clip = draw_context.clip;
    if (!fhd_equal_rect(prior, viewport) &&
        !ui_intersection(&clip, clip, ui_screen_rect(transform, prior)))
        return 0;
    if (!ui_intersection(&clip, clip, viewport))
        return 0;
    if (set_scissor(device, &clip) < 0)
        return draw(device, type, count, output, stride);
    int result = draw(device, type, count, output, stride);
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
    Patch patches[39] = {
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
