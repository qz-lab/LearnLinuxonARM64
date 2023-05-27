#
OUTPUT=../output

HW_OPT="-machine virt -cpu cortex-a53 -smp 2 -m 1024"

# load Linux Kernel
BOOT_OPT="-kernel ${PWD}/../linux/output/arch/arm64/boot/Image	\
	-drive file=disk.img,format=raw								\
	-append root=/dev/vda1 rootfstype=ext2 rw"

# load bare-metal binary
BOOT_OPT="-device loader,file=${OUTPUT}/bare-metal.bin,addr=0x8000,cpu-num=0"

# Whether GDB connection is enabled since the beginning
if [ "$1" == "debug" ]; then
	DEBUG_OPT="-S -s"
else
	DEBUG_OPT=""
fi


echo $DEBUG_OPT
set -e
#
# build the tareget first
make O=${OUTPUT} KBUILD_AFLAGS="-g" KBUILD_CFLAGS="-g -fno-stack-protector"

# start QEMU in subshell
$(qemu-system-aarch64 ${HW_OPT} ${BOOT_OPT} ${DEBUG_OPT})
	
# launch gdb
#ddd --debugger arch64-unknown-linux-gnu-gdb ${OUTPUT}/bare-metal -ex "target remote localhost:1234"

