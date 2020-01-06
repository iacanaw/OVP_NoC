#!/bin/sh
cd peripheral
rm -rf pse.pse
cd ..
# Check Installation supports this example
checkinstall.exe -p install.pkg --nobanner || exit
CROSS=OR1K
make -C application CROSS=${CROSS}
make -C module
make -C peripheral/whnoc_dma NOVLNV=1
make -C peripheral/synchronizer NOVLNV=1
make -C peripheral/iterator NOVLNV=1
make -C peripheral/networkInterface NOVLNV=1
make -C harness
harness/harness.${IMPERAS_ARCH}.exe \
     --program cpu0=application/application0.${CROSS}.elf $* \
     --program cpu1=application/application1.${CROSS}.elf $* \
     --program cpu2=application/application2.${CROSS}.elf $* \
     --program cpu3=application/application3.${CROSS}.elf $* \
     --program cpu4=application/application4.${CROSS}.elf $* \
     --program cpu5=application/application5.${CROSS}.elf $* \
     --program cpu6=application/application6.${CROSS}.elf $* \
     --program cpu7=application/application7.${CROSS}.elf $* \
     --program cpu8=application/application8.${CROSS}.elf $* \
     --program cpu9=application/application9.${CROSS}.elf $* \
     --program cpu10=application/application10.${CROSS}.elf $* \
     --program cpu11=application/application11.${CROSS}.elf $* \
     --program cpu12=application/application12.${CROSS}.elf $* \
     --program cpu13=application/application13.${CROSS}.elf $* \
     --program cpu14=application/application14.${CROSS}.elf $* \
     --program cpu15=application/application15.${CROSS}.elf $* \
     --program cpu16=application/application16.${CROSS}.elf $* \
     --program cpu17=application/application17.${CROSS}.elf $* \
     --program cpu18=application/application18.${CROSS}.elf $* \
     --program cpu19=application/application19.${CROSS}.elf $* \
     --program cpu20=application/application20.${CROSS}.elf $* \
     --program cpu21=application/application21.${CROSS}.elf $* \
     --program cpu22=application/application22.${CROSS}.elf $* \
     --program cpu23=application/application23.${CROSS}.elf $* \
     --program cpu24=application/application24.${CROSS}.elf $* \
     --program cpu25=application/application25.${CROSS}.elf $* \
     --program cpu26=application/application26.${CROSS}.elf $* \
     --program cpu27=application/application27.${CROSS}.elf $* \
     --program cpu28=application/application28.${CROSS}.elf $* \
     --program cpu29=application/application29.${CROSS}.elf $* \
     --program cpu30=application/application30.${CROSS}.elf $* \
     --program cpu31=application/application31.${CROSS}.elf $* \
     --program cpu32=application/application32.${CROSS}.elf $* \
     --program cpu33=application/application33.${CROSS}.elf $* \
     --program cpu34=application/application34.${CROSS}.elf $* \
     --program cpu35=application/application35.${CROSS}.elf --imperasintercepts $* \
$*
