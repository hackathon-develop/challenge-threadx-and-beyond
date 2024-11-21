{
  description = "Flake utils demo";

  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = nixpkgs.legacyPackages.${system}; in
      {
        packages = {
          default = pkgs.callPackage
            ({ stdenvNoCC, cmake, ninja, gcc-arm-embedded }:
              stdenvNoCC.mkDerivation {
                name = "az3166-xxx-firmware";
                nativeBuildInputs = [
                  cmake
                  ninja
                  gcc-arm-embedded
                ];

                src = builtins.path {
                  path = ./.;
                  name = "source";
                };

                dontUseCmakeConfigure = true;
                buildPhase = ''
                  runHook preBuild
                  bash MXChip/AZ3166/scripts/build.sh
                  runHook postBuild
                '';

                installPhase = ''
                  runHook preInstall
                  mkdir --parent -- "$out"
                  cp -- MXChip/AZ3166/build/app/mxchip_threadx.{bin,elf,hex} "$out/"
                  runHook postInstall
                '';
              })
            { };
        };
        apps = {
          flash = flake-utils.lib.mkApp {
            drv = pkgs.writeShellApplication {
              name = "flash";
              runtimeInputs = [ pkgs.udisks ];
              text = ''
                TARGET_DIR="''${TARGET_DIR:="/run/media/$USER/AZ3166/"}"
                if [ ! -d "$TARGET_DIR" ]
                then
                  udisksctl mount \
                    --block-device /dev/disk/by-id/usb-MBED_microcontroller_*
                fi
                cp ${self.packages.${system}.default}/*.bin "$TARGET_DIR"
              '';
            };
          };
        };
      }
    );
}
