# Introduction

This repository contains a few mpv C plugins which have either been written
from scratch or ported from already existing lua scripts.

# mvi.c

C rewrite of [image.lua](https://gist.github.com/haasn/7919afd765e308fa91cbe19a64631d0f) written by [haasn](https://gist.github.com/haasn).
All relevant copyright belongs to him. License Unknown.


# xscrnsaver.c

C Plugin version of mpv's Xscrnsaver functionality which was removed in commit [3f75b3c](https://github.com/mpv-player/mpv/commit/3f75b3c3439241c209349908fa190c0382e44f05).
Prevents screen from blanking during mpv playback using libXScrnSaver.
All relevant copyright belongs to the original authors. (L)GPL 2.0
