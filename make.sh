rm -f LeptonVid.o LeptonVid
cd leptonSDKEmb32PUB
make
cd ..
gcc -O2 -Wall -W -c LeptonVid.c -o LeptonVid.o
gcc -W -O1 LeptonVid.o -LleptonSDKEmb32PUB -lLEPTON_SDK -lwiringPi -o LeptonVid
