Name:           geany-plugins-preview
Version:        0.2.4
Release:        1%{?dist}
Summary:        Preview plugin to render Markdown in Geany

License:        GPLv3+
URL:            https://github.com/xiota/geany-preview

Source0:        https://github.com/xiota/geany-preview/archive/refs/tags/v%{version}.tar.gz

# Disable LTO (segfaults when loaded as plugin)
%global _lto_cflags %{nil}

# Build dependencies
BuildRequires:  gcc-c++
BuildRequires:  git
BuildRequires:  meson
BuildRequires:  ninja-build
BuildRequires:  pkgconfig(geany)
BuildRequires:  pkgconfig(gtk+-3.0)
BuildRequires:  pkgconfig(libsoup-3.0)
BuildRequires:  pkgconfig(md4c-html)
BuildRequires:  pkgconfig(tomlplusplus)
BuildRequires:  pkgconfig(webkit2gtk-4.1)
BuildRequires:  podofo0.10-devel

Requires:       geany

%description
The Preview plugin provides a real-time preview of rendered Markdown and
other lightweight markup languages in a webview shown in the sidebar.

%prep
%setup -q -n geany-preview-%{version}

%build
%meson \
    --wrap-mode=default \
    -Dmarkdown_backend=md4c
%meson_build

%install
%meson_install

%files
%{_libdir}/geany/preview.so
