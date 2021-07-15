{ buildtype ? "release"
, compiler ? "clang"
, mesonFlags ? ""

, nixpkgs ? import (builtins.fetchGit {
    url = "https://siriobalmelli@github.com/siriobalmelli-foss/nixpkgs.git";
    ref = "refs/tags/sirio-2021-07-12";
    }) {}
, libjudy ? import (builtins.fetchGit {  # TODO: upstream pkgconfig or Meson Judy
    url = "https://siriobalmelli@github.com/siriobalmelli/libjudy.git";
    ref = "585024ad05642dfbfa84e0df7b99fb33eae0d2b1";
    }) { inherit nixpkgs; }
, nonlibc ? nixpkgs.nonlibc or import (builtins.fetchGit {
    url = "https://siriobalmelli@github.com/siriobalmelli/nonlibc.git";
    ref = "975066ba049ace0042dfd811cea5af0be7787133";
    }) { inherit nixpkgs; }
}:

with nixpkgs;

stdenv.mkDerivation rec {
  name = "xdpacket";
  version = "0.4.0";

  meta = with lib; {
    description = "the eXtremely Direct Packet handler";
    homepage = https://siriobalmelli.github.io/xdpacket/;
    license = licenses.gpl2;
    platforms = platforms.linux;  # TODO: expand to used BSD sockets
    maintainers = with maintainers; [ siriobalmelli ];
  };

  nativeBuildInputs = [
    clang
    gcc
    meson
    ninja
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

  buildInputs = [
    libjudy
    libyaml
    nonlibc
  ];

  # just work with the current directory (aka: Git repo), no fancy tarness
  src = if lib.inNixShell then null
    else nix-gitignore.gitignoreSource [] ./.;

  # Override the setupHook in the meson nix derivation,
  # so that meson doesn't automatically get invoked from there.
  meson = pkgs.meson.overrideAttrs ( oldAttrs: rec { setupHook = ""; });

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
