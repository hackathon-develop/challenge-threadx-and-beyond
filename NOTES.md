# Architecture

# Building and Running

## Vanilla Linux

```bash
# compile everything
./MXChip/AZ3166/scripts/build.sh

# mount the virtual storage device that actually flashes bin files to the MCU
[ -d /run/media/$USER/AZ3166/ ] || udisksctl mount --block-device /dev/disk/by-id/usb-MBED_microcontroller_*

# copy over the built binary to the MCU
cp -- ./MXChip/AZ3166/build/app/mxchip_threadx.bin /run/media/$USER/AZ3166/
```

## Nix

Make sure you have Nix installed, with both `flakes` and `nix-command` enabled in the `experimental-features`. Refer to `man 5 nix.conf` for details on how to achieve that.
The [Determinate Systems Installer](https://github.com/DeterminateSystems/nix-installer) is a handy tool which directly installs Nix and activates the aforementioned experimental features.

Then simply run:

```bash
nix run '.?submodules=1#'flash
```

# Datasheets for the harwdare

- HTS221: Humidity sensor
  - https://www.st.com/en/mems-and-sensors/hts221.html
- LIS2MDL: Magnetometer
  - https://www.st.com/en/mems-and-sensors/lis2mdl.html
- LPS22HB: Pressure Sensor
  - https://www.st.com/en/mems-and-sensors/lps22hb.html
- LSM6DSL: IMU
  - https://www.st.com/en/mems-and-sensors/lsm6dsl.html
