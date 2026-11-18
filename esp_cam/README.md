# ESP CAM

## Image resolution
The esp cam (OV5640) has a native 4:3 aspect ratio. Therefore, these are the resolution alternatives that followe the 4:3 aspect ratio:

|Constant | Size | Megapixels|
| --- | --- | --- |
|FRAMESIZE_QQVGA | 160x120 | 0.02 |
|FRAMESIZE_QVGA | 320x240 | 0.08 |
|FRAMESIZE_VGA | 640x480 | 0.31 |
|FRAMESIZE_SVGA | 800x600 | 0.48 |
|FRAMESIZE_XGA | 1024x768 | 0.79 |
|FRAMESIZE_UXGA | 1600x1200 | 1.92 |
|FRAMESIZE_QXGA | 2048x1536 | 3.15 |
|FRAMESIZE_QSXGA | 2592x1944 | 5.04 | 

One can also change the `kJpegQuality`; however, that is somewhat of a black box. I would recommend to keep it as the standard `12`.

If one want to change it, lower `kJpegQuality` number equals better quality.

## Programs

### simple_image

This program takes a photo and then goes to sleep. As we probabilly want.

### Spotlight

A class for the spothlight and a simple code example.

### Webbapp

Contains two scripts. Both starts a webb-aplication on local host, where: `video` sends a livestream on the local host, and `images` sends images to local host which updates each (around) 15 seconds.

Both these scripts uses Waveshares code example.



