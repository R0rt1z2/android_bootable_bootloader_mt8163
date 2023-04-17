# Bootloader for MT8163 based devices
This repository contains the source code for the bootloader (LK) of MT8163 devices. To build the bootloader, please follow the instructions below.

## Dependencies
Before building the bootloader, you need to install the following dependencies:
```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install libc6:i386 libstdc++6:i386 zlib1g:i386
```

## Building the Bootloader
To build the bootloader, follow the steps below:
1. Clone the repository to your local machine.
2. Navigate to the repository directory.
3. Run the build script with the target device name as a parameter (`./build.sh <target>`).
