Name: rest2smtp
Version: 0.9.1
Release: 1%{?dist}
Summary: REST API to send emails via SMTP

License: (Your project's license)
URL: https://github.com/badbat75/rest2smtp

BuildRequires: cmake
BuildRequires: libconfig-devel
BuildRequires: libmicrohttpd-devel
BuildRequires: json-c-devel
BuildRequires: libcurl-devel
BuildRequires: uuid-devel
BuildRequires: systemd-devel

Requires: libconfig
Requires: libmicrohttpd
Requires: json-c
Requires: uuid
Requires: systemd

%description
This project exposes a REST API that accepts a JSON message to send an email via the SMTP protocol to an email server. The application can use a queue when specified in the config file.

%prep
%autosetup

%build
%cmake -DCMAKE_BUILD_TYPE=Release .
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

%post
mkdir -p %{_localstatedir}/spool/rest2smtp

%files
%{_bindir}/rest2smtp
%{_sysconfdir}/rest2smtp/rest2smtp.conf

%changelog
* Fri May 05 2023 Emiliano De Simoni <emiliano.desimoni@outlook.com> - 0.9.1-1
- Added rest2smtp.spec to build RPM

* Fri May 05 2023 Emiliano De Simoni <emiliano.desimoni@outlook.com> - 0.9-1
- Initial package
