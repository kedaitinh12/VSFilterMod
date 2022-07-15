#pragma once

#include "Rasterizer.h"

// Overlay mixer
class COverlayGetter
{
public:
    COverlayGetter() {}

    virtual DWORD getcolor1(int x, int y) { return 0; }
    virtual DWORD getcolor2(int x, int y) { return 0; }
};

// Solid color
class COverlayColor : public COverlayGetter
{
private:
    DWORD c1, c2;

public:
    COverlayColor(DWORD color1, DWORD color2) { c1 = color1; c2 = color2; }

    DWORD getcolor1(int x, int y) { return c1; }
    DWORD getcolor2(int x, int y) { return c2; }
};

#ifdef _VSMOD
// Gradient Color
class COverlayGradient : public COverlayGetter
{
private:
    MOD_GRADIENT mod_grad;
    int type;

public:
    COverlayGradient(MOD_GRADIENT m, int typ) { mod_grad = m; type = typ; }

    DWORD getcolor1(int x, int y) { return mod_grad.getmixcolor(x, y, type); }
    DWORD getcolor2(int x, int y) { return mod_grad.getmixcolor(x, y, 1); }
};

// Clip mask
class COverlayAlpha : public COverlayGetter
{
private:
    MOD_MOVEVC mod_vc;

public:
    COverlayAlpha(MOD_MOVEVC m) { mod_vc = m; }

    DWORD getcolor1(int x, int y) { return mod_vc.GetAlphaValue(x, y); }
    DWORD getcolor2(int x, int y) { return mod_vc.GetAlphaValue(x, y); }
};
#else
// Clip mask
class COverlayAlpha : public COverlayGetter
{
private:
    byte* mask;
    int    width;
    int    height;

public:
    COverlayAlpha(byte* Mask, int W, int H) { mask = Mask; width = W; height = H; }

    DWORD getcolor1(int x, int y) { return mask[x + (height - y) * width]; }
    DWORD getcolor2(int x, int y) { return mask[x + (height - y) * width]; }
};

#endif

class COverlayMixer
{
protected:
    // Color of layers
    COverlayGetter* Color;

public:
    RasterizerNfo* Info;

    COverlayMixer(RasterizerNfo* Info, COverlayGetter* Color);

    // Mixer
    virtual void PixMix(DWORD* dst, DWORD color, BYTE alpha);
    virtual DWORD SafeSubstract(DWORD a, DWORD b);

    virtual void Draw(bool Body);
};

class COverlayMixerSSE2 : public COverlayMixer
{
protected:

public:
    COverlayMixerSSE2(RasterizerNfo* Info, COverlayGetter* Color) : COverlayMixer(Info, Color) {}

    void PixMix(DWORD* dst, DWORD color, BYTE alpha);
    DWORD SafeSubstract(DWORD a, DWORD b);
};

// Mix with clip
template<class T> class COverlayAlphaMixer : public T
{
private:
    // Alpha of layers
    COverlayGetter* Alpha;

public:
    COverlayAlphaMixer(RasterizerNfo* Info, COverlayGetter* Color, COverlayGetter* Alpha)
        : T(Info, Color) {
        this->Alpha = Alpha;
    }

    void Draw(bool Body);
};

#if defined(_VSMOD) && defined(_LUA)
// Error
int lua_Error(lua_State* L, CStringA Text);

// Check binding
void* l_CheckMix(lua_State* L);

template<class T> int lua_RendererGet(lua_State* L);
template<class T> int lua_RendererMix(lua_State* L);

// Mix with clip
template<class T> class COverlayLuaMixer : public T, public CMyLua
{
public:
    // Custom function name
    CString Function;
    int     m_entry;
    int     m_layer;

    // Alpha of layers
    COverlayGetter* Alpha;
    bool    m_body;

    COverlayLuaMixer(RasterizerNfo* Info, COverlayGetter* Color, COverlayGetter* Alpha)
        : T(Info, Color) {
        this->Alpha = Alpha;
    }

    void Draw(bool Body);
};

#endif
