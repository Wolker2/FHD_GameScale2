// Font acquisition happens before native measurement as well as rendering.
// Rebind only known body fonts, preserving the native cache/refcount API.
// Private aliases prevent the game's per-draw smoothing toggle from restoring
// stock keys, rebuilding text textures or changing metrics on every frame.
BSTR(font_style_key, L"FHDFontStyle");
BSTR(font_style_caption_ru, L"Вариант шрифта");
BSTR(font_style_caption_en, L"Font style");
BSTR(font_original_ru, L"Оригинальный");
BSTR(font_original_en, L"Original");
BSTR(font_orbit, L"Exo 2");
BSTR(font_dense_ru, L"Verdana плотный");
BSTR(font_dense_en, L"Verdana dense");
BSTR(font_compact_ru, L"Игровой полужирный");
BSTR(font_compact_en, L"Game semibold");
BSTR(font_terminal, L"Courier");
BSTR(font_script_ru, L"Каллиграфический жирный");
BSTR(font_script_en, L"Calligraphic bold");
BSTR(font_print_ru, L"Печатный жирный");
BSTR(font_print_en, L"Print bold");
BSTR(font_script_regular_ru, L"Каллиграфический обычный");
BSTR(font_script_regular_en, L"Calligraphic regular");
BSTR(font_print_regular_ru, L"Печатный обычный");
BSTR(font_print_regular_en, L"Print regular");
BSTR(font_value0, L"0");
BSTR(font_value1, L"1");
BSTR(font_value2, L"2");
BSTR(font_value3, L"3");
BSTR(font_value4, L"4");
BSTR(font_value5, L"5");
BSTR(font_value6, L"6");
BSTR(font_value7, L"7");
BSTR(font_value8, L"8");
static const ScaleChoice font_choices_ru[] = {
    {0, font_original_ru.text, font_value0.text}, {1, font_orbit.text, font_value1.text},
    {2, font_dense_ru.text, font_value2.text}, {3, font_compact_ru.text, font_value3.text},
    {4, font_terminal.text, font_value4.text},
    {5, font_script_ru.text, font_value5.text},
    {6, font_print_ru.text, font_value6.text},
    {7, font_script_regular_ru.text, font_value7.text},
    {8, font_print_regular_ru.text, font_value8.text},
};
static const ScaleChoice font_choices_en[] = {
    {0, font_original_en.text, font_value0.text}, {1, font_orbit.text, font_value1.text},
    {2, font_dense_en.text, font_value2.text}, {3, font_compact_en.text, font_value3.text},
    {4, font_terminal.text, font_value4.text},
    {5, font_script_en.text, font_value5.text},
    {6, font_print_en.text, font_value6.text},
    {7, font_script_regular_en.text, font_value7.text},
    {8, font_print_regular_en.text, font_value8.text},
};
static int cached_font_style = -1;
static U32 native_font_tail __attribute__((used));
static U32 native_font_class_ref __attribute__((used));

static int read_font_style(void)
{
    return read_percent(font_style_key.text, font_choices_en, COUNT_OF(font_choices_en));
}

static int selected_font_style(void)
{
    if (cached_font_style < 0 && *(void **)game_address(build->cfg))
        cached_font_style = read_font_style();
    return cached_font_style < 0 ? 0 : cached_font_style;
}

BSTR(style_v8, L"Font.FHDGS23_v8");
BSTR(style_v8b, L"Font.FHDGS23_v8b");
BSTR(style_v9, L"Font.FHDGS23_v9");
BSTR(style_v9b, L"Font.FHDGS23_v9b");
BSTR(style_v11, L"Font.FHDGS23_v11");
BSTR(style_v12, L"Font.FHDGS23_v12");
BSTR(style_v12b, L"Font.FHDGS23_v12b");
BSTR(style_v13, L"Font.FHDGS23_v13");
BSTR(style_s9, L"Font.FHDGS23_s9");
BSTR(style_s11, L"Font.FHDGS23_s11");
BSTR(style_s12, L"Font.FHDGS23_s12");
BSTR(style_s13, L"Font.FHDGS23_s13");
BSTR(style_s14, L"Font.FHDGS23_s14");
BSTR(style_c10, L"Font.FHDGS23_c10");
BSTR(style_c11, L"Font.FHDGS23_c11");
BSTR(style_c12, L"Font.FHDGS23_c12");
BSTR(style_c13, L"Font.FHDGS23_c13");
BSTR(style_script14, L"Font.FHDGS27_script14");
BSTR(style_script15, L"Font.FHDGS27_script15");
BSTR(style_script18, L"Font.FHDGS27_script18");
BSTR(style_script20, L"Font.FHDGS27_script20");
BSTR(style_script22, L"Font.FHDGS27_script22");
BSTR(style_script24, L"Font.FHDGS27_script24");
BSTR(style_print13, L"Font.FHDGS28_print13");
BSTR(style_print14, L"Font.FHDGS28_print14");
BSTR(style_print16, L"Font.FHDGS28_print16");
BSTR(style_print18, L"Font.FHDGS28_print18");
BSTR(style_print20, L"Font.FHDGS28_print20");
BSTR(style_print22, L"Font.FHDGS28_print22");
BSTR(style_script_regular14, L"Font.FHDGS29_script_regular14");
BSTR(style_script_regular15, L"Font.FHDGS29_script_regular15");
BSTR(style_script_regular18, L"Font.FHDGS29_script_regular18");
BSTR(style_script_regular20, L"Font.FHDGS29_script_regular20");
BSTR(style_script_regular22, L"Font.FHDGS29_script_regular22");
BSTR(style_script_regular24, L"Font.FHDGS29_script_regular24");
BSTR(style_print_regular13, L"Font.FHDGS29_print_regular13");
BSTR(style_print_regular14, L"Font.FHDGS29_print_regular14");
BSTR(style_print_regular16, L"Font.FHDGS29_print_regular16");
BSTR(style_print_regular18, L"Font.FHDGS29_print_regular18");
BSTR(style_print_regular20, L"Font.FHDGS29_print_regular20");
BSTR(style_print_regular22, L"Font.FHDGS29_print_regular22");
BSTR(style_orbit13, L"Font.FHDGS30_orbit13");
BSTR(style_orbit14, L"Font.FHDGS30_orbit14");
BSTR(style_orbit16, L"Font.FHDGS30_orbit16");
BSTR(style_orbit18, L"Font.FHDGS30_orbit18");
BSTR(style_orbit20, L"Font.FHDGS30_orbit20");
BSTR(style_orbit22, L"Font.FHDGS30_orbit22");
BSTR(style_orbit_b13, L"Font.FHDGS30_orbit_b13");
BSTR(style_orbit_b14, L"Font.FHDGS30_orbit_b14");
BSTR(style_orbit_b16, L"Font.FHDGS30_orbit_b16");
BSTR(style_orbit_b18, L"Font.FHDGS30_orbit_b18");
BSTR(style_orbit_b20, L"Font.FHDGS30_orbit_b20");
BSTR(style_orbit_b22, L"Font.FHDGS30_orbit_b22");

typedef struct { const Wide *key; int size, bold; } FontRole;
static const FontRole font_roles[] = {
    {L"Font.1Small",0,0}, {L"Font.1Normal",0,0}, {L"Font.2Small",0,0}, {L"Font.Verdana8",0,0},
    {L"Font.1SmallBold",0,1}, {L"Font.1NormalBold",0,1}, {L"Font.2SmallBold",0,1}, {L"Font.Verdana8bold",0,1},
    {L"Font.2Normal",1,0}, {L"Font.1Intro",1,0}, {L"Font.Verdana9",1,0},
    {L"Font.2NormalBold",1,1}, {L"Font.Verdana9bold",1,1},
    {L"Font.2Intro",2,0}, {L"Font.Verdana11",2,0},
    {L"Font.1Big",3,1}, {L"Font.Verdana12",3,0}, {L"Font.Verdana12bold",3,1},
    {L"Font.Verdana13",4,0}, {L"Font.2Big",5,0}, {L"Font.1Huge",5,0}, {L"Font.2Huge",5,0},
    {L"Font.Storopia09",1,0}, {L"Font.Storopia10",1,0}, {L"Font.Storopia11",2,0},
    {L"Font.Storopia12",3,0}, {L"Font.Storopia13",4,0}, {L"Font.Storopia14",5,0},
};

static const Wide *style_font_key(const Wide *key, int style)
{
    if (!key || style < 1 || style > 8)
        return key;
    const FontRole *role = 0;
    for (unsigned i = 0; i < COUNT_OF(font_roles); ++i) {
        if (same_text(key, font_roles[i].key)) {
            role = &font_roles[i];
            break;
        }
    }
    if (!role)
        return key; // Decorative, tiny, fixed-width and custom mod fonts stay intact.
    if (style == 1) {
        const Wide *normal[] = {style_orbit13.text,style_orbit14.text,style_orbit16.text,
            style_orbit18.text,style_orbit20.text,style_orbit22.text};
        const Wide *bold[] = {style_orbit_b13.text,style_orbit_b14.text,style_orbit_b16.text,
            style_orbit_b18.text,style_orbit_b20.text,style_orbit_b22.text};
        return role->bold ? bold[role->size] : normal[role->size];
    }
    if (style == 7) {
        // Native regular outlines: no bold expansion, including bold text roles.
        const Wide *keys[] = {style_script_regular14.text,style_script_regular15.text,
            style_script_regular18.text,style_script_regular20.text,
            style_script_regular22.text,style_script_regular24.text};
        return keys[role->size];
    }
    if (style == 8) {
        const Wide *keys[] = {style_print_regular13.text,style_print_regular14.text,
            style_print_regular16.text,style_print_regular18.text,
            style_print_regular20.text,style_print_regular22.text};
        return keys[role->size];
    }
    if (style == 6) {
        // Upright bold print, including both native regular and bold text roles.
        const Wide *keys[] = {style_print13.text,style_print14.text,style_print16.text,
            style_print18.text,style_print20.text,style_print22.text};
        return keys[role->size];
    }
    if (style == 5) {
        // Baked bold script glyphs, acquired before measuring and wrapping.
        // Both normal and bold roles use the same calligraphic family.
        const Wide *keys[] = {style_script14.text,style_script15.text,style_script18.text,
            style_script20.text,style_script22.text,style_script24.text};
        return keys[role->size];
    }
    if (style == 3) {
        // The replacement variant has a nine-point bold minimum, including
        // native small/bold labels. Other variants retain their own sizes.
        const Wide *keys[] = {style_s9.text,style_s9.text,style_s11.text,style_s12.text,style_s13.text,style_s14.text};
        return keys[role->size];
    }
    if (role->bold || style == 2)
        return role->size == 0 ? style_v8b.text : role->size == 1 ? style_v9b.text : style_v12b.text;
    const Wide *keys[] = {style_c10.text,style_c11.text,style_c12.text,style_c12.text,style_c13.text,style_c13.text};
    return keys[role->size];
}

static CALLED_FROM_ASM void prepare_font_cache(void *control)
{
    // Also catch configuration becoming available after early plugin loading.
    // This existing callback runs before the settings screen is laid out.
    disable_government_tables(0);
    int style = selected_font_style();
    if (!style || !control)
        return;
    const Wide *key = *(const Wide **)((Byte *)control + 0x10);
    const Wide *target = style_font_key(key, style);
    if (target && !same_text(key, target)) {
        void **methods = *(void ***)control;
        void (DELPHI *set_key)(void *, const Wide *) = methods[1];
        set_key(control, target);
    }
}

static ASSEMBLY void font_acquire_new(void)
{
    // The native entry has already saved EBX and copied EAX into EBX.
    __asm__("pushal\n pushl %eax\n call prepare_font_cache\n addl $4,%esp\n popal\n"
            "movl native_font_class_ref,%edx\n movl (%edx),%edx\n jmp *native_font_tail");
}

static ASSEMBLY void font_acquire_old(void)
{
    __asm__("pushal\n pushl %eax\n call prepare_font_cache\n addl $4,%esp\n popal\n"
            "pushl %ebp\n movl %esp,%ebp\n subl $8,%esp\n jmp *native_font_tail");
}
