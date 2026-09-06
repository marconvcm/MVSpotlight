Name:           mvspotlight
Version:        1.0.0
Release:        1%{?dist}
Summary:        macOS Spotlight-inspired launcher for GNOME 50 on Wayland

License:        MIT
URL:            https://github.com/marconvcm/MVSpotlight
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.20
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  qt6-qtsvg-devel
BuildRequires:  desktop-file-utils

Requires:       qt6-qtbase-gui
Requires:       qt6-qtdeclarative
Requires:       qt6-qtsvg
Requires:       hicolor-icon-theme

Recommends:     gh

%description
A polished desktop launcher and search application for GNOME 50 on Linux,
inspired by macOS Spotlight, built with Qt 6, Qt Quick, Wayland, and a Lua
plugin ecosystem.

%prep
%autosetup

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%check
%ctest

%post
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :

%postun
if [ $1 -eq 0 ] ; then
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    /usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi

%posttrans
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files
%license LICENSE
%doc README.md
%{_bindir}/mvspotlight
%{_datadir}/applications/*.desktop
%{_sysconfdir}/xdg/autostart/mvspotlight.desktop
%{_datadir}/icons/hicolor/scalable/apps/*.svg
%{_datadir}/mvspotlight/
%{_prefix}/lib/systemd/user/mvspotlight.service

%changelog
* Sat Sep 05 2026 MVSpotlight Contributors <marconvm@users.noreply.github.com> - 1.0.0-1
- Initial RPM release of MVSpotlight 1.0.0
