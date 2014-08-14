
#include "stdafx.h"
#include "OverlayMix.h"
#include <math.h>

// COverlayMixer class
COverlayMixer::COverlayMixer(RasterizerNfo * Info, COverlayGetter * Color)
{
    this->Info = Info;
    this->Color = Color;
}

const void COverlayMixer::PixMix(DWORD * dst, DWORD color, BYTE alpha)
{
    DWORD a = ((alpha * (color >> 24)) >> 6) & 0xff;
    DWORD ia = 256 - a;
    a += 1;

    *dst = ((((*dst & 0x00ff00ff) * ia + (color & 0x00ff00ff) * a) & 0xff00ff00) >> 8)
           | ((((*dst & 0x0000ff00) * ia + (color & 0x0000ff00) * a) & 0x00ff0000) >> 8)
           | ((((*dst >> 8) & 0x00ff0000) * ia) & 0xff000000);
}

const DWORD COverlayMixer::SafeSubstract(DWORD a, DWORD b)
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

    if(Body)
    {
        while(h--)
        {
            for(int wt = 0; wt < gran; ++wt)
                PixMix(&dst[wt], Color->getcolor1(wt, h), s[wt*2]);
            for(int wt = gran; wt < Info->w; ++wt)
                PixMix(&dst[wt], Color->getcolor2(wt, h), s[wt*2]);

            s += 2 * Info->overlayp;
            dst = (DWORD*)((char *)dst + Info->pitch);
        }
    }
    else
    {
        while(h--)
        {
            for(int wt = 0; wt < gran; ++wt)
                PixMix(&dst[wt], Color->getcolor1(wt, h), SafeSubstract(s[wt*2+1], s[wt*2]));
            for(int wt = gran; wt < Info->w; ++wt)
                PixMix(&dst[wt], Color->getcolor2(wt, h), SafeSubstract(s[wt*2+1], s[wt*2]));

            s += 2 * Info->overlayp;
            dst = (DWORD*)((char *)dst + Info->pitch);
        }
    }
}

const void COverlayMixerSSE2::PixMix(DWORD * dst, DWORD color, BYTE alpha)
{
    alpha = ((alpha * (color >> 24)) >> 6) & 0xff;
    color &= 0xffffff;

    __m128i zero = _mm_setzero_si128();
    __m128i a = _mm_set1_epi32(((alpha + 1) << 16) | (0x100 - alpha));
    __m128i d = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*dst), zero);
    __m128i s = _mm_unpacklo_epi8(_mm_cvtsi32_si128(color), zero);
    __m128i r = _mm_unpacklo_epi16(d, s);

    r = _mm_madd_epi16(r, a);
    r = _mm_srli_epi32(r, 8);
    r = _mm_packs_epi32(r, r);
    r = _mm_packus_epi16(r, r);

    *dst = (DWORD)_mm_cvtsi128_si32(r);
}

const DWORD COverlayMixerSSE2::SafeSubstract(DWORD a, DWORD b)
{
    __m128i ap = _mm_cvtsi32_si128(a);
    __m128i bp = _mm_cvtsi32_si128(b);
    __m128i rp = _mm_subs_epu16(ap, bp);

    return (DWORD)_mm_cvtsi128_si32(rp);
}
