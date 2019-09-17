# dependencies setup script for Travis and AppVeyor CI

DUNE_VERSION="master"

# make sure we get the right mingw64 version of g++ on appveyor
PATH=/mingw64/bin:$PATH
echo "PATH=$PATH"
echo "MSYSTEM: $MSYSTEM"

which g++
which python
which cmake
g++ --version
gcc --version
cmake --version

# download Dune dependencies
for repo in dune-common dune-typetree dune-pdelab dune-multidomaingrid
do
  git clone -b support/dune-copasi --depth 1 --recursive https://gitlab.dune-project.org/santiago.ospina/$repo.git
done
for repo in core/dune-geometry core/dune-istl core/dune-localfunctions staging/dune-functions staging/dune-uggrid staging/dune-logging
do
  git clone -b $DUNE_VERSION --depth 1 --recursive https://gitlab.dune-project.org/$repo.git
done

# temporarily use fork of dunegrid to fix gmshreader on windows:
# todo: when fixed on master, add core/dune-grid back to list above
git clone -b gmsh_reader_fix --depth 1 --recursive https://gitlab.dune-project.org/liam.keegan/dune-grid.git

# on windows, symlinks from git repos don't work
# msys git replaces symlinks with a text file containing the linked file location
# so here we identify all such files, and replace them with the linked file
# note msys defines MSYSTEM variable: use this to check if we are on msys/windows
if [[ $MSYSTEM ]]; then
	rootdir=$(pwd)
		for repo in $(ls -d dune-*/)
		do
			echo "repo: $repo"
			cd $rootdir/$repo
			for f in $(git ls-files -s | awk '/120000/{print $4}')
			do
				dname=$(dirname "$f")
				fname=$(basename "$f")
				relf=$(cat $f)
				src="$rootdir/$repo/$dname/$relf"
				dst="$rootdir/$repo/$dname/$fname"
				echo "  - copying $src --> $dst"
				cp $src $dst
			done
		done
	cd $rootdir
fi

# patch cmake macro to avoid build failure when fortran compiler not found, e.g. on osx
cd dune-common
wget https://gist.githubusercontent.com/lkeegan/059984b71f8aeb0bbc062e85ad7ee377/raw/e9c7af42c47fe765547e60833a72b5ff1e78123c/cmake-patch.txt
echo '' >> cmake-patch.txt
git apply cmake-patch.txt
cd ../

./dune-common/bin/dunecontrol --opts=${DUNE_OPTIONS_FILE} all

# if [[ $MSYSTEM ]]; then
# 	cat dune-copasi/build-cmake/src/CMakeFiles/dune_copasi.dir/linklibs.rsp
# 	ldd dune-copasi/build-cmake/src/dune_copasi.exe
# fi