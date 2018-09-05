# Replicator Agent Linux spec File
Summary: Replicator Agent
Name: SFTKdua-Linux
Version: 2.1.1
Release: 001
Copyright: Softek Technology Corporation.  All Rights Reserved.
Vendor: Softek Technology Corporation.  All Rights Reserved.
Group: system
#Source:
BuildRoot: /var/tmp/dua-root

%description
Replicator Agent

#%prep
#%setup

#%build
%install

%clean

%pre

%post
ln -fs /etc/init.d/SFTKdua /etc/rc3.d/S26SFTKdua
ln -fs /etc/init.d/SFTKdua /etc/rc4.d/S26SFTKdua
ln -fs /etc/init.d/SFTKdua /etc/rc5.d/S26SFTKdua

%preun
# Pre-removal script for linux 
rm /etc/rc3.d/S26SFTKdua
rm /etc/rc4.d/S26SFTKdua
rm /etc/rc5.d/S26SFTKdua
/etc/init.d/SFTKdua stop

%postun


%files
%attr(0555,root,root) /etc/init.d/SFTKdua
%dir %attr(0755,root,root) /opt/SFTKdua
%dir %attr(0755,root,root) /opt/SFTKdua/bin
%attr(0555,root,root) /opt/SFTKdua/bin/dtcAgent
%attr(0555,root,root) /opt/SFTKdua/bin/killdua
%dir %attr(0755,root,root) /etc/opt/SFTKdua
%attr(0644,root,root) /etc/opt/SFTKdua/dtcAgent.cfg
%dir %attr(0755,root,root) /var/opt/SFTKdua
%dir %attr(0755,root,root) /var/opt/SFTKdua/Agn_tmp

%changelog
