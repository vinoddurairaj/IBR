#! /bin/sh
# RHEL7_GA_lsb_rpm_dependencies_install.sh

cd /root/RPM
rpm -ivh redhat-lsb-submod-security-4.1-24.el7.x86_64.rpm
sleep 2 # Putting a delay to be able to check for error messages

rpm -ivh redhat-lsb-submod-multimedia-4.1-24.el7.x86_64.rpm
sleep 2

rpm -ivh pax-3.4-19.el7.x86_64.rpm
sleep 2

rpm -ivh spax-1.5.2-11.el7.x86_64.rpm
sleep 2

rpm -ivh redhat-lsb-core-4.1-24.el7.x86_64.rpm
sleep 2

rpm -ivh redhat-lsb-cxx-4.1-24.el7.x86_64.rpm
sleep 2

rpm -ivh libpng12-1.2.50-6.el7.x86_64.rpm
sleep 2

rpm -ivh redhat-lsb-desktop-4.1-24.el7.x86_64.rpm
sleep 2

rpm -ivh perl-XML-NamespaceSupport-1.11-10.el7.noarch.rpm
sleep 2

rpm -ivh perl-XML-SAX-Base-1.08-7.el7.noarch.rpm
sleep 2

rpm -ivh perl-XML-SAX-0.99-9.el7.noarch.rpm
sleep 2

rpm -ivh perl-XML-LibXML-2.0018-5.el7.x86_64.rpm
sleep 2

rpm -ivh perl-Class-ISA-0.36-1010.el7.noarch.rpm
sleep 2

rpm -ivh perl-Pod-Plainer-1.03-4.el7.noarch.rpm
sleep 2

rpm -ivh redhat-lsb-languages-4.1-24.el7.x86_64.rpm
sleep 2

rpm -ivh foomatic-filters-4.0.9-6.el7.x86_64.rpm
sleep 2

rpm -ivh foomatic-db-filesystem-4.0-40.20130911.el7.noarch.rpm
sleep 2

rpm -ivh foomatic-db-ppds-4.0-40.20130911.el7.noarch.rpm
sleep 2

rpm -ivh foomatic-db-4.0-40.20130911.el7.noarch.rpm
sleep 2

rpm -ivh redhat-lsb-printing-4.1-24.el7.x86_64.rpm
sleep 2

rpm -ivh redhat-lsb-4.1-24.el7.x86_64.rpm