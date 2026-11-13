// The information screen has its own frame and builds results using PanelInfo.
BSTR(info_frame_path, L"GI,Bm.FormInfo3.2BG");
BSTR(info_close, L"ButFormClose");
BSTR(info_search, L"ButSearch");
BSTR(info_news, L"ButNews");
BSTR(info_next, L"ButNext");
BSTR(info_prev, L"ButPrev");
BSTR(info_preview, L"TV");
BSTR(info_query, L"PanelSearch");

static Control *DELPHI info_layout_hook(void *form, const Wide *key)
{
    Control *results = native_get(form, key);
    int percent = read_scale();
    if (percent == 100 || !is_native_type(results, build->scroll_set_size) || !results->parent)
        return results;

    Control *panel = results->parent;
    Control *frame = find_image(panel, info_frame_path.text);
    if (!frame || has_dialog_art(frame) || panel->size.x < 400 || panel->size.x > 800 ||
        frame->size.x != panel->size.x || frame->position.x != 0 ||
        results->size.x < 300 || results->size.x >= panel->size.x)
        return results;

    int original = panel->size.x;
    int width = ui_min(original * percent / 100, screen_width() - 24);
    if (width <= original)
        return results;
    Point frame_size = {width, frame->size.y};
    Control *art = prepare_art(frame, frame_size);
    if (!art)
        return results;

    int extra = width - original;
    set_position(panel, (Point){panel->position.x - extra / 2, panel->position.y});
    set_size(panel, (Point){width, panel->size.y});
    finish_art(frame, art, frame_size, 100);
    *(U32 *)((Byte *)art + 0x8c) = DIALOG_ART_MARKER;

    Control *bar = *(Control **)((Byte *)results + 0x144);
    Point bar_size = bar ? bar->size : (Point){0, 0};
    Point bar_position = bar ? bar->position : (Point){0, 0};
    Rect *bar_rect = (Rect *)((Byte *)results + 0x15a);
    bar_rect->left += extra;
    bar_rect->right += extra;
    set_size(results, (Point){results->size.x + extra, results->size.y});
    if (bar) {
        set_position(bar, (Point){bar_position.x + extra, bar_position.y});
        set_size(bar, bar_size);
    }

    for (Control *child = panel->first; child; child = child->next) {
        if (child == frame || child == results || child == bar ||
            child == *(Control **)((Byte *)results + 0x140))
            continue;
        Point pos = child->position;
        const Wide *name = control_name(child);
        if (same_text(name, info_query.text) || same_text(name, info_preview.text) ||
            (name && name[0] == 'P' && name[1] == 'a' && name[2] == 'n' && name[3] == 'e' &&
             name[4] == 'l' && name[5] == 'M' && name[6] >= '0' && name[6] <= '9'))
            pos.x += extra / 2;
        else if (is_native_type(child, build->image_set_size) &&
                 pos.x >= bar_position.x - 1 && child->size.x <= 32 &&
                 (same_text(image_path(child), L"GI,Bm.SB.2V1_UpH") ||
                  same_text(image_path(child), L"GI,Bm.SB.2V1_DownH")))
            pos.x += extra;
        else if (is_native_type(child, build->button_set_size) &&
                 (same_text(name, info_close.text) || same_text(name, info_search.text) ||
                  same_text(name, info_news.text) || same_text(name, info_prev.text) ||
                  same_text(name, info_next.text))) {
            int edge = fhd_wide_x(pos.x, original, width);
            int button_width = fhd_wide_x(pos.x + child->size.x, original, width) - edge;
            if (scale_button_axes(child, percent, 100, (Point){button_width, child->size.y}))
                pos.x = edge;
            else
                pos.x = fhd_wide_x(pos.x + child->size.x / 2, original, width) - child->size.x / 2;
        }
        set_position(child, pos);
    }
    return results;
}
