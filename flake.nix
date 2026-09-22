{
  description = "Functional, bytecode interpreted language written in C";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

    # Externally extensible flake systems. See <https://github.com/nix-systems/nix-systems>.
    systems.url = "github:nix-systems/default";
  };

  outputs = { self, systems, nixpkgs, ... }:
    let
      # Nixpkgs library functions.
      lib = nixpkgs.lib;

      # Iterate over each system, configured via the `systems` input.
      eachSystem = lib.genAttrs (import systems);

      llvmPkgs' = pkgs: pkgs.llvmPackages_22;
      clangStdenv' = pkgs: llvmPkgs:
        if pkgs.stdenv.targetPlatform.isDarwin
        then llvmPkgs.stdenv
        else pkgs.stdenvAdapters.useWildLinker llvmPkgs.stdenv;
    in
    {
      packages = eachSystem (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          llvmPkgs = llvmPkgs' pkgs;
          clangStdenv = clangStdenv' pkgs llvmPkgs;
          commonDrvOpts = pname: {
            inherit pname;
            version = "0.1.0";
            src = ./.;
            outputs = [ "out" ];
            nativeBuildInputs = with pkgs; [ meson ninja ];
            meta = {
              mainProgram = "clam";
              homepage = "https://github.com/jawadcode/clam";
              license = [ pkgs.lib.licenses.mit ];
              maintainers = [ "jawadcode" ];
            };
          };
        in
        rec {
          clam-debug = clangStdenv.mkDerivation (commonDrvOpts "clam-debug" // {
            mesonBuildType = "debugoptimized";
            # # Without this we get a _FORTIFY_SOURCE related compiler warning from
            # # clang, so we need to disable it for debug builds, for a relevant GH
            # # issue, see: https://github.com/NixOS/nixpkgs/issues/60919
            # hardeningDisable = [ "fortify" ];
            mesonFlags = [ "-Db_sanitize=address,undefined" ];
            enableParallelBuilding = true;
          });
          clam = clangStdenv.mkDerivation (commonDrvOpts "clam" // {
            mesonBuildType = "release";
            mesonFlags = [ "-Dstrip=true" ];
          });
          default = clam;
        }
      );
      devShells = eachSystem (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          llvmPkgs = llvmPkgs' pkgs;
          clangStdenv = clangStdenv' pkgs llvmPkgs;
        in
        {
          default = pkgs.mkShell.override { stdenv = clangStdenv; } {
            inputsFrom = lib.attrValues self.packages.${system};
            nativeBuildInputs = [ llvmPkgs.clang-tools ];
            packages = with pkgs; [ llvmPkgs.bintools llvmPkgs.lldb meson ninja clang-analyzer mesonlsp ];
          };
        });
    };
}
