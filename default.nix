{ 	# deps
	system ? builtins.currentSystem,
	nixpkgs ? import <nixpkgs> { inherit system; },
	nonlibc ? nixpkgs.nonlibc or import <nonlibc> { inherit system;
							inherit buildtype;
							inherit compiler;
							inherit dep_type;
							inherit mesonFlags;
	},
	# options
	buildtype ? "release",
	compiler ? "clang",
	dep_type ? "shared",
	mesonFlags ? ""
}:

# note that "nonlibc" above should not be clobbered by this
with import <nixpkgs> { inherit system; };

stdenv.mkDerivation rec {
	name = "xdpacket";
	outputs = [ "out" ];

	buildInputs = [
		clang
		liburcu
		meson
		ninja
		nonlibc
		pandoc
		pkgconfig
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
		echo "flags: $mFlags"
		echo "prefix: $out"
		CC=${compiler} meson --prefix=$out build $mFlags
		cd build
		''; 

	buildPhase = "ninja";
	doCheck = true;
	checkPhase = "ninja test";
	installPhase = "ninja install";
}
