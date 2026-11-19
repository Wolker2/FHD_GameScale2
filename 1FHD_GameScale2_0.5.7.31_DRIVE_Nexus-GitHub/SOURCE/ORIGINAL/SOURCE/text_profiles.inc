// Original font glyphs and weights are retained. Fractional UI scales use
// linear coverage with a symmetric contrast curve, rather than uneven
// nearest-neighbour stroke duplication or unrestricted blurry interpolation.
typedef struct {
    int percent;
    float contrast, bias;
} TextProfile;

static const TextProfile text_profiles[] = {
    {125, 1.50f, -0.250f},
    {150, 1.40f, -0.200f},
    {175, 1.25f, -0.125f},
    {200, 1.00f,  0.000f},
};

static unsigned text_profile_index(int percent)
{
    if (percent >= 200) return 3;
    if (percent >= 163) return 2;
    if (percent >= 138) return 1;
    return 0;
}

typedef U32 (WINAPI *ComRelease)(void *);
typedef int (WINAPI *GetTexture)(void *, U32, void **);
typedef int (WINAPI *CreatePixelShader)(void *, const U32 *, void **);
typedef int (WINAPI *ShaderState)(void *, void *);
typedef int (WINAPI *GetShaderState)(void *, void **);

static void *text_shader_device, *text_shaders[3];
static Byte text_shader_attempted[3];

static void release_text_object(void *object)
{
    if (object)
        ((ComRelease)(*(void ***)object)[2])(object);
}

static void *text_profile_shader(void *device, unsigned profile)
{
    if (device != text_shader_device) {
        for (unsigned i = 0; i < COUNT_OF(text_shaders); ++i) {
            release_text_object(text_shaders[i]);
            text_shaders[i] = 0;
            text_shader_attempted[i] = 0;
        }
        text_shader_device = device;
    }
    if (profile >= COUNT_OF(text_shaders) || text_shader_attempted[profile])
        return profile < COUNT_OF(text_shaders) ? text_shaders[profile] : 0;
    text_shader_attempted[profile] = 1;
    // ps_2_0: dcl t0.xy; dcl v0; dcl_2d s0; def c0;
    // texld r0,t0,s0; mad_sat r0.a,r0.a,c0.x,c0.y; mul oC0,r0,v0.
    U32 code[] = {
        0xffff0200,
        0x0200001f, 0x80000000, 0xb0030000,
        0x0200001f, 0x80000000, 0x900f0000,
        0x0200001f, 0x90000000, 0xa00f0800,
        0x05000051, 0xa00f0000, 0, 0, 0, 0,
        0x03000042, 0x800f0000, 0xb0e40000, 0xa0e40800,
        0x04000004, 0x80180000, 0x80ff0000, 0xa0000000, 0xa0550000,
        0x03000005, 0x800f0800, 0x80e40000, 0x90e40000,
        0x0000ffff
    };
    union { float f; U32 u; } contrast = {.f=text_profiles[profile].contrast};
    union { float f; U32 u; } bias = {.f=text_profiles[profile].bias};
    code[12] = contrast.u;
    code[13] = bias.u;
    CreatePixelShader create = (*(void ***)device)[106];
    if (create(device, code, &text_shaders[profile]) < 0)
        text_shaders[profile] = 0;
    return text_shaders[profile];
}

static int begin_text_profile(void *device)
{
    unsigned profile = text_profile_index(draw_context.transform.percent);
    if (profile == 3)
        return 0; // Exact 2x glyphs retain the sharp native pixel grid.
    void **methods = *(void ***)device;
    GetTexture get_texture = methods[64];
    CreatePixelShader create = methods[106];
    ShaderState set_shader = methods[107];
    GetShaderState get_shader = methods[108];
    if (!get_texture || !create || !set_shader || !get_shader)
        return 0;
    void *texture = 0, *previous = 0;
    if (get_texture(device, 0, &texture) < 0 || !texture)
        return 0; // Lines and solid-colour label decorations do not sample a font.
    release_text_object(texture);
    if (get_shader(device, &previous) < 0)
        return 0;
    if (previous) {
        release_text_object(previous);
        return 0; // Preserve any rendering program installed by another mod.
    }
    void *shader = text_profile_shader(device, profile);
    if (!shader)
        return 0;
    SetSamplerState set = methods[69];
    if (set(device, 0, sampler_mag_filter, 2) < 0 ||
        set(device, 0, sampler_min_filter, 2) < 0 || set_shader(device, shader) < 0) {
        set(device, 0, sampler_mag_filter, texture_point_filter);
        set(device, 0, sampler_min_filter, texture_point_filter);
        return 0;
    }
    return 1;
}
