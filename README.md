# QtMultimedia GPhoto plugin
This plugin adds support for Qt Multimedia classes working with camera (QCamera or QML Camera item) to access the generic photo cameras supported by [gphoto2 library](http://www.gphoto.org/).

Plugin surely must be considered experimental and supports only the basic gphoto2 functionality. It was tested on Ubuntu 13.10 and Qt 5.3 with Canon EOS 550D mirrored camera and proved itself to work nicely for a two-day proof-of-concept. I expect every camera [supporting image capture and liveview functionality](http://www.gphoto.org/proj/libgphoto2/support.php) via gphoto2 library to work.

The code was mainly inspired by QNX/BlackBerry QtMultimedia plugin contained in Qt sources.

More than one camera support haven't been tested and probably wouldn't work.

## Installation
```sh
qmake
make
make install
```

## Usage
After installing the plugin you may access your camera using any QtMultimedia camera app (for example, you may use camera example provided with Qt itself).

Note that since most cameras doesn't support sending orientation sensor data via PTP you will need to rotate the preview and captured images yourself when using camera not in portrait orientation. You can rotate viewfinder preview using the orientation property supported by QML VideoOutput item.

## License
[LGPL 2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)  Copyright © 2014 Boris Moiseev

> This software depends on Qt 5 and libgphoto2 libraries using LGPL 2.1 license. Feel free to contact me if you're interested in using this library on other terms.

