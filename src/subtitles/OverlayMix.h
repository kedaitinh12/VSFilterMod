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

public:
    COverlayGradient(MOD_GRADIENT m) { mod_grad = m; }

    DWORD getcolor1(int x, int y) { return mod_grad.getmixcolor(x, y, 0); }
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
    byte * mask;
    int    width;
    int    height;

public:
    COverlayAlpha(byte * Mask, int W, int H) { mask = Mask; width = W; height = H; }

    DWORD getcolor1(int x, int y) { return mask[x + (height - y) * width]; }
    DWORD getcolor2(int x, int y) { return mask[x + (height - y) * width]; }
};

#endif

class COverlayMixer
{
protected:
    RasterizerNfo * Info;

    // Color of layers
    COverlayGetter * Color;

    // Mixer
    const static void PixMix(DWORD * dst, DWORD color, BYTE alpha);
    const static DWORD SafeSubstract(DWORD a, DWORD b);

public:
    COverlayMixer(RasterizerNfo * Info, COverlayGetter * Color);
    
    virtual void Draw(bool Body);
};

class COverlayMixerSSE2 : public COverlayMixer
{
protected:
    const static void PixMix(DWORD * dst, DWORD color, BYTE alpha);
    const static DWORD SafeSubstract(DWORD a, DWORD b);

public:
    COverlayMixerSSE2(RasterizerNfo * Info, COverlayGetter * Color) : COverlayMixer(Info, Color) {}
};

// Mix with clip
template<class T> class COverlayAlphaMixer : public T
{
private:
    // Alpha of layers
    COverlayGetter * Alpha;

public:
    COverlayAlphaMixer(RasterizerNfo * Info, COverlayGetter * Color, COverlayGetter * Alpha)
        : T(Info, Color) { this->Alpha = Alpha; }

    void Draw(bool Body)
    {
        int h = Info->h;

        byte* s = Info->s;
        DWORD* dst = Info->dst;
        int gran = (Info->sw[1] == 0xffffffff) ? Info->w : min(Info->sw[3] + 1 - Info->xo, Info->w);

        if(Body)
        {
            while(h--)
            {
                for(int wt = 0; wt < gran; ++wt)
                    PixMix(&dst[wt], Color->getcolor1(wt, h), s[wt*2] * Alpha->getcolor1(wt, h) >> 6);
                for(int wt = gran; wt < Info->w; ++wt)
                    PixMix(&dst[wt], Color->getcolor2(wt, h), s[wt*2] * Alpha->getcolor2(wt, h) >> 6);

                s += 2 * Info->overlayp;
                dst = (DWORD*)((char *)dst + Info->pitch);
            }
        }
        else
        {
            while(h--)
            {
                for(int wt = 0; wt < gran; ++wt)
                    PixMix(&dst[wt], Color->getcolor1(wt, h), SafeSubstract(s[wt*2+1], s[wt*2]) * Alpha->getcolor1(wt, h) >> 6);
                for(int wt = gran; wt < Info->w; ++wt)
                    PixMix(&dst[wt], Color->getcolor2(wt, h), SafeSubstract(s[wt*2+1], s[wt*2]) * Alpha->getcolor2(wt, h) >> 6);

                s += 2 * Info->overlayp;
                dst = (DWORD*)((char *)dst + Info->pitch);
            }
        }
    }
};