{
  pkgs ? import (fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/8b27c1239e5c421a2bbc2c65d52e4a6fbf2ff296.tar.gz";
    sha256 = "1gx0hihb7kcddv5h0k7dysp2xhf1ny0aalxhjbpj2lmvj7h9g80a";
  }) {}
}:
pkgs.stdenv.mkDerivation rec {
  pname = "simgrid-test";
  version = "0.1.0";

  buildInputs = [
    pkgs.cmake
    pkgs.boost
    pkgs.simgrid
    pkgs.nlohmann_json
    pkgs.gdb
  ];

}