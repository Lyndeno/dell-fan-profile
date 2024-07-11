{
  inputs = {
    nixpkgs = {
      url = "github:NixOS/nixpkgs/nixos-unstable";
    };
  };

  outputs = {self, nixpkgs, ...} @ args: let
    pkgs = nixpkgs.legacyPackages.x86_64-linux;
  in {
    devShells.x86_64-linux.default = let
      kernel = pkgs.linux;
    in pkgs.mkShell {
      buildInputs = with pkgs; [ kernel.dev gnumake bear];
      KERNELRELEASE="${kernel.modDirVersion}";
      KERNEL_DIR="${kernel.dev}/lib/modules/${kernel.modDirVersion}/build";
    };

    packages.x86_64-linux.default = {kernel ? pkgs.linux}: pkgs.stdenv.mkDerivation rec {
      name = "dell-platform-profile";
      src = self;

      nativeBuildInputs = kernel.moduleBuildDependencies;

      makeFlags = [
        "KERNELRELEASE=${kernel.modDirVersion}"
        "KERNEL_DIR=${kernel.dev}/lib/modules/${kernel.modDirVersion}/build"
        "INSTALL_MOD_PATH=$(out)"
      ];

    };
  };
}
