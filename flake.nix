{
  description = "Functional, bytecode interpreted language written in C";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    # Externally extensible flake systems. See <https://github.com/nix-systems/nix-systems>.
    systems.url = "github:nix-systems/default";
  };

  outputs = { self, systems, nixpkgs, ... }:
    let
      # Nixpkgs library functions.
      lib = nixpkgs.lib;

      # Iterate over each system, configured via the `systems` input.
      eachSystem = lib.genAttrs (import systems);
    in
    {
      packages = eachSystem (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        rec {
          clam = pkgs.stdenv.mkDerivation rec {
            pname = "clam";
            version = "0.1.0";
            src = ./.;
            outputs = [ "out" "dev" ];

            nativeBuildInputs = [ pkgs.meson pkgs.ninja ];
            mesonBuildType = "release";
            enableParallelBuilding = true;

            installPhase = ''
              mkdir -p $out/bin/
              cp ${pname} $out/bin/
            '';

            meta = {
              homepage = "https://github.com/jawadcode/clam";
              license = with pkgs.lib.licenses; [ mit ];
              maintainers = [ "Jawad W. Ahmed" ];
            };
          };
          default = clam;
        });

      devShells = eachSystem
        (system:
          let
            pkgs = nixpkgs.legacyPackages.${system};
          in
          {
            testing = pkgs.mkShell {
              packages = [ self.packages.${system}.clam ];
            };
            default = pkgs.mkShell.override { stdenv = pkgs.clang18Stdenv; }
              {
                inputsFrom = lib.attrValues self.packages;
                packages = [
                  pkgs.mesonlsp
                  pkgs.clang-tools
                ];
              };
          });
    };
}
