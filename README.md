# LeptonVid
Lepton 3 SPI / GPIO streaming console application

This program is a very simple tool to retrieve frames/pictures from Lepton 3 Modules and print them on standard output. The are many tools like this over there but I found them to have very big timing issues that I've not been able to fix.
this application was written for use on RaspberryPi 0 W boards connecting the Lepton 3 Module (with an adapter board) trough SPI port. To achieve the maximum performances and avoid synchronization issues I'm using an extra GPIO pin on which I received an interrupt signal from the Lepton 3 camera itself (see chapter 4.2.3.3.3 https://www.flir.com/globalassets/imported-assets/document/flir-lepton-engineering-datasheet.pdf).

As suggested by Luke Van Horn in his project is better to increase SPI device buffer size to increase overall performances (on RPi0W boards)) adding ```spidev.buffer=131072``` to ```/boot/cmdline.txt```

References:

Usefult repositories with Lepton 3 Camera software (running on Raspberry Pi)
https://github.com/novacoast/Lepton-3-Module
https://github.com/lukevanhorn/Lepton3

Official FLIR Lepton 3 support resource page
https://www.flir.com/support/products/lepton#Resources
