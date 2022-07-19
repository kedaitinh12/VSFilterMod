A modification of VSFilterMod by Vmoe Fansub

New override tags: [wiki page](https://github.com/computerfan/VSFilterMod/wiki)

Important note: **DO NOT use mod tags in a softsub (ASS file) distribution**, because most players just ignore them, and they may not efficient enough for real-time rendering.

Base branches:
* Masaiki/VSFilterMod: r5.2.7
* sorayuki/VSFilterMod: r5.2.6
* teplofizik/VSFilterMod: @3651264 (Aug 21, 2014)

Usage
=====
## Aegisub
Decompress VSFilterMod.dll to Aegisub\csri directory. Open Options dialoge in Aegisub, then enter Advanced -> Video option and change Subtitles provider to csri/vsfiltermod_textsub.

## Vapousynth
    vsfm.TextSubMod(clip clip, string file[, int charset=1, float fps=-1.0, string vfr='', int accurate=0])
    vsfm.VobSub(clip clip, string file)

* clip: Clip to process. Only YUV420P8, YUV420P10, YUV420P16 and RGB24 are supported.
* accurate: 1: enable accurate render for 10/16bit (~2x slower). / 0: disable (default)

## MPC-BE
1. run `regsvr32.exe VSFilterMod.dll` with administrator privileges
2. Select "VSFilter/xy-VSFilter" on the select of Options/Subtitles/Subtitle renderer

Knowing Issues
=====
* Opentype font (such as Source Han Sans) has a much smaller size when displayed vertically (used like @Source Han Sans). (subtitle renders which origin from VSFilter use GDI to render fonts, but GDI performs badly on opentype fonts.)
