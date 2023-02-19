{ pkgs }: {
	deps = [
		pkgs.gh
  		pkgs.time
  		pkgs.htop
  		pkgs.pkgconfig
  		pkgs.valgrind
  		pkgs.vim
  		pkgs.llvmPackages_13.llvm
  		pkgs.clang_12
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}