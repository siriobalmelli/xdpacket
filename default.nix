{
  # options
  buildtype ? "release",
  compiler ? "clang",
  dep_type ? "shared",
  mesonFlags ? "",

  # deps
  system ? builtins.currentSystem,
  nixpkgs ? import (builtins.fetchGit {
    url = "https://github.com/siriobalmelli-foss/nixpkgs.git";
    ref = "sirio";
    }) {}
}:

with nixpkgs;

nixpkgs.stdenv.mkDerivation rec {
  name = "xdpacket";
  version = "0.2.0";
  outputs = [ "out" ];

  meta = with nixpkgs.stdenv.lib; {
    description = "the eXtremely Direct Packet handler";
    homepage = https://siriobalmelli.github.io/xdpacket/;
    license = licenses.gpl2;
    platforms = platforms.unix;
    maintainers = [ "https://github.com/siriobalmelli" ];
  };

  libjudy = nixpkgs.libjudy or import (builtins.fetchGit {
    url = "https://github.com/siriobalmelli/libjudy.git";
    ref = "master";
    }) {};
  nonlibc = nixpkgs.nonlibc or import (builtins.fetchGit {
    url = "https://github.com/siriobalmelli/nonlibc.git";
    ref = "master";
    }) {};

  inputs = [
    nixpkgs.gcc
    nixpkgs.clang
    nixpkgs.meson
    nixpkgs.ninja
    nixpkgs.pandoc
    nixpkgs.pkgconfig
  ];
  buildInputs = if ! lib.inNixShell then inputs else inputs ++ [
    nixpkgs.cscope
    nixpkgs.gdb
    nixpkgs.man
    nixpkgs.pahole
    nixpkgs.valgrind
    nixpkgs.which
  ];
  propagatedBuildInputs = [
    nixpkgs.libyaml
    libjudy
    nonlibc
  ];

  # just work with the current directory (aka: Git repo), no fancy tarness
  src = if lib.inNixShell then null else ./.;

  # Override the setupHook in the meson nix derviation,
  # so that meson doesn't automatically get invoked from there.
  meson = nixpkgs.pkgs.meson.overrideAttrs ( oldAttrs: rec { setupHook = ""; });

  # don't harden away position-dependent speedups for static builds
  hardeningDisable = [ "pic" "pie" ];

  # build
  mFlags = mesonFlags
    + " --buildtype=${buildtype}"
    + " -Ddep_type=${dep_type}";

  configurePhase = ''
      echo "flags: $mFlags"
      echo "prefix: $out"
      CC=${compiler} meson --prefix=$out build $mFlags
      cd build
  '';

  buildPhase = ''
      ninja
  '';
  doCheck = true;
  checkPhase = ''
      ninja test
  '';
  installPhase = ''
      ninja install
  '';
}
