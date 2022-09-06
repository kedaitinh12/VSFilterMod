
#include "stdafx.h"
#include "OverlayMix.h"
#include <math.h>

static __forceinline DWORD blendMixColor(DWORD* dst, DWORD* color, MOD_BLEND mod_blendMode) {
    if (mod_blendMode == BLEND_NORMAL) {
        return *color;
    }
    DWORD c_rb = (*color & 0x00ff00ff);
    DWORD c_g = (*color & 0x0000ff00);
    DWORD d_rb = (*dst & 0x00ff00ff);
    DWORD d_g = (*dst & 0x0000ff00);
    DWORD blend_rb, blend_g;
    if (mod_blendMode == BLEND_OVERLAY) {
        DWORD tmp_r, tmp_g, tmp_b;
        if (((d_rb >> 16) & 0x000000ff) < 0x80) {
            tmp_r = 2 * div_255_fast(((c_rb >> 16) & 0x000000ff) * ((d_rb >> 16) & 0x000000ff));
        }
        else {
            tmp_r = 0xff - 2 * div_255_fast((0x000000ff - ((c_rb >> 16) & 0x000000ff)) * (0x000000ff - ((d_rb >> 16) & 0x000000ff)));
        }
        if ((d_g >> 8) < 0x80) {
            tmp_g = 2 * div_255_fast((c_g >> 8) * (d_g >> 8));
        }
        else {
            tmp_g = 0xff - 2 * div_255_fast((0x000000ff - (c_g >> 8)) * (0x000000ff - (d_g >> 8)));
        }
        if ((d_rb & 0x000000ff) < 0x80) {
            tmp_b = 2 * div_255_fast((c_rb & 0x000000ff) * (d_rb & 0x000000ff));
        }
        else {
            tmp_b = 0xff - 2 * div_255_fast((0x000000ff - (c_rb & 0x000000ff)) * (0x000000ff - (d_rb & 0x000000ff)));
        }
        blend_rb = tmp_r << 16 | tmp_b;
        blend_g = tmp_g << 8;
    }
    else if (mod_blendMode == BLEND_ADD) {
        DWORD tmp_r = (c_rb & 0x00ff0000) + (d_rb & 0x00ff0000);
        DWORD tmp_g = c_g + d_g;
        DWORD tmp_b = (c_rb & 0x000000ff) + (d_rb & 0x000000ff);
        tmp_r = tmp_r <= 0x00ff0000 ? tmp_r : 0x00ff0000;
        tmp_g = tmp_g <= 0x0000ff00 ? tmp_g : 0x0000ff00;
        tmp_b = tmp_b <= 0x000000ff ? tmp_b : 0x000000ff;
        blend_rb = tmp_r | tmp_b;
        blend_g = tmp_g;
    }
    else if (mod_blendMode == BLEND_SUBSTRACT) {
        DWORD tmp_r = (d_rb & 0x00ff0000) > (c_rb & 0x00ff0000) ? (d_rb & 0x00ff0000) - (c_rb & 0x00ff0000) : 0;
        DWORD tmp_g = d_g > c_g ? d_g - c_g : 0;
        DWORD tmp_b = (d_rb & 0x000000ff) > (c_rb & 0x000000ff) ? (d_rb & 0x000000ff) - (c_rb & 0x000000ff) : 0;
        blend_rb = tmp_r | tmp_b;
        blend_g = tmp_g;
    }
    else if (mod_blendMode == BLEND_MULTIPLY) {
        DWORD tmp_r = div_255_fast(((c_rb >> 16) & 0x000000ff) * ((d_rb >> 16) & 0x000000ff));
        DWORD tmp_g = div_255_fast((c_g >> 8) * (d_g >> 8));
        DWORD tmp_b = div_255_fast((c_rb & 0x000000ff) * (d_rb & 0x000000ff));
        blend_rb = tmp_r << 16 | tmp_b;
        blend_g = tmp_g << 8;
    }
    else if (mod_blendMode == BLEND_SCREEN) {
        DWORD tmp_r = 0xff - div_255_fast((0x000000ff - ((c_rb >> 16) & 0x000000ff)) * (0x000000ff - ((d_rb >> 16) & 0x000000ff)));
        DWORD tmp_g = 0xff - div_255_fast((0x000000ff - (c_g >> 8)) * (0x000000ff - (d_g >> 8)));
        DWORD tmp_b = 0xff - div_255_fast((0x000000ff - (c_rb & 0x000000ff)) * (0x000000ff - (d_rb & 0x000000ff)));
        blend_rb = tmp_r << 16 | tmp_b;
        blend_g = tmp_g << 8;
    }
    else if (mod_blendMode == BLEND_DIFFERENCE) {
        DWORD tmp_r = (c_rb & 0x00ff0000) > (d_rb & 0x00ff0000) ? (c_rb & 0x00ff0000) - (d_rb & 0x00ff0000) : (d_rb & 0x00ff0000) - (c_rb & 0x00ff0000);
        DWORD tmp_g = c_g > d_g ? c_g - d_g : d_g - c_g;
        DWORD tmp_b = (c_rb & 0x000000ff) > (d_rb & 0x000000ff) ? (c_rb & 0x000000ff) - (d_rb & 0x000000ff) : (d_rb & 0x000000ff) - (c_rb & 0x000000ff);
        blend_rb = tmp_r | tmp_b;
        blend_g = tmp_g;
    }
    else if (mod_blendMode == BLEND_SUBSTRACT_REVERSE) {
        DWORD tmp_r = (c_rb & 0x00ff0000) > (d_rb & 0x00ff0000) ? (c_rb & 0x00ff0000) - (d_rb & 0x00ff0000) : 0;
        DWORD tmp_g = c_g > d_g ? c_g - d_g : 0;
        DWORD tmp_b = (c_rb & 0x000000ff) > (d_rb & 0x000000ff) ? (c_rb & 0x000000ff) - (d_rb & 0x000000ff) : 0;
        blend_rb = tmp_r | tmp_b;
        blend_g = tmp_g;
    }
    else if (mod_blendMode == BLEND_SUBSTRACT_INVERSE) {
        d_rb = 0x00ff00ff - d_rb;
        d_g = 0x0000ff00 - d_g;
        DWORD tmp_r = (c_rb & 0x00ff0000) > (d_rb & 0x00ff0000) ? (c_rb & 0x00ff0000) - (d_rb & 0x00ff0000) : 0;
        DWORD tmp_g = c_g > d_g ? c_g - d_g : 0;
        DWORD tmp_b = (c_rb & 0x000000ff) > (d_rb & 0x000000ff) ? (c_rb & 0x000000ff) - (d_rb & 0x000000ff) : 0;
        blend_rb = tmp_r | tmp_b;
        blend_g = tmp_g;
    }
    else {
        blend_rb = c_rb;
        blend_g = c_g;
    }
    return blend_rb | blend_g;
}

// COverlayMixer class
COverlayMixer::COverlayMixer(RasterizerNfo* Info, COverlayGetter* Color)
{
    this->Info = Info;
    this->Color = Color;
}

void COverlayMixer::PixMix(DWORD* dst, DWORD color, BYTE alpha, MOD_BLEND mod_blendMode = BLEND_NORMAL)
{
    DWORD a = ((alpha * (color >> 24)) >> 6) & 0xff;
    DWORD ia = 256 - a;
    a += 1;

    DWORD blendColor = blendMixColor(dst, &color, mod_blendMode);

    *dst = ((((*dst & 0x00ff00ff) * ia + (color & 0x00ff00ff) * a) & 0xff00ff00) >> 8)
        | ((((*dst & 0x0000ff00) * ia + (color & 0x0000ff00) * a) & 0x00ff0000) >> 8)
        | ((((*dst >> 8) & 0x00ff0000) * ia) & 0xff000000);
}

DWORD COverlayMixer::SafeSubstract(DWORD a, DWORD b)
{
#ifndef _WIN64
    __m64 ap = _mm_cvtsi32_si64(a);
    __m64 bp = _mm_cvtsi32_si64(b);
    __m64 rp = _mm_subs_pu16(ap, bp);
    DWORD r = (DWORD)_mm_cvtsi64_si32(rp);
    _mm_empty();
    return r;
#else
    // For whatever reason Microsoft's x64 compiler doesn't support MMX intrinsics
    return (b > a) ? 0 : a - b;
#endif
}

// Draw
void COverlayMixer::Draw(bool Body)
{
    int h = Info->h;

    byte* s = Info->s;
    DWORD* dst = Info->dst;
    int gran = (Info->sw[1] == 0xffffffff) ? Info->w : min(Info->sw[3] + 1 - Info->xo, Info->w);

    if (Body)
    {
        while (h--)
        {
            for (int wt = 0; wt < gran; ++wt)
                PixMix(&dst[wt], Color->getcolor1(wt, h), s[wt * 2], Info->mod_blendMode);
            for (int wt = gran; wt < Info->w; ++wt)
                PixMix(&dst[wt], Color->getcolor2(wt, h), s[wt * 2], Info->mod_blendMode);

            s += 2 * Info->overlayp;
            dst = (DWORD*)((char*)dst + Info->pitch);
        }
    }
    else
    {
        s = Info->src;
        while (h--)
        {
            for (int wt = 0; wt < gran; ++wt)
                PixMix(&dst[wt], Color->getcolor1(wt, h), SafeSubstract(s[wt * 2 + 1], s[wt * 2]), Info->mod_blendMode);
            for (int wt = gran; wt < Info->w; ++wt)
                PixMix(&dst[wt], Color->getcolor2(wt, h), SafeSubstract(s[wt * 2 + 1], s[wt * 2]), Info->mod_blendMode);

            s += 2 * Info->overlayp;
            dst = (DWORD*)((char*)dst + Info->pitch);
        }
    }
}

void COverlayMixerSSE2::PixMix(DWORD* dst, DWORD color, BYTE alpha, MOD_BLEND mod_blendMode = BLEND_NORMAL)
{
    BYTE palpha = ((alpha * (color >> 24)) >> 6) & 0xff;
    color &= 0xffffff;

    DWORD blendColor = blendMixColor(dst, &color, mod_blendMode); // TODO: use sse2

    __m128i zero = _mm_setzero_si128();
    __m128i a = _mm_set1_epi32(((palpha + 1) << 16) | (0x100 - palpha));
    __m128i d = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*dst), zero);
    __m128i s = _mm_unpacklo_epi8(_mm_cvtsi32_si128(blendColor), zero);
    __m128i r = _mm_unpacklo_epi16(d, s);

    r = _mm_madd_epi16(r, a);
    r = _mm_srli_epi32(r, 8);
    r = _mm_packs_epi32(r, r);
    r = _mm_packus_epi16(r, r);

    *dst = (DWORD)_mm_cvtsi128_si32(r);
}

DWORD COverlayMixerSSE2::SafeSubstract(DWORD a, DWORD b)
{
    __m128i ap = _mm_cvtsi32_si128(a);
    __m128i bp = _mm_cvtsi32_si128(b);
    __m128i rp = _mm_subs_epu16(ap, bp);

    return (DWORD)_mm_cvtsi128_si32(rp);
}


template<class T> void COverlayAlphaMixer<T>::Draw(bool Body)
{
    int h = Info->h;

    byte* s = Info->s;
    DWORD* dst = Info->dst;
    int gran = (Info->sw[1] == 0xffffffff) ? Info->w : min(Info->sw[3] + 1 - Info->xo, Info->w);

    if (Body)
    {
        while (h--)
        {
            for (int wt = 0; wt < gran; ++wt)
                PixMix(&dst[wt], Color->getcolor1(wt, h), s[wt * 2] * Alpha->getcolor1(wt, h) >> 6, Info->mod_blendMode);
            for (int wt = gran; wt < Info->w; ++wt)
                PixMix(&dst[wt], Color->getcolor2(wt, h), s[wt * 2] * Alpha->getcolor2(wt, h) >> 6, Info->mod_blendMode);

            s += 2 * Info->overlayp;
            dst = (DWORD*)((char*)dst + Info->pitch);
        }
    }
    else
    {
        while (h--)
        {
            for (int wt = 0; wt < gran; ++wt)
                PixMix(&dst[wt], Color->getcolor1(wt, h), SafeSubstract(s[wt * 2 + 1], s[wt * 2]) * Alpha->getcolor1(wt, h) >> 6, Info->mod_blendMode);
            for (int wt = gran; wt < Info->w; ++wt)
                PixMix(&dst[wt], Color->getcolor2(wt, h), SafeSubstract(s[wt * 2 + 1], s[wt * 2]) * Alpha->getcolor2(wt, h) >> 6, Info->mod_blendMode);

            s += 2 * Info->overlayp;
            dst = (DWORD*)((char*)dst + Info->pitch);
        }
    }
}

template void COverlayAlphaMixer<COverlayMixer>::Draw(bool Body);
template void COverlayAlphaMixer<COverlayMixerSSE2>::Draw(bool Body);

#if defined(_VSMOD) && defined(_LUA)
int lua_Error(lua_State* L, CStringA Text)
{
    lua_pushstring(L, Text);
    lua_error(L);

    return 0;
}

// Check binding
void* l_CheckMix(lua_State* L)
{
    lua_getfield(L, 1, "link");
    void* mix = lua_islightuserdata(L, -1) ? lua_touserdata(L, -1) : NULL;
    lua_pop(L, 1);
    if (mix)
        return mix;
    else
        return (void*)lua_Error(L, "Argument #1 is invalid, no valid 'link' field (need table 'renderer')");
}

// Get pixel local pix, color = r:get(x, y)
// 1: renderer
// 2: x
// 3: y
template<class T> int lua_RendererGet(lua_State* L)
{
    if (!lua_istable(L, 1)) return lua_Error(L, "Argument #1 is invalid (need table 'renderer')");
    if (!lua_isnumber(L, 2)) return lua_Error(L, "Argument #2 is invalid (need number 'x')");
    if (!lua_isnumber(L, 3)) return lua_Error(L, "Argument #3 is invalid (need number 'y')");

    COverlayLuaMixer<T>* Mix = (COverlayLuaMixer<T> *)l_CheckMix(L);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);

    int Alpha = 0;
    int Color = 0xff000000;
    if (((x < Mix->Info->w) && (x >= 0)) &&
        ((y < Mix->Info->h) && (y >= 0)))
    {
        Alpha = Mix->Info->s[x * 2 + 2 * Mix->Info->overlayp * (Mix->Info->h - y - 1)];
        Color = *(DWORD*)((char*)&Mix->Info->dst[0] + Mix->Info->pitch * (Mix->Info->h - y - 1));
    }

    lua_pushinteger(L, Alpha);
    lua_pushinteger(L, Color);
    return 2;
}

// Mix pixel
// 1: renderer
// 2: x
// 3: y
// 4: color
template<class T> int lua_RendererMix(lua_State* L)
{
    if (!lua_istable(L, 1)) return lua_Error(L, "Argument #1 is invalid (need table 'renderer')");
    if (!lua_isnumber(L, 2)) return lua_Error(L, "Argument #2 is invalid (need number 'x')");
    if (!lua_isnumber(L, 3)) return lua_Error(L, "Argument #3 is invalid (need number 'y')");
    if (!lua_isnumber(L, 4)) return lua_Error(L, "Argument #4 is invalid (need number 'color')");
    if (!lua_isnumber(L, 5)) return lua_Error(L, "Argument #5 is invalid (need number 'alpha')");

    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    int color = lua_tointeger(L, 4) & 0xFFFFFF;
    int alpha = lua_tointeger(L, 5) & 0xFF;
    COverlayLuaMixer<T>* Mix = (COverlayLuaMixer<T> *)l_CheckMix(L);

    color |= (alpha << 24);

    if (((x < Mix->Info->w) && (x >= 0)) &&
        ((y < Mix->Info->h) && (y >= 0)))
    {
        DWORD* dst = (DWORD*)((char*)&Mix->Info->dst[0] + Mix->Info->pitch * (Mix->Info->h - y - 1));
        byte* src = &Mix->Info->s[2 * Mix->Info->overlayp * (Mix->Info->h - y - 1)];
        byte A = (Mix->m_body) ? src[x * 2] : Mix->SafeSubstract(src[x * 2 + 1], src[x * 2]);

        if (Mix->Alpha) A = (A * Mix->Alpha->getcolor1(x, y)) >> 6;

        if (Mix->m_body)
            Mix->PixMix(&dst[x], color, A, Mix->Info->mod_blendMode);
        else
            Mix->PixMix(&dst[x], color, A, Mix->Info->mod_blendMode);
    }
    return 0;
}

template int lua_RendererGet<COverlayMixer>(lua_State* L);
template int lua_RendererGet<COverlayMixerSSE2>(lua_State* L);

template int lua_RendererMix<COverlayMixer>(lua_State* L);
template int lua_RendererMix<COverlayMixerSSE2>(lua_State* L);

template<class T> void COverlayLuaMixer<T>::Draw(bool Body)
{
    m_body = Body;

    CStringA Func(Function);
    if (LuaHasFunction(L, Function))
    {
        // Find function =D
        lua_getglobal(L, Func);

        // Create line table
        lua_newtable(L);
        LuaAddIntegerField(L, "id", m_entry);
        LuaAddIntegerField(L, "layer", m_layer);
        LuaAddBoolField(L, "body", Body);
        LuaAddIntegerField(L, "height", Info->h);
        LuaAddIntegerField(L, "width", Info->w);
        LuaAddIntegerField(L, "gran", (Info->sw[1] == 0xffffffff) ? Info->w : min(Info->sw[3] + 1 - Info->xo, Info->w));

        // Colors
        LuaAddIntegerField(L, "c1", Info->sw[0] & 0xffffff);
        LuaAddIntegerField(L, "c2", Info->sw[2] & 0xffffff);
        LuaAddIntegerField(L, "a1", (Info->sw[0] >> 24) & 0xff);
        LuaAddIntegerField(L, "a2", (Info->sw[2] >> 24) & 0xff);

        // Saved user data
        {
            CStringA index;
            index.Format("sub_%d", m_entry);
            lua_getglobal(L, index);
            if (lua_istable(L, -1))
                lua_setfield(L, -2, "user");
            else
                lua_pop(L, 1);
        }

        // Create renderer
        lua_newtable(L);
        LuaAddUserField(L, "link", this);
        LuaAddFunctionField(L, "get", &lua_RendererGet<T>);
        LuaAddFunctionField(L, "mix", &lua_RendererMix<T>);

        // function(line, rend)
        if (lua_pcall(L, 2, 0, 0) != 0)
        {
            // error
            CString ErrorText = L"Error: ";
            CString LuaErrorText(lua_tostring(L, -1));

            LuaError(ErrorText + LuaErrorText);
        }
    }
}

template void COverlayLuaMixer<COverlayMixer>::Draw(bool Body);
template void COverlayLuaMixer<COverlayMixerSSE2>::Draw(bool Body);

#endif
