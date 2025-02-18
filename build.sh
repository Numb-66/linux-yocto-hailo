#!/bin/bash

SCRIPT_PATH=$(realpath $(dirname "$0"))

if [ -f "${SCRIPT_PATH}/build.cfg" ]; then
    source "${SCRIPT_PATH}/build.cfg"
else
    source "${SCRIPT_PATH}/build.cfg.default"
fi

source ${TOOLCHAIN}

MAKE="make ARCH=arm64 CROSS_COMPILE=aarch64-poky-linux- -j16"

extract_machine() {
	if [ -f "${DEPLOY_DIR}/machine_name.txt" ]; then
        cat "${DEPLOY_DIR}/machine_name.txt"
        return
    fi
    local filename=$(find ${DEPLOY_DIR}/ -type l -name 'u-boot-*.dtb' -printf %P)
    local machine_name=${filename#u-boot-}
    echo ${machine_name%.dtb}
}

if [ -z "$MACHINE" ]; then
	MACHINE=$(extract_machine)
fi

# Require success of commands
set -e

# Function to align a file by adding padding to make its size a multiple of `pad_size`
align_file() {
    binary_file="$1"
    pad_size="$2"

    # Get the current size of the file
    current_size=$(stat --format=%s "$binary_file")

    # Calculate padding needed to make the file size a multiple of `pad_size`
    padding_needed=$(expr $pad_size - \( $current_size % $pad_size \))

    # If no padding is needed (already aligned), exit early
    if [ "$padding_needed" -eq "$pad_size" ]; then
        return
    fi

    # Use dd to add zero padding (or any other byte pattern) to the file
    dd if=/dev/zero bs=1 count="$padding_needed" >> "$binary_file"
}

make_install_module() {
	module_path=$1
	$MAKE M=$module_path
	if [ "${EXPORT_MODULES_TO_NFS}" = "yes" ]; then
		sudo make M=$module_path INSTALL_MOD_PATH=${NFS_DIR} modules_install
	fi
}

make_all(){
	if [ ! -f .config ]; then
		if [ -f "${DEPLOY_DIR}/kernel.config" ]; then
			cp "${DEPLOY_DIR}/kernel.config" ${SCRIPT_PATH}/.config
		else
			echo ".config file not found in current directory or deploy directory. Exiting."
			exit 1
		fi
	fi
	$MAKE DTC_FLAGS=-@
	cp ${DEPLOY_DIR}/fitImage-its-${MACHINE} fit-image.its
	if grep -q "gzip" fit-image.its; then
		cp arch/arm64/boot/Image.gz linux.bin
	else
		cp arch/arm64/boot/Image linux.bin
	fi
	align_file linux.bin 64
	cp ${DEPLOY_DIR}/bl31.bin .
	align_file bl31.bin 64
	align_file arch/arm64/boot/dts/hailo/${MACHINE}.dtb 64
	uboot-mkimage -f fit-image.its fitImage
	uboot-mkimage -E -B 0x40 -F -k ${DEPLOY_DIR} -r fitImage
	cp fitImage ${DEPLOY_DIR}
	cp vmlinux ${DEPLOY_DIR}
	if [ "${EXPORT_MODULES_TO_NFS}" = "yes" ]; then
		sudo INSTALL_MOD_PATH=${NFS_DIR} $MAKE modules_install
	fi
	if [ -n "${HAILORT_DIR}" ]; then
		make_install_module ${HAILORT_DIR}
	fi
	if [ -n "${ENCODER_DIR}" ]; then
		make_install_module ${ENCODER_DIR}
	fi
}

if [ $# -eq 0 ]
	then
		make_all
	else
		$MAKE $1
fi
