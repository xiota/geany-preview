plugin_name := preview


.SILENT:
.PHONY: all dist dist-clean install uninstall clean help update-version clear-version configure build

all: configure build

install:
	cd build && \
	meson install && \
	:

uninstall:
	sudo rm -f /usr/lib/geany/$(plugin_name).so ; \
	sudo rm -rf /usr/share/geany-$(plugin_name) ; \
	sudo rm -rf /usr/share/doc/geany-$(plugin_name) ; \
	:

clean:
	rm -rf build && \
	:

help:
	echo "Please provide a target:" ; \
	echo "  [default]   - configure and build" ; \
	echo "   install    - install to system" ; \
	echo "   uninstall  - delete from system" ; \
	echo "   clean      - delete build dir" ; \
	:

update-version:
	version=$(shell git describe --tags --abbrev=7 | sed -E 's/^[^0-9]*//;s/-([0-9]*-g.*)$$/.r\1/;s/-/./g') && \
	meson rewrite kwargs set project / version $$version && \
	echo "project version: $$version" && \
	:

clear-version:
	meson rewrite kwargs delete project / version - && \
	:

configure:
	export PKG_CONFIG_PATH="$(pkgconf_path)" && \
	meson setup build -Dcpp_std=c++17 && \
	:

build:
	export PKG_CONFIG_PATH="$(pkgconf_path)" && \
	cd build && \
	ninja && \
	:

dist:
	export version=$(shell git describe --tags --abbrev=7 | sed -E 's/^[^0-9]*//;s/-([0-9]*-g.*)$$/.r\1/;s/-/./g') && \
	echo "... $${version} ..." && \
	export pkgdir="geany-plugin-preview_$$version" && \
	echo "... $${pkgdir} ..." && \
	mkdir -p "$${pkgdir}" && \
	cp --reflink=auto -r -t "$${pkgdir}/" -- data docs source tools config* *.md Makefile meson* && \
	pushd "$${pkgdir}" && \
	echo "rewrite version..." && \
	meson rewrite kwargs set project / version $$version && \
	echo "rewrite depends..." && \
	meson rewrite kwargs delete dependency libpodofo version xxx && \
	popd && \
	echo "tar..." && \
	tar -c -J --numeric-owner --owner=0 -f "$${pkgdir}.orig.tar.xz" "$${pkgdir}" && \
	:

dist-clean:
	rm -rf geany-plugin-preview_*/ && \
	rm -f geany-plugin-preview_*.orig.tar.xz && \
	:
