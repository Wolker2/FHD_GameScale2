#ifndef FHD_SCALE_MATH_H
#define FHD_SCALE_MATH_H

typedef struct { int x, y; } Point;
typedef struct { int left, top, right, bottom; } Rect;

static inline int fhd_dialog_width(int original, int left, int percent)
{
    if (percent <= 100 || percent > 150 || original < 1 || original > 4096 || left < 1)
        return original;
    int extra = original * (percent - 100) / 100;
    if (extra > left)
        extra = left;
    return original + extra;
}

static inline int fhd_wide_x(int x, int original, int widened)
{
    int product = x * widened;
    int adjustment = product < 0 ? -original / 2 : original / 2;
    return (product + adjustment) / original;
}

static inline int fhd_equal_rect(Rect a, Rect b)
{
    return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
}

static inline int fhd_gov_width(int original, int left, int screen, int percent)
{
    if (original < 1 || left < 0 || screen < 800 || screen > 16384 ||
        percent <= 100 || percent > 150)
        return original;
    int center = left + original / 2;
    int edge = screen / 2 + 96;
    int room = edge - center;
    if (center - 12 < room)
        room = center - 12;
    int wanted = original * percent / 100;
    if (wanted > room * 2)
        wanted = room * 2;
    return wanted > original ? wanted : original;
}

static inline int fhd_scale_px(int value, int percent)
{
    int product = value * percent;
    return (product + (product < 0 ? -50 : 50)) / 100;
}

static inline int fhd_hud_percent(int width, int requested)
{
    if (width < 1024 || width > 16384 || requested <= 100 || requested > 200)
        return 100;
    int limit = (width - 42) * 100 / 970;
    if (limit < 105)
        return 100;
    return requested < limit ? requested : limit;
}

static inline int fhd_nav_x(int x, int width, int percent)
{
    const int left = 430, right = 540;
    int new_left = fhd_scale_px(left, percent);
    int new_right = fhd_scale_px(right, percent);
    if (x <= left)
        return fhd_scale_px(x, percent);
    if (x >= width - right)
        return width - fhd_scale_px(width - x, percent);
    return new_left + (x - left) * (width - new_left - new_right) / (width - left - right);
}

static inline int fhd_nav_source_x(int x, int width, int percent)
{
    const int left = 430, right = 540;
    int new_left = fhd_scale_px(left, percent);
    int new_right = fhd_scale_px(right, percent);
    if (x < new_left)
        return x * left / new_left;
    if (x >= width - new_right)
        return width - right + (x - (width - new_right)) * right / new_right;
    return left + (x - new_left) * (width - left - right) / (width - new_left - new_right);
}

#endif
