{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShellNoCC {
  env.CMAKE_EXPORT_COMPILE_COMMANDS = "true";

  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    gcc-arm-embedded
  ];

  shellHook = ''
    build(){
      ./MXChip/AZ3166/scripts/build.sh
    }

    flash(){
      udisksctl mount --block-device /dev/disk/by-id/usb-MBED_microcontroller_*
      cp -- ./MXChip/AZ3166/build/app/mxchip_threadx.bin /run/media/$USER/AZ3166/
    }
  '';
}
