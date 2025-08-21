{ pkgs ? import <nixpkgs> {}}:

pkgs.mkShell {
  packages = with pkgs; [ 
    hello 
    # gcc-arm-embedded
    # cmake
    # gnumake
    # git
    # openocd
    # npm
    platformio
    nodejs_24
    cmake
  ];
}