{
  # options
  buildtype ? "release",
  compiler ? "clang",
  dep_type ? "shared",
  mesonFlags ? "",

  # deps
  system ? builtins.currentSystem,
  nixpkgs ? import <nixpkgs> { inherit system; }
}:

with nixpkgs;

nixpkgs.stdenv.mkDerivation rec {
  name = "xdpacket";
  version = "0.0.6";
  outputs = [ "out" ];

  meta = with nixpkgs.stdenv.lib; {
    description = "the eXtremely Direct Packet handler";
    homepage = https://siriobalmelli.github.io/xdpacket/;
    license = licenses.gpl2;
    platforms = platforms.unix;
    maintainers = [ "https://github.com/siriobalmelli" ];
  };

  libjudy = nixpkgs.libjudy or import ./libjudy {};
  nonlibc = nixpkgs.nonlibc or import ./nonlibc {};
  memorywell = nixpkgs.memorywell or import ./memorywell {};

  buildInputs = [
    nixpkgs.gcc
    nixpkgs.clang
    nixpkgs.meson
    nixpkgs.ninja
    nixpkgs.pandoc
    nixpkgs.pkgconfig
    nixpkgs.dpkg
    nixpkgs.fpm
    nixpkgs.rpm
    nixpkgs.zip
  ];
  propagatedBuildInputs = [
    nixpkgs.libyaml
    libjudy
    memorywell
    nonlibc
  ];

  # just work with the current directory (aka: Git repo), no fancy tarness
  src = ./.;

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

  # Build packages outside $out then move them in: fpm seems to ignore
  #+	the '-x' flag that we need to avoid packaging packages inside packages
  postFixup = ''
      mkdir temp
      for pk in "deb" "rpm" "tar" "zip"; do
          if ! fpm -f -t $pk -s dir -p temp/ -n $name -v $version \
              --description "${meta.description}" \
              --license "${meta.license.spdxId}" \
              --url "${meta.homepage}" \
              --maintainer "${builtins.head meta.maintainers}" \
              "$out/=/"
          then
              echo "ERROR (non-fatal): could not build $pk package" >&2
          fi
      done
      mkdir -p $out/var/cache/packages
      mv -fv temp/* $out/var/cache/packages/
  '';
}
