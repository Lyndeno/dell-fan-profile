{
  inputs = {
    nixpkgs = {
      url = "github:NixOS/nixpkgs/nixos-unstable";
    };
  };

  outputs = {nixpkgs, ...} @ args: let
    pkgs = nixpkgs.legacyPackages.x86_64-linux;
    kernel = pkgs.linux;
  in {
    devShells.x86_64-linux.default = pkgs.mkShell {
      buildInputs = with pkgs; [ kernel.dev gnumake bear];
      KERNELRELEASE="${kernel.modDirVersion}";
      KERNEL_DIR="${kernel.dev}/lib/modules/${kernel.modDirVersion}/build";
    };
  };
}
