if [[ ! -e build ]]; then
	mkdir build
fi
cd build
cmake ..
make
./sample-ns $@
