{
  description = "Functional, bytecode interpreted language written in C";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    # Externally extensible flake systems. See <https://github.com/nix-systems/nix-systems>.
    systems.url = "github:nix-systems/default";
  };

  outputs = {
    self,
    systems,
    nixpkgs,
    ...
  }: let
    # Nixpkgs library functions.
    lib = nixpkgs.lib;

    # Iterate over each system, configured via the `systems` input.
    eachSystem = lib.genAttrs (import systems);
  in {
    packages = eachSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in rec {
      clam-debug = pkgs.clang18Stdenv.mkDerivation rec {
        pname = "clam";
        version = "0.1.0";
        src = ./.;
        outputs = ["out" "dev"];

        nativeBuildInputs = [pkgs.meson pkgs.ninja];
        mesonBuildType = "debug";
        # Without this we get a _FORTIFY_SOURCE related compiler warning from
        # clang, so we need to disable it for debug builds, for a relevant GH
        # issue, see: https://github.com/NixOS/nixpkgs/issues/60919
        hardeningDisable = ["fortify"];
        mesonFlags = ["-Db_sanitize=address,undefined"];
        enableParallelBuilding = true;

        installPhase = ''
          mkdir -p $out/bin/
          cp ${pname} $out/bin/
        '';

        meta = {
          homepage = "https://github.com/jawadcode/clam";
          license = [pkgs.lib.licenses.mit];
          maintainers = ["Jawad W. Ahmed"];
        };
      };
      clam-release = pkgs.clang18Stdenv.mkDerivation rec {
        pname = "clam";
        version = "0.1.0";
        src = ./.;
        outputs = ["out" "dev"];

        nativeBuildInputs = [pkgs.meson pkgs.ninja];
        mesonBuildType = "release";
        mesonFlags = ["-Db_lto=true" "-Dstrip=true"];
        enableParallelBuilding = true;

        installPhase = ''
          mkdir -p $out/bin/
          cp ${pname} $out/bin/
        '';

        meta = {
          homepage = "https://github.com/jawadcode/clam";
          license = [pkgs.lib.licenses.mit];
          maintainers = ["Jawad W. Ahmed"];
        };
      };
      default = clam-release;
    });

    devShells =
      eachSystem
      (system: let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        testing = pkgs.mkShell {
          packages = with pkgs; [
            llvm_18 # Just to get us llvm-symbolizer
            lldb # Currently at version 18
            self.packages.${system}.clam-debug
          ];
          ASAN_SYMBOLIZER_PATH = "${lib.getExe' pkgs.llvm_18 "llvm-symbolizer"}";
        };
        default =
          pkgs.mkShell.override {stdenv = pkgs.clang18Stdenv;}
          {
            inputsFrom = lib.attrValues self.packages.${system};
            packages = with pkgs; [
              llvm_18
              lldb
              mesonlsp
              clang-analyzer
            ];
          };
      });
  };
}
