{
  # options
  buildtype ? "release",
  compiler ? "clang",
  dep_type ? "shared",
  mesonFlags ? "",

  # deps
  system ? builtins.currentSystem,
  nixpkgs ? import <nixpkgs> { inherit system; },
  nonlibc ? (nixpkgs.nonlibc or import <nonlibc> {
    inherit system;
    inherit buildtype;
    inherit compiler;
    inherit dep_type;
    inherit mesonFlags;
  })
}:

with nixpkgs;

stdenv.mkDerivation rec {
  name = "xdpacket";
  version = "0.0.3";
  outputs = [ "out" ];

  meta = with stdenv.lib; {
    description = "the eXtremely Direct Packet handler";
    homepage = https://siriobalmelli.github.io/xdpacket/;
    license = licenses.gpl2;
    platforms = platforms.unix;
    maintainers = [ "https://github.com/siriobalmelli" ];
  };

  buildInputs = [
    clang
    meson
    ninja
    pandoc
    pkgconfig
    dpkg
    fpm
    rpm
    zip
  ];
  propagatedBuildInputs = [
    nonlibc
  ];

  # just work with the current directory (aka: Git repo), no fancy tarness
  src = ./.;

  # Override the setupHook in the meson nix derviation,
  # so that meson doesn't automatically get invoked from there.
  meson = pkgs.meson.overrideAttrs ( oldAttrs: rec { setupHook = ""; });

  # don't harden away position-dependent speedups for static builds
  hardeningDisable = [ "pic" "pie" ];

  # build
  mFlags = mesonFlags
    + " --buildtype=${buildtype}"
    + " -Ddep_type=${dep_type}";

  configurePhase = ''
      echo "------------------ build env --------------------------"
      env
      echo "-------------------------------------------------------"
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

  # Allow YouCompleteMe and other tooling to see into the byzantine
  #+	labyrinth of library includes.
  # TODO: this string manipulation ought to be done in Nix.
  shellHook = ''
      export CPATH=$(echo $NIX_CFLAGS_COMPILE | sed "s/ \?-isystem /:/g")
  '';
}
