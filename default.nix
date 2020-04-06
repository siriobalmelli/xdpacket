{
  # options
  buildtype ? "release",
  compiler ? "clang",
  mesonFlags ? "",

  # deps
  system ? builtins.currentSystem,
  nixpkgs ? import (builtins.fetchGit {
    url = "https://siriobalmelli@github.com/siriobalmelli-foss/nixpkgs.git";
    ref = "master";
    }) {}
}:

with nixpkgs;

stdenv.mkDerivation rec {
  name = "xdpacket";
  version = "0.4.0";
  outputs = [ "out" ];

  meta = with nixpkgs.stdenv.lib; {
    description = "the eXtremely Direct Packet handler";
    homepage = https://siriobalmelli.github.io/xdpacket/;
    license = licenses.gpl2;
    platforms = platforms.unix;
    maintainers = [ "https://github.com/siriobalmelli" ];
  };

  libjudy = import (builtins.fetchGit {  # TODO: upstream pkgconfig or Meson Judy
    url = "https://siriobalmelli@github.com/siriobalmelli/libjudy.git";
    ref = "master";
    }) {};
  nonlibc = nixpkgs.nonlibc or import (builtins.fetchGit {
    url = "https://siriobalmelli@github.com/siriobalmelli/nonlibc.git";
    ref = "master";
    }) {};

  buildInputs = [
    clang
    gcc
    libjudy
    libyaml
    meson
    ninja
    nonlibc
    pandoc
    pkgconfig
  ] ++ lib.optional lib.inNixShell [
    cscope
    gdb
    man
    valgrind
    which
  ] ++ lib.optional (lib.inNixShell && (!stdenv.isDarwin)) [
    pahole
  ];

  # just work with the current directory (aka: Git repo), no fancy tarness
  src = if nixpkgs.lib.inNixShell then null
    else nixpkgs.nix-gitignore.gitignoreSource [] ./.;

  # Override the setupHook in the meson nix derivation,
  # so that meson doesn't automatically get invoked from there.
  meson = nixpkgs.pkgs.meson.overrideAttrs ( oldAttrs: rec { setupHook = ""; });

  # don't harden away position-dependent speedups for static builds
  hardeningDisable = [ "pic" "pie" ];

  # build
  mFlags = mesonFlags
    + " --buildtype=${buildtype}";

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
