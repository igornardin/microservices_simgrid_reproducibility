{
  pkgs ? import (fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/8b27c1239e5c421a2bbc2c65d52e4a6fbf2ff296.tar.gz";
    sha256 = "1gx0hihb7kcddv5h0k7dysp2xhf1ny0aalxhjbpj2lmvj7h9g80a";
  }) {}
}:

let 
  simgrid = pkgs.simgrid.overrideAttrs(oldAttrs: rec {
  version = oldAttrs.version + "-vm_load";
  rev = "b2e72d6b597494e29ef3471073fb5a8c2e3e563c"; # The desired SimGrid commit.
  src = pkgs.fetchurl {
    url = "https://github.com/igornardin/simgrid/archive/refs/tags/v1.0.tar.gz";
    sha256 = "sha256-gVTkq3IwYwwfHhFsnetVlPZJYgrN6b6COl/v6SmudEs=";
  };
});

in pkgs.stdenv.mkDerivation rec {
  pname = "Microservices_simgrid";
  version = "0.1.0";

  buildInputs = [
    pkgs.cmake
    pkgs.boost
    simgrid
    pkgs.nlohmann_json
    pkgs.gdb
    pkgs.python312
    pkgs.python312Packages.networkx
    pkgs.python312Packages.graphviz
    pkgs.python312Packages.pygraphviz
    pkgs.python312Packages.pandas
  ];

}