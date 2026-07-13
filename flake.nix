{
  description = "Functional, bytecode interpreted language written in C";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

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

    llvmPkgs' = pkgs: pkgs.llvmPackages_22;
    clangStdenv' = pkgs: llvmPkgs:
      if pkgs.stdenv.targetPlatform.isDarwin
      then llvmPkgs.stdenv
      # I am speed
      else pkgs.stdenvAdapters.useMoldLinker llvmPkgs.stdenv;
  in {
    packages = eachSystem (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};
        llvmPkgs = llvmPkgs' pkgs;
        clangStdenv = clangStdenv' pkgs llvmPkgs;
        baseOpts = rec {
          pname = "clam";
          version = "0.1.0";
          src = ./.;
          outputs = ["out"];
          nativeBuildInputs = with pkgs; [meson ninja];
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
      in rec {
        clam-debug = clangStdenv.mkDerivation (baseOpts
          // {
            mesonBuildType = "debug";
            # Without this we get a _FORTIFY_SOURCE related compiler warning from
            # clang, so we need to disable it for debug builds, for a relevant GH
            # issue, see: https://github.com/NixOS/nixpkgs/issues/60919
            hardeningDisable = ["fortify"];
            mesonFlags = ["-Db_sanitize=address,undefined"];
            enableParallelBuilding = true;
          });
        clam-release = clangStdenv.mkDerivation (baseOpts
          // {
            mesonBuildType = "release";
            mesonFlags = [
              "-Db_lto=true"
              "-Dstrip=true"
            ];
          });
        default = clam-release;
      }
    );
    devShells = eachSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      llvmPkgs = llvmPkgs' pkgs;
      clangStdenv = clangStdenv' pkgs llvmPkgs;
    in {
      default = pkgs.mkShell.override {stdenv = clangStdenv;} {
        inputsFrom = lib.attrValues self.packages.${system};
        nativeBuildInputs = [llvmPkgs.clang-tools];
        packages = with pkgs; [llvmPkgs.bintools llvmPkgs.lldb meson ninja clang-analyzer mesonlsp];
        # I think this is no longer necessary but I'll keep it around
        # ASAN_SYMBOLIZER_PATH = "${lib.getExe' pkgs.llvm_19 "llvm-symbolizer"}";
      };
    });
  };
}
