#!/bin/bash -e
export PS4="$ "
set -x

if [ ! -e $HOME/.local/bin/meson ]; then
	wget https://bootstrap.pypa.io/get-pip.py
	python3.6 get-pip.py --user
	pip3 install --user meson
	rm get-pip.py
fi

if [ ! -e $HOME/.local/bin/ninja ]; then
	wget https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-linux.zip
	unzip ninja-linux.zip -d ~/.local/bin
	rm ninja-linux.zip
fi

cd $HOME/build/deps
unset COVERAGE

if [ ! -d crunch ]; then
	git clone https://github.com/DX-MON/crunch crunch
	cd crunch
	meson build --prefix=/usr --buildtype=release -Dstrip=true
	ninja -C build
	sudo ninja -C build install
	cd ..
fi

if [ ! -d rSON ]; then
	git clone https://github.com/DX-MON/rSON rSON
	cd rSON
	meson build --prefix=/usr --buildtype=release -Dstrip=true
	ninja -C build
	sudo ninja -C build install
	cd ..
fi
