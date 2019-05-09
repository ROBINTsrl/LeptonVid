# LeptonVid
### FLIR Lepton 3 SPI / GPIO streaming console application

This program is a very simple tool to retrieve frames/pictures from __FLIR Lepton 3__ modules and print them on standard output. 

There are many tools like this over there but I found many of them to have very big frame syncronization issues that I've not been able to fix.

This application was written for use on __RaspberryPi 0 W__ boards connecting the __Lepton 3 Module__ (with an adapter board) trough _SPI_ port. To achieve the maximum performances and avoid synchronization issues I'm using an extra _GPIO_ pin on which I received an interrupt signal from the __Lepton 3__ module itself (see chapter [4.2.3.3.3 Frame Synchronization](https://www.flir.com/globalassets/imported-assets/document/flir-lepton-engineering-datasheet.pdf)).

As suggested by Luke van Horn in [his project](https://github.com/lukevanhorn/Lepton3) is better to increase SPI device buffer size to increase overall performances (on __RPi0W__ boards)) adding `spidev.buffer=131072` to `/boot/cmdline.txt`

### Usage

Once compiled the tool act as a typical _Unix_ console application; typing `LeptonVid -?` will print out the usage:
```
[-f frames] 
[-F frames timeout] 
[-D startup delay] 
[-p port] 
[-s bitrate] 
[-i strip frame delimiters] 
[-r reset Lepton on startup]
[-w gpio pin]
```

__frames__: is the number of frames the application will wait from module, please note that only valid frames are taken into account. Omitting this option will cause LeptonVid to run forever until interrupted.

__frames timeout__: allows to specify the timeout time in microseconds of the ioctl read function (default value is 65535 microseconds).

__startup delay__: time to wait before interacting with camera module in seconds. Please note that official documentation states that the internal module initialization and calibration procedure will take not less than 5 seconds (default value is set to 5 seconds).

__port__: allows to specify which device to use (default value is `/dev/spidev0.0`).

__bitrate__: SPI bus port bitrate in _Hz_ (default value is set to 16MHz).

__strip frame delimiters__: when specified this option remove frame delimiters (i.e. `F`, `EF` tags printed before and after the frame body).

__reset on startup__: using this option will force __Lepton module__ to be reset on application startup.

__gpio pin__: using this option to specifiy/change the __Raspebbery Pi__ pin used to input the _VSYNC_ signal coming from __Lepton module__ please note that we are using [WiringPi](http://wiringpi.com/)  pin enumeration standard (default value is set to 6)

### References

Usefult repositories with __Lepton 3__ module software (running on __Raspberry Pi__):
1. https://github.com/novacoast/Lepton-3-Module 
2. https://github.com/lukevanhorn/Lepton3 
3. [Official __FLIR Lepton 3__ module support](https://www.flir.com/support/products/lepton#Overview)
4. [Official __FLIR Lepton__ SDK](https://lepton.flir.com/sdk/)

Groupgets campaigns:
- https://groupgets.com/manufacturers/flir/products/lepton-3-0
- https://groupgets.com/manufacturers/flir/products/lepton-3-5
