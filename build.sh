#!/bin/bash
# Compile script for lineage kernel
# Copyright (C) 2020-2021 Adithya R. | (C) 2024 - Shoiya Akari.

SECONDS=0
KBUILD_USER='Thereache'
KBUILD_HOST='Trr-labs'
CORES=$(nproc --all)
KERNEL_DIR=$(pwd)
TC_DIR="$KERNEL_DIR/toolchain"
AK3_DIR="$KERNEL_DIR/AnyKernel3"
DEFCONFIG="vendor/lahaina-qgki_defconfig vendor/debugfs.config vendor/xiaomi_QGKI.config vendor/redwood_QGKI.config"
ZIPNAME="lineage-redwood-$(date '+%Y%m%d-%H%M').zip"

# Here is clang
bash init_clang.sh
MAKE_PARAMS="O=out ARCH=arm64 CC=clang LLVM=1 LLVM_IAS=1 \
                CROSS_COMPILE=$TC_DIR/bin/aarch64-linux-gnu- CROSS_COMPILE_ARM32=$TC_DIR/bin/arm-linux-gnueabi- LD=ld.lld 2>&1 \
                TARGET_PRODUCT=redwood"
export PATH="$TC_DIR/bin:$PATH"
if [[ $1 = "-r" || $1 = "--regen" ]]; then
	make $MAKE_PARAMS $DEFCONFIG savedefconfig
	cp out/defconfig arch/arm64/configs/$DEFCONFIG
	echo -e "\nSuccessfully regenerated defconfig at $DEFCONFIG"
	exit
fi
if [[ $1 = "-c" || $1 = "--clean" ]]; then
	rm -rf out
	echo "Cleaned output folder"
fi
mkdir -p out
make $MAKE_PARAMS $DEFCONFIG
echo -e "\nStarting compilation...\n"
make -j$(nproc --all) $MAKE_PARAMS || exit $?
make -j$(nproc --all) $MAKE_PARAMS INSTALL_MOD_PATH=modules INSTALL_MOD_STRIP=1 modules_install
kernel="out/arch/arm64/boot/Image"
dtb="out/arch/arm64/boot/dts/vendor/qcom/yupik.dtb"
dtbo="out/arch/arm64/boot/dts/vendor/qcom/redwood-sm7325-overlay.dtbo"
if [ ! -f "$kernel" ] || [ ! -f "$dtb" ] || [ ! -f "$dtbo" ]; then
	echo -e "\nCompilation failed!"
	exit 1
fi
echo -e "\nKernel compiled succesfully! Zipping up...\n"
if [ -d "$AK3_DIR" ]; then
	cp -r $AK3_DIR AnyKernel3
	git -C AnyKernel3 checkout redwood &> /dev/null
elif ! git clone -q https://github.com/Thereache/AnyKernel3 -b 5.4; then
	echo -e "\nAnyKernel3 repo not found locally and couldn't clone from GitHub! Aborting..."
	exit 1
fi
cp $kernel AnyKernel3
cp $dtb AnyKernel3/dtb
python2 scripts/dtc/libfdt/mkdtboimg.py create AnyKernel3/dtbo.img --page_size=4096 $dtbo
cp $(find out/modules/lib/modules/5.4* -name '*.ko') AnyKernel3/modules/vendor/lib/modules/
cp out/modules/lib/modules/5.4*/modules.{alias,dep,softdep} AnyKernel3/modules/vendor/lib/modules
cp out/modules/lib/modules/5.4*/modules.order AnyKernel3/modules/vendor/lib/modules/modules.load
sed -i 's/\(kernel\/[^: ]*\/\)\([^: ]*\.ko\)/\/vendor\/lib\/modules\/\2/g' AnyKernel3/modules/vendor/lib/modules/modules.dep
sed -i 's/.*\///g' AnyKernel3/modules/vendor/lib/modules/modules.load
rm -rf out/arch/arm64/boot out/modules
cd AnyKernel3
zip -r9 "../$ZIPNAME" * -x .git README.md *placeholder
cd ..
rm -rf AnyKernel3
echo -e "\nCompleted in $((SECONDS / 60)) minute(s) and $((SECONDS % 60)) second(s) !"
echo "Zip: $ZIPNAME"
