{
	description = "Sinewave synth CLAP plugin";

	inputs = {
		};

	outputs = { self, nixpkgs }:
		let
			# Why doesn't the flakes build system handle this automatically?!
			forAllSystems = nixpkgs.lib.genAttrs nixpkgs.lib.systems.flakeExposed;
			nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; });
		in {
			packages = forAllSystems (system: {
				default =
					nixpkgsFor.${system}.stdenv.mkDerivation {
						name = "sine-ping";
						src = self;
						buildInputs = with nixpkgsFor.${system}; [ clap ];
						installPhase = ''
							mkdir -p $out/bin
							cp sine-ping.so $out/bin/
							'';
						};
					});
			};
}


