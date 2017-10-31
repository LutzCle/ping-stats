{ pkgs ? (import <nixpkgs> {}) }:
   
let env = with pkgs; [ gdb gcc clang ];
in pkgs.stdenv.mkDerivation rec {
  name = "C++Env";
  src = ./.;
  version = "0.0.0";

  buildInputs = [ env ];
}
