# LeptonVid
### FLIR Lepton 3 SPI / GPIO streaming console application

This program is a very simple tool to retrieve frames/pictures from __FLIR Lepton 3__ modules and print them on standard output. The are many tools like this over there but I found many of them to have very big frame syncronization issues that I've not been able to fix.

This application was written for use on __RaspberryPi 0 W__ boards connecting the __Lepton 3 Module__ (with an adapter board) trough _SPI_ port. To achieve the maximum performances and avoid synchronization issues I'm using an extra _GPIO_ pin on which I received an interrupt signal from the __Lepton 3__ module itself (see chapter [4.2.3.3.3 Frame Synchronization](https://www.flir.com/globalassets/imported-assets/document/flir-lepton-engineering-datasheet.pdf)).

As suggested by Luke Van Horn in his project [2] is better to increase SPI device buffer size to increase overall performances (on __RPi0W__ boards)) adding `spidev.buffer=131072` to `/boot/cmdline.txt`

### References

Usefult repositories with __Lepton 3__ module software (running on __Raspberry Pi__):
1. https://github.com/novacoast/Lepton-3-Module 
2. https://github.com/lukevanhorn/Lepton3 
3. [Official __FLIR Lepton 3__ module support](https://www.flir.com/support/products/lepton#Overview)
4. [Official __FLIR Lepton__ SDK](https://lepton.flir.com/sdk/)

Groupgets campaigns:
- https://groupgets.com/manufacturers/flir/products/lepton-3-0
- https://groupgets.com/manufacturers/flir/products/lepton-3-5
