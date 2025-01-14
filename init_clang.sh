#!/bin/bash
check_and_install() {
  if ! command -v "$1" &> /dev/null; then
    echo "Error: $1 not found! Installing..."
    sudo apt update
    sudo apt install -y "$2"
  else
    echo "$1: yes"
  fi
}
if [ -d "toolchain" ]; then
  echo "Toolchain already exists. Skipping download."
else
  echo "Toolchain not found. Downloading and setting up..."
  mkdir -p toolchain
  cd toolchain
  echo 'Checking for system requirements...'
  check_and_install "zstd" "zstd"
  check_and_install "bsdtar" "libarchive-tools"
  check_and_install "wget" "wget"
  check_and_install "cpio" "cpio"
  check_and_install "flex" "flex"
  check_and_install "bc" "bc"
  echo 'Download antman and sync'
  bash <(curl -s "https://raw.githubusercontent.com/Neutron-Toolchains/antman/main/antman") -S
  echo 'Patch for glibc'
  bash <(curl -s "https://raw.githubusercontent.com/Neutron-Toolchains/antman/main/antman") --patch=glibc
  cd ..
  echo 'Done'
fi
