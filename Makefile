.SILENT:
.PHONY: all build clean clear clear-version configure help info install sysinstall uninstall update-version
notarget: help

plugin_name := preview

info: help
help:
	echo "Please provide a target:" ; \
	echo "   build      - build in ./build" ; \
	echo "   clean      - delete build and pkgdir" ; \
	echo "   configure  - configure in ./build" ; \
	echo "   install    - install to ./pkgdir" ; \
	echo "   sysinstall - install to system" ; \
	echo "   uninstall  - delete from system" ; \
	:

all: update-version configure build install

update-version:
	version=$(shell git describe --tags --long --abbrev=7 | sed -E 's/^[^0-9]*//;s/-([0-9]*-g.*)$$/.r\1/;s/-/./g') && \
	version="$${version##.r0.*}" && \
	meson rewrite kwargs set project / version $$version && \
	echo "project version: $$version" && \
	:

clear-version:
	meson rewrite kwargs delete project / version - && \
	:

install:
	meson install -C build --destdir=../pkgdir

sysinstall:
	sudo meson install -C build

uninstall:
uninstall:
	sudo rm -f /usr/lib/geany/$(plugin_name).so ; \
	sudo rm -rf /usr/share/geany-$(plugin_name) ; \
	sudo rm -rf /usr/share/doc/geany-$(plugin_name) ; \
	:

clean-all: clear-version clean

clean:
	rm -rf build && \
	rm -rf pkgdir && \
	:

clear: clear-version clean

configure:
	if [ ! -d build ] ; then meson setup build ; else meson setup build --reconfigure ; fi

build:
	if [ ! -d build ] ; then meson setup build ; fi && \
	meson compile -C build && \
	:
