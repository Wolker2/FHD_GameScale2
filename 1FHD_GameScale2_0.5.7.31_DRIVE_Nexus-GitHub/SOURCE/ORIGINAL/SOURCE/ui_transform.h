#ifndef FHD_UI_TRANSFORM_H
#define FHD_UI_TRANSFORM_H

typedef struct {
    int percent;
    // Offsets are in hundredths of a physical pixel.
    int x100, y100;
} UiTransform;

static const UiTransform ui_identity = {100, 0, 0};

static inline int ui_is_identity(UiTransform t)
{
    return t.percent == 100 && !t.x100 && !t.y100;
}

static inline int ui_min(int a, int b) { return a < b ? a : b; }
static inline int ui_max(int a, int b) { return a > b ? a : b; }

static inline int ui_floor_div(int value, int divisor)
{
    int quotient = value / divisor;
    return quotient - (value % divisor < 0);
}

static inline int ui_ceil_div(int value, int divisor)
{
    return -ui_floor_div(-value, divisor);
}

static inline int ui_same_transform(UiTransform a, UiTransform b)
{
    return a.percent == b.percent && a.x100 == b.x100 && a.y100 == b.y100;
}

static inline UiTransform ui_place(int percent, Point logical, Point physical)
{
    return (UiTransform){percent, physical.x * 100 - logical.x * percent,
                        physical.y * 100 - logical.y * percent};
}

static inline Rect ui_screen_rect(UiTransform t, Rect r)
{
    if (ui_is_identity(t))
        return r;
    // Use the same boundary for the start and end of adjacent controls.
    return (Rect){
        ui_ceil_div(r.left * t.percent + t.x100 - 50, 100),
        ui_ceil_div(r.top * t.percent + t.y100 - 50, 100),
        ui_ceil_div(r.right * t.percent + t.x100 - 50, 100),
        ui_ceil_div(r.bottom * t.percent + t.y100 - 50, 100)
    };
}

static inline Rect ui_logical_clip(UiTransform t, Rect r)
{
    if (ui_is_identity(t))
        return r;
    // Round outward to keep all source pixels that overlap the physical clip.
    return (Rect){
        ui_floor_div(r.left * 100 - t.x100, t.percent),
        ui_floor_div(r.top * 100 - t.y100, t.percent),
        ui_ceil_div(r.right * 100 - t.x100, t.percent),
        ui_ceil_div(r.bottom * 100 - t.y100, t.percent)
    };
}

static inline Point ui_logical_point(UiTransform t, Point p)
{
    if (ui_is_identity(t))
        return p;
    return (Point){ui_floor_div(p.x * 100 + 50 - t.x100, t.percent),
                   ui_floor_div(p.y * 100 + 50 - t.y100, t.percent)};
}

static inline Point ui_screen_point(UiTransform t, Point p)
{
    if (ui_is_identity(t))
        return p;
    return (Point){ui_floor_div(p.x * t.percent + t.x100 + t.percent / 2, 100),
                   ui_floor_div(p.y * t.percent + t.y100 + t.percent / 2, 100)};
}

static inline int ui_intersection(Rect *out, Rect a, Rect b)
{
    *out = (Rect){ui_max(a.left, b.left), ui_max(a.top, b.top),
                  ui_min(a.right, b.right), ui_min(a.bottom, b.bottom)};
    return out->left < out->right && out->top < out->bottom;
}

static inline int ui_axis_offset(int percent, int low, int high, int extent, int pivot)
{
    int offset = (100 - percent) * pivot;
    int min_offset = -low * percent;
    int max_offset = extent * 100 - high * percent;
    return ui_max(min_offset, ui_min(max_offset, offset));
}

static inline UiTransform ui_group_transform(int percent, Rect bounds, Point view, Point pivot)
{
    int w = bounds.right - bounds.left;
    int h = bounds.bottom - bounds.top;
    if (w < 1 || h < 1)
        return ui_identity;
    // Keep a large window usable when the requested size exceeds the screen.
    percent = ui_min(percent, ui_min(view.x * 100 / w, view.y * 100 / h));
    if (percent <= 100)
        return ui_identity;
    return (UiTransform){percent,
        ui_axis_offset(percent, bounds.left, bounds.right, view.x, pivot.x),
        ui_axis_offset(percent, bounds.top, bounds.bottom, view.y, pivot.y)};
}

#endif
