# %PRODUCTNAME% Linux spec File
#/bin/uname -r|grep "2.6" 1 > /dev/null 2>&1
Summary: Transparent Data Migration Facility.
Name: %{PRODUCTNAME_TOKEN}
Version: %VERSION% 
Release: %BUILDNUM%
License: %COPYRIGHTYEAR 2001%, %COMPANYNAME%. All Rights Reserved.
Vendor: %COMPANYNAME%.
Group: system

# The following pre requisite and global options are required to comply with LSB.  C.F. RFX-123
AutoReqProv:	no
PreReq:		lsb >= 3.0

%global _binary_filedigest_algorithm 1
%global _source_filedigest_algorithm 1
%global _source_payload       w9.gzdio
%global _binary_payload       w9.gzdio

#Source: 

%description
%PRODUCTNAME%, is a network-based disk mirroring product for AIX, HP-UX, Solaris and Linux 
that enables data exchange and synchronization support of local applications, data sharing among remote sites ,
and disaster recovery.

%pre

check_distribution() {
# Validates that we're being installed on a supported distribution.
# Supported distribution: Red Hat 4, 5, 6 and 7, SuSE 10 and 11.
# Returns 0 if we detect a valid distribution and some error value otherwise.

    result=0
    if [ -f ${REDHAT_DIST} ]
    then
        if ! egrep "Red Hat Enterprise Linux.*release (4|5|6|7)" ${REDHAT_DIST} 2>&1 > /dev/null 
        then
           	echo ""
            echo "This software can only be installed on Red Hat Enterprise 4, 5, 6 or 7."
            result=1
        fi
    elif [ -f ${SUSE_DIST} ]
    then
        if ! egrep "SUSE Linux Enterprise Server (10|11|12)" ${SUSE_DIST} 2>&1 > /dev/null 
        then
    	    echo ""
            echo "This software can only be installed on SuSE 10, 11 or 12."
            result=1
        fi
    else
    	echo ""
    	echo "ERROR: This software can only be installed on a Red Hat or SuSE Linux distribution."
        result=1
    fi
    return $result
}

compare_copy() {
    if ! cmp -s $1 $2
    then
        echo "ERROR: Backup copy of \"$1\" to \"$2\" failed."
	exit 1;
    fi
    return 0
}

REDHAT_DIST=/etc/redhat-release
SUSE_DIST=/etc/SuSE-release

if ! check_distribution
then
    exit 1
fi

PCRELEASE=`/bin/uname -r`
MODULESDEP=/lib/modules/${PCRELEASE}/modules.dep
if [ ! -f ${MODULESDEP} ]
then
    echo Creating ${MODULESDEP}
    /sbin/depmod -a || {
	echo "ERROR: The depmod program had issues creating the modules.dep file"
    }
fi

#%prep
#%setup  


#%build
%install

%ifdistribution == redhat
 %ifdistrelease >= 6
	cd ..
	mkdir -p %{BUILDROOT}
	mkdir -p %{BUILDROOT}/%{OPTDIR}
	mkdir -p %{BUILDROOT}/%{ETCOPTDIR}/%{OEM}%{dtcQ}
	mkdir -p %{BUILDROOT}/%{VAROPTDIR}/%{OEM}%{dtcQ}
	mkdir -p %{BUILDROOT}/%{VAROPTDIR}/%{OEM}%{dtcQ}/Agn_tmp
	mkdir -p %{BUILDROOT}/etc/init.d
	mkdir -p %{BUILDROOT}/sbin
	mkdir -p %{BUILDROOT}/usr/sbin
	mkdir -p %{BUILDROOT}/var/run/%{OEM}%{dtcQ}
	cp -pR %{CURDIR}/pkg/%{OEM}%{dtcQ} %{BUILDROOT}/%{OPTDIR}
	cp -p %{CURDIR}/pkg/etc/init.d/%{OEM}%{dtcQ}-* %{BUILDROOT}/etc/init.d
	cp -pR %{CURDIR}/pkg/etc/opt/%{OEM}%{dtcQ} %{BUILDROOT}/%{ETCOPTDIR}
	for x in %{MODULES_EXCLUDED} ; do rm -f %{BUILDROOT}/%{ETCOPTDIR}/%{OEM}%{dtcQ}/driver/${x} ; done
	rmdir --ignore-fail-on-non-empty %{BUILDROOT}/%{ETCOPTDIR}/%{OEM}%{dtcQ}/driver/linux-*
	cp -pR %{CURDIR}/pkg/sbin/ %{BUILDROOT}
	cp -pR %{CURDIR}/pkg/usr/sbin/ %{BUILDROOT}/usr
 %endif
%endif

%clean

%post
compare_copy() {
    if ! cmp -s $1 $2
    then
        echo "ERROR: Backup copy of \"$1\" to \"$2\" failed."
	exit 1;
    fi
    return 0
}

SERVICES=/etc/services
BASEDIR=/opt
RMD_PORT=575

REDHAT_DIST=/etc/redhat-release
SUSE_DIST=/etc/SuSE-release

isSuSE12=0
if [ -f ${SUSE_DIST} ]
then
    if egrep "SUSE Linux Enterprise Server 12" ${SUSE_DIST} 2>&1 > /dev/null 
    then
        isSuSE12=1
    fi
fi


if [ -f ${REDHAT_DIST} ]
then
SYSINIT=/etc/rc.d/rc.sysinit
elif [ -f ${SUSE_DIST} ]
then
LOCALFS=/etc/init.d/boot.localfs
fi

HALT=/etc/init.d/halt
SEDTMP=/var/tmp/%OEM%%Q%.sed

if [ "$BASEDIR" != "/opt" ]; then
        ln -s $BASEDIR/%OEM%%Q%  /opt/%OEM%%Q% > /dev/null 2>&1
fi

#
# create soft link for dtcautmount for
# backward compatibility
#
ln -s ${BASEDIR}/%OEM%%Q%/bin/%Q%modfs ${BASEDIR}/%OEM%%Q%/bin/%Q%automount

FTDLINKS="%Q%checkpoint %Q%configtool %Q%debugcapture %Q%hostinfo %Q%info %Q%init %Q%killpmd %Q%killrefresh %Q%killrmd %Q%licinfo %Q%monitortool %Q%override %Q%perftool %Q%pmd %Q%refresh %Q%rmdreco %Q%set %Q%start %Q%stop in.%Q% in.pmd in.rmd kill%Q%master killpmds killrefresh killrmds launch%Q%master launchpmds launchrefresh throtd %Q%modfs %Q%automount in.%QAGN% %Q%agentset launchagent killagent %Q%psmigrate %Q%panalyze launchnetanalysis %Q%netanalysis %Q%stopnetanalysis %Q%psmigrate272"
FTDLINKS="$FTDLINKS %Q%monitortty %Q%hrdb_maths %Q%failover %Q%sendfailover"
# Note: launchbackfresh, killbackfresh, %Q%backfresh, %Q%killbackfresh deactivated as of TDMF IP 2.8.0
#
# dynamic volume expansion feature disabled
# %Q%reconfig %Q%expand %Q%limitsize
#
# make links from /usr/local/bin to ${BASEDIR}/%OEM%%Q%/bin/*
#
if [ -d /usr/local/bin -a -w /usr/local/bin ]; then
    echo "Creating Symbolic Links in /usr/local/bin"
	for LINK in $FTDLINKS
    do
        ln -s ${BASEDIR}/%OEM%%Q%/bin/${LINK} /usr/local/bin/${LINK} > /dev/null 2>&1
    done
fi
# Place in.%Q% in $SERVICES
echo " find and create entry of in.%Q% in $SERVICES"
/bin/cp -p $SERVICES $SERVICES.pre%Q%
compare_copy $SERVICES $SERVICES.pre%Q%
(/bin/egrep -v 'in\.%Q%' $SERVICES.pre%Q% ; \
 echo "in.%Q%           ${RMD_PORT}/tcp                 # %OEM%%Q% master daemon" ) \
        > $SERVICES

# Check if the flag file exists indicating that we use the legacy licensing mechanism
# in which case the license is provided by Product Support or we reuse the already existing customer license.
# For TDMFIP 2.9.0, force the legacy licensing mechanism (but preserve the old code based on the flag file).
if [ -f /usr/bin/touch ]
then
    /usr/bin/touch /var/opt/SFTKdtc/SFTKdtc_use_legacy_mechanism
else
    /bin/touch /var/opt/SFTKdtc/SFTKdtc_use_legacy_mechanism
fi
 
if [ -f /var/opt/SFTKdtc/SFTKdtc_use_legacy_mechanism ]
then
    /bin/rm /etc/opt/%OEM%%Q%/DTC.lic.perm
else
    # Rename the new packaged permanent license file to the effective name
    echo "Making DTC.lic.perm the effective DTC.lic."
    /bin/mv /etc/opt/%OEM%%Q%/DTC.lic.perm /etc/opt/%OEM%%Q%/DTC.lic
    /bin/chmod 0444 /etc/opt/%OEM%%Q%/DTC.lic
fi

# Restore shell script files from past revs, and license file if legacy mechanism used
if [ -f /var/opt/SFTKdtc/SFTKdtc_use_legacy_mechanism ]
then
    echo "Restore license file and shell script files from past revs"
else
    echo "Restore shell script files from past revs"
fi
THISDIR="`/bin/pwd`"
if [ -d /var/opt/%OEM%%Q% ]; then
    cd /var/opt/%OEM%%Q%
    a="`find . -type d -print | xargs ls -td | /bin/egrep ^\./%OEM%%Q% | head -1`"
    if [ "${a}" != "" ]; then
        # Starting with release 2.8.0 (product ownership going to IBM STG), the license file is now
        # part of the package; we do not need to restore a previously installed license UNLESS the flag file
        # has been found indicating that we use the old mechanism for this site.
        if [ -f /var/opt/SFTKdtc/SFTKdtc_use_legacy_mechanism ]
        then
            for i in ${a}/*.lic
            do
                echo "Restoring previously saved %OEM%%Q% license key file."
                /bin/cp ${i} /etc/opt/%OEM%%Q% > /dev/null 2>&1
            done
        fi
	    if [ -f ${a}/%Q%Agent.cfg ]; then
	        echo "Restoring previous Agent config file."
	        /bin/cp ${a}/%Q%Agent.cfg /etc/opt/%OEM%%Q% > /dev/null 2>&1
	    fi
        for i in ${a}/devlist.conf
        do
            echo "Restoring previously saved %OEM%%Q% device list file."
            /bin/cp ${i} /etc/opt/%OEM%%Q% > /dev/null 2>&1
        done
        for i in ${a}/*.sh
        do
            # We do not want to restore the dtc_Linux_bootdrive_post_failover script
            # as it may change from one release to another (PROD12058)
            echo $i | /bin/grep dtc_Linux_bootdrive_post_failover 1> /dev/null 2>&1
            if [ $? -ne 0 ]
            then
                /bin/cp $i /etc/opt/%OEM%%Q% > /dev/null 2>&1
            fi
        done
        /bin/chmod +x /etc/opt/%OEM%%Q%/*.sh > /dev/null 2>&1
    fi
fi

# see if Previous %PRODUCTNAME% Installation saves exist
echo "see if Previous %PRODUCTNAME% Installation saves exist"
cd /var/opt/%OEM%%Q%
b="`find . -type d -print | xargs ls -td | egrep ^\./%OEM%%Q% | head -1`"
if [ "${b}" = "" ]; then
    if [ -d /var/opt/FTSWftd ]; then
        cd /var/opt/FTSWftd
        a="`find . -type d -print | xargs ls -td | egrep / | head -1`"
        if [ "${a}" != "" ]; then
            # Starting with release 2.8.0 (product ownership going to IBM STG), the license file is now
            # part of the package; we do not need to restore a previously installed license UNLESS the flag file
            # has been found indicating that we use the old mechanism for this site.
            if [ -f /var/opt/SFTKdtc/SFTKdtc_use_legacy_mechanism ]
            then
                if [ "${a}/*.lic" != "" ]; then
                    echo "Converting and restoring previously saved %PRODUCTNAME% Data"
                    echo "   license key file to the %PRODUCTNAME% %VERSION% installation."
                fi
                for i in ${a}/*.lic
                do
                    /bin/cat ${i} | /bin/sed 's/FTD_LICENSE/%CAPQ%_LICENSE/g' > /etc/opt/%OEM%%Q%/%CAPQ%.lic
                done
            fi
            # see if there are any shell scripts for checkpointing
            cd ${a}
            if [ "*.sh" != "" ]; then
                echo "Migrating previously saved %PRODUCTNAME% checkpoint shell script files."
                echo "*** WARNING"
                echo "\07*** WARNING:  Automatic conversion of checkpoint script files "
                echo "*** WARNING:  is being attempted.  These files may require "
                echo "*** WARNING:  additional editing before use!!!"
                echo "*** WARNING"
            fi
            for i in *.sh
            do
                /bin/cat $i | /bin/sed '{
s/FTSWftd/%OEM%%Q%/g
s/ftd/%Q%/g
s/FTD/%CAPQ%/g
}' > /etc/opt/%OEM%%Q%/$i
            /bin/chmod 755 /etc/opt/%OEM%%Q%/$i
            done
        fi
    fi
fi

# Always make sure modifications to /etc/init.d (rc.d) files are done before chkconfig commands
if [ -f ${REDHAT_DIST} ]
then
grep "%PKGNM%-scan" ${SYSINIT} > /dev/null 2>&1
if [ $? -ne 0 ]
then
    if [ -f ${SYSINIT} ]
    then
        # save current copy off, if it exists, where it can be restored in case of failure
        echo "Saving current $SYSINIT to ${SYSINIT}.new%CAPQ%"
        /bin/cp -p $SYSINIT ${SYSINIT}.new%CAPQ%
        compare_copy $SYSINIT ${SYSINIT}.new%CAPQ%
        /bin/gzip -f -S .gz.save ${SYSINIT}.new%CAPQ%
        #
    fi
    PCRELEASE=`/bin/uname -r`
    case "$PCRELEASE" in
	    2.6.9-*)
    		lineno=`/bin/cat ${SYSINIT} | /bin/sed -n -e '/Check filesystems/=' | tail -1`
    		if [ -z "${lineno}" ]
    		then
        		echo "ERROR: ${SYSINIT} context to modify was not found";
    		else
        		echo "${lineno}i\\" > $SEDTMP
        		echo "#####################################\\" >> $SEDTMP
			echo "/etc/init.d/%OEM%%Q%-scan start\\" >> $SEDTMP
        		echo "#####################################\\" >> $SEDTMP
			/bin/cat ${SYSINIT} | /bin/sed -f  $SEDTMP > ${SYSINIT}.tmp
			/bin/mv ${SYSINIT}.tmp ${SYSINIT}
			/bin/chmod 755 ${SYSINIT}
			/bin/rm $SEDTMP
    		fi
            # On pre-RHEL7 releases, remove the .service files implemented for RHEL7 and above to avoid confusion (and warning msgs on SuSE)
            /bin/rm -f /etc/init.d/%OEM%%Q%-*.service > /dev/null 2>&1
		;;
	    2.6.18-*)
    		lineno=`/bin/cat ${SYSINIT} | /bin/sed -n -e '/strstr "$cmdline" fastboot/=' | tail -1`
    		if [ -z "${lineno}" ]
    		then
           	  echo "ERROR: ${SYSINIT} context to modify was not found";
    		else
        	  echo "${lineno}i\\" > $SEDTMP
        	  echo "#####################################\\" >> $SEDTMP
		  echo "%Q%rootstate=\`LC_ALL=C awk '/ \\\/ / && (\$3 !~ /rootfs/) { print \$4 }' /proc/mounts\`\\" >> $SEDTMP
		  echo "if [ \"\$%Q%rootstate\" != \"rw\" ] ; then\\" >> $SEDTMP
		  echo "  echo \"Remounting root filesystem in read-write mode: \"\\" >> $SEDTMP
		  echo "  mount -n -o remount,rw /\\" >> $SEDTMP
		  echo "  echo \"Running %OEM%%Q%-scan start: \"\\" >> $SEDTMP
		  echo "  /etc/init.d/%OEM%%Q%-scan start\\" >> $SEDTMP
		  echo "  echo \"Remounting root filesystem in read-only mode: \"\\" >> $SEDTMP
		  echo "  mount -n -o remount,ro /\\" >> $SEDTMP
		  echo "else\\" >> $SEDTMP
		  echo "  echo \"Running %OEM%%Q%-scan start: \"\\" >> $SEDTMP
		  echo "  /etc/init.d/%OEM%%Q%-scan start\\" >> $SEDTMP
		  echo "fi\\" >> $SEDTMP
        	  echo "#####################################\\" >> $SEDTMP
		  /bin/cat ${SYSINIT} | /bin/sed -f  $SEDTMP > ${SYSINIT}.tmp
		  /bin/mv ${SYSINIT}.tmp ${SYSINIT}
		  /bin/chmod 755 ${SYSINIT}
		  /bin/rm $SEDTMP
    		fi
            # On pre-RHEL7 releases, remove the .service files implemented for RHEL7 and above to avoid confusion (and warning msgs on SuSE)
            /bin/rm -f /etc/init.d/%OEM%%Q%-*.service > /dev/null 2>&1
		;;
	    2.6.32-*)
    		lineno=`/bin/cat ${SYSINIT} | /bin/sed -n -e '/strstr "$cmdline" fastboot/=' | tail -1`
    		if [ -z "${lineno}" ]
    		then
           	  echo "ERROR: ${SYSINIT} context to modify was not found";
    		else
        	  echo "${lineno}i\\" > $SEDTMP
        	  echo "#####################################\\" >> $SEDTMP
		  echo "%Q%rootstate=\`LC_ALL=C awk '/ \\\/ / && (\$3 !~ /rootfs/) { print \$4 }' /proc/mounts\`\\" >> $SEDTMP
		  echo "if [ \"\$%Q%rootstate\" != \"rw\" ] ; then\\" >> $SEDTMP
		  echo "  echo \"Remounting root filesystem in read-write mode: \"\\" >> $SEDTMP
		  echo "  mount -n -o remount,rw /\\" >> $SEDTMP
		  echo "  echo \"Running %OEM%%Q%-scan start: \"\\" >> $SEDTMP
		  echo "  /etc/init.d/%OEM%%Q%-scan start\\" >> $SEDTMP
		  echo "  echo \"Remounting root filesystem in read-only mode: \"\\" >> $SEDTMP
		  echo "  mount -n -o remount,ro /\\" >> $SEDTMP
		  echo "else\\" >> $SEDTMP
		  echo "  echo \"Running %OEM%%Q%-scan start: \"\\" >> $SEDTMP
		  echo "  /etc/init.d/%OEM%%Q%-scan start\\" >> $SEDTMP
		  echo "fi\\" >> $SEDTMP
        	  echo "#####################################\\" >> $SEDTMP
		  /bin/cat ${SYSINIT} | /bin/sed -f  $SEDTMP > ${SYSINIT}.tmp
		  /bin/mv ${SYSINIT}.tmp ${SYSINIT}
		  /bin/chmod 755 ${SYSINIT}
		  /bin/rm $SEDTMP
    		fi
            # On pre-RHEL7 releases, remove the .service files implemented for RHEL7 and above to avoid confusion (and warning msgs on SuSE)
            /bin/rm -f /etc/init.d/%OEM%%Q%-*.service > /dev/null 2>&1
		;;
	    3.7.0-*|3.10.0-*)
            # Starting with RHEL 7, rc.sysinit is no longer used, but replaced by the systemd infrastructure.
            # We need to install our service files and enable them to have our scripts called at boot time.
            # Also, check that /etc/modprobe.d/sftkdtc.conf exists; if not, create it
            if [ ! -d /etc/modprobe.d ]
	        then
                /bin/mkdir /etc/modprobe.d
	        fi

            if [ ! -e '/etc/modprobe.d/sftkdtc.conf' ]
            then
	            /usr/bin/touch /etc/modprobe.d/sftkdtc.conf
            fi

		    echo "Installing our %OEM%%Q%-start.service and %OEM%%Q%-startdaemons.service files and enabling them to have our scripts called at boot time"
            /bin/cp -p /etc/init.d/%OEM%%Q%-start.service /etc/systemd/system/.
            /usr/bin/systemctl enable SFTKdtc-start.service
            /bin/cp -p /etc/init.d/%OEM%%Q%-startdaemons.service /etc/systemd/system/.
            /usr/bin/systemctl enable SFTKdtc-startdaemons.service

		    echo "Installing our %OEM%%Q%-startmaster.service and %OEM%%Q%-startpmds.service files and enabling them to have our scripts called at boot time"
            /bin/cp -p /etc/init.d/%OEM%%Q%-startmaster.service /etc/systemd/system/.
            /usr/bin/systemctl enable SFTKdtc-startmaster.service
            /bin/cp -p /etc/init.d/%OEM%%Q%-startpmds.service /etc/systemd/system/.
            /usr/bin/systemctl enable SFTKdtc-startpmds.service

            # Since our startdaemons script becomes a service, we must remove the old startdaemons links from the /etc/rc.d directories
		    echo "Removing the old %OEM%%Q%-startdaemons links from the /etc/rc.d directories"
            /bin/rm -f /etc/rc2.d/S25SFTKdtc-startdaemons > /dev/null 2>&1
            /bin/rm -f /etc/rc3.d/S25SFTKdtc-startdaemons > /dev/null 2>&1
            /bin/rm -f /etc/rc4.d/S25SFTKdtc-startdaemons > /dev/null 2>&1
            /bin/rm -f /etc/rc5.d/S25SFTKdtc-startdaemons > /dev/null 2>&1
		;;
	    *)
		echo "ERROR: Unknown REDHAT kernel ${PCRELEASE} - ${SYSINIT} not modified"
		echo -n "ERROR: "; head -1 /etc/redhat-release
		;;
    esac
    
fi

elif [ -f ${SUSE_DIST} ]
then

    if [ $isSuSE12 -eq 0 ]
    then 
        grep "%PKGNM%-scan" ${LOCALFS} > /dev/null 2>&1
        if [ $? -ne 0 ]
        then
            # save current copy off where it can be restored in case of failure
            echo "Saving current $LOCALFS to ${LOCALFS}.new%CAPQ%"
            /bin/cp -p $LOCALFS ${LOCALFS}.new%CAPQ%
            compare_copy $LOCALFS ${LOCALFS}.new%CAPQ%
            /bin/gzip -f -S .gz.save ${LOCALFS}.new%CAPQ%
            #
            /bin/awk '/^#.*Should-(Stop|Start):/ { print $0 " %PKGNM%-scan"; next; }
	        { print $0; }' ${LOCALFS} >${LOCALFS}.tmp
            /bin/mv ${LOCALFS}.tmp ${LOCALFS}
            /bin/chmod 755 ${LOCALFS}
        fi
        # On pre-RHEL7 and non-RedHat releases (except SuSE 12), remove the .service files implemented for RHEL7 and above to avoid confusion (and warning msgs on SuSE)
        if [ $isSuSE12 -eq 0 ]
        then 
            /bin/rm -f /etc/init.d/%OEM%%Q%-*.service > /dev/null 2>&1
        fi
    else
        # On SuSE 12.x as on RHEL 7, rc.sysinit is no longer used, but replaced by the systemd infrastructure.
        # We need to install our service files and enable them to have our scripts called at boot time.
        # Also, check that /etc/modprobe.d/sftkdtc.conf exists; if not, create it
        if [ ! -d /etc/modprobe.d ]
	    then
            /bin/mkdir /etc/modprobe.d
	    fi

        if [ ! -e '/etc/modprobe.d/sftkdtc.conf' ]
        then
	        /usr/bin/touch /etc/modprobe.d/sftkdtc.conf
        fi

		echo "Installing our %OEM%%Q%-start.service and %OEM%%Q%-startdaemons.service files and enabling them to have our scripts called at boot time"
        /bin/cp -p /etc/init.d/%OEM%%Q%-start.service /etc/systemd/system/.
        /usr/bin/systemctl enable SFTKdtc-start.service
        /bin/cp -p /etc/init.d/%OEM%%Q%-startdaemons.service /etc/systemd/system/.
        /usr/bin/systemctl enable SFTKdtc-startdaemons.service

		echo "Installing our %OEM%%Q%-startmaster.service and %OEM%%Q%-startpmds.service files and enabling them to have our scripts called at boot time"
        /bin/cp -p /etc/init.d/%OEM%%Q%-startmaster.service /etc/systemd/system/.
        /usr/bin/systemctl enable SFTKdtc-startmaster.service
        /bin/cp -p /etc/init.d/%OEM%%Q%-startpmds.service /etc/systemd/system/.
        /usr/bin/systemctl enable SFTKdtc-startpmds.service

        # Since our startdaemons script becomes a service, we must remove the old startdaemons links from the /etc/rc.d directories
		echo "Removing the old %OEM%%Q%-startdaemons links from the /etc/rc.d directories"
        /bin/rm -f /etc/rc2.d/S25SFTKdtc-startdaemons > /dev/null 2>&1
        /bin/rm -f /etc/rc3.d/S25SFTKdtc-startdaemons > /dev/null 2>&1
        /bin/rm -f /etc/rc4.d/S25SFTKdtc-startdaemons > /dev/null 2>&1
        /bin/rm -f /etc/rc5.d/S25SFTKdtc-startdaemons > /dev/null 2>&1
    fi
fi

# NOTE: on RHEL 7 and SuSE 12.x there is no /etc/init.d/halt file; then we install our SFTKdtc-stop.service file which contains the instructions
# to have our SFTKdtc-scan script called at shutdown.
if [ -f ${HALT} ]
then
    grep "/sbin/%Q%stop" ${HALT} > /dev/null 2>&1
    if [ $? -ne 0 ]
    then
        # save current copy off where it can be restored in case of failure
        echo "Saving current $HALT to ${HALT}.new%CAPQ%"
        /bin/cp -p $HALT ${HALT}.new%CAPQ%
        compare_copy $HALT ${HALT}.new%CAPQ%
        /bin/gzip -f -S .gz.save ${HALT}.new%CAPQ%
        #
        # Regexp for SuSE: .*umsdos[ \t]*fs[ \t].*
        # Regexp for Red Hat: .*Remount[ \t]*read[ \t].*
        /bin/awk '/.*umsdos[ \t]*fs[ \t].*|.*Remount[ \t]*read[ \t].*/ {
	        print "########################################################";
	        print "echo \"Stopping %COMPANYNAME2% %PRODUCTNAME% Groups\"";
	        print "### Issue two syncs because 2nd will not start until first is finished";
	        print "/bin/sync;/bin/sync";
	        print "/sbin/%Q%stop -sa";
	        print "/bin/sync;/bin/sync";
	        print "########################################################";
	        print $0; next; }
	        {  print $0; }' ${HALT} >${HALT}.tmp
        /bin/mv ${HALT}.tmp ${HALT}
        /bin/chmod 755 ${HALT}
    fi
else
    # We need to install our service file and enable it to have our scripts called at shutdown time.
	echo "Installing our %OEM%%Q%-stop.service file and enabling it to have our scripts called at shutdown time"
    /bin/cp -p /etc/init.d/%OEM%%Q%-stop.service /etc/systemd/system/.
    /usr/bin/systemctl enable SFTKdtc-stop.service
    # NOTE: we must start the SFTKdtc-stop service at installation so that our scan script will be called at the first shutdown
    # after install; the way our stop service is defined is that it does nothing upon start but remains active in systemd,
    # and when a shutdown occurs, since our stop service has an ExecStop directive, it will be called by the shutdown sequence.
    /usr/bin/systemctl start SFTKdtc-stop.service
fi
#Now install our daemons/services
PCRELEASE=`/bin/uname -r`
case "$PCRELEASE" in
	3.7.0-*|3.10.0-*|3.12.28-*|3.12.49-*)
        /sbin/chkconfig --add %OEM%%Q%-chkpt_boot
        /sbin/chkconfig --add %OEM%%Q%-startagent
	;;
	*)
        /sbin/chkconfig --add %OEM%%Q%-startdaemons
        /sbin/chkconfig --add %OEM%%Q%-chkpt_boot
        /sbin/chkconfig --add %OEM%%Q%-startagent
	;;
esac

if [ -f ${SUSE_DIST} ]
then
    /sbin/chkconfig --add %OEM%%Q%-scan
fi

# If RHEL7 or SuSE 12.x, run the startdaemons script and then launch the master daemon and the PMDs because
# startdaemons does not do it on RHEL7 or SUSE 12.x
case "$PCRELEASE" in
	3.7.0-*|3.10.0-*|3.12.28-*|3.12.49-*)
        cd /
        /etc/init.d/%OEM%%Q%-startdaemons start
        cd /var/run/%PKGNM%
		echo "Launching /%OPTDIR%/%PKGNM%/bin/in.%Q%"
        /usr/bin/nohup /%OPTDIR%/%PKGNM%/bin/in.%Q% ${LOGOPT} >/tmp/in.%Q%.log 2>&1 &
        /bin/sleep 5
        /usr/bin/nohup /%OPTDIR%/%PKGNM%/bin/launchpmds >/tmp/in.pmd.log 2>&1 &
	;;
	*)
        (cd / ; /etc/init.d/%OEM%%Q%-startdaemons start)
	;;
esac

exit 0

%preun
# Pre-removal script for linux 
compare_copy() {
    if ! cmp -s $1 $2
    then
        echo "ERROR: Backup copy of \"$1\" to \"$2\" failed."
	exit 1;
    fi
    return 0
}

# Kill any daemons
BASEDIR=/opt
VERSION=%VERSION%
PKGINST=%OEM%%Q%
$BASEDIR/$PKGINST/bin/killagent
$BASEDIR/$PKGINST/bin/killpmds
$BASEDIR/$PKGINST/bin/killrmds
$BASEDIR/$PKGINST/bin/killrefresh
$BASEDIR/$PKGINST/bin/kill%Q%master
$BASEDIR/$PKGINST/bin/%Q%stop -a

# Check running
$BASEDIR/%PKGNM%/bin/%Q%info -a 1> /dev/null 2>&1
if [ $? -eq 0 ]
then
	echo
	echo "ERROR : All the groups couldn't be stopped."
	echo
	exit 1
fi

rmmod  %MODULENAME% >/dev/null 2>&1

# See if the driver is currently loaded
a=`/sbin/lsmod | /bin/egrep -c /%MODULENAME%/`
if [ $a != "0" ]; then
        echo
        echo "ERROR: The %MODULENAME% device driver is still loaded into the kernel. This"
        echo "package cannot be removed until the %MODULENAME% driver is unloaded. Before"
        echo "attempting another rpm -e, do the following:"
        echo "     ""1. Make sure that no %Q% devices are currently mounted"
        echo "     ""2. Remove the %MODULENAME% driver by typing \"rmmod %MODULENAME%\"".
        echo 
        exit 1
fi

# Check for references in fstab, there could be many many others
a=`/bin/grep -c /%Q%/ /etc/fstab`
if [ $a != "0" ]; then
        echo
        echo "*******************************************************************"
        echo "Warning: There are references to %Q% devices in /etc/fstab."
        echo "These should be removed before the next reboot.  There may be"
        echo "references to %Q% devices in other files created by a system"
        echo "administrator that need to be removed."
        echo "*******************************************************************"
        echo 
fi

# get rid of any residual .cur files
if [ "/etc/opt/%OEM%%Q%/*.cur" != "" ]; then
    /bin/rm /etc/opt/%OEM%%Q%/*.cur > /dev/null 2>&1
fi

# Check if any .cfg files exist
dobackup=0
a=/etc/opt/%OEM%%Q%/[ps]???.cfg
if [ `echo $a | wc -w` == 1 ]; then
        if [ -f $a ];then
                dobackup=1
        fi
else
        dobackup=1
fi
if [ $dobackup == 1 ]; then
        # Create /var/opt/%OEM%%Q%, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%" ]; then
                mkdir /var/opt/%OEM%%Q%
        fi

        # Create /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%/%OEM%%Q%$VERSION" ]; then
                mkdir /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        fi

        # if any prior config files exist in save directory, nukem
        b=/var/opt/%OEM%%Q%/%OEM%%Q%${VERSION}/[ps]???.cfg
	if [ `echo $b | wc -w` == 1 ]; then
		if [ -f $b ];then
                	/bin/rm -f $b
        	fi
	else
               	/bin/rm -f $b
	fi

        echo Moving %OEM%%Q% Config files to /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        for i in $a
        do
                /bin/mv $i /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION/
        done
fi

# Check if any .sh files exist
dobackup=0
a=/etc/opt/%OEM%%Q%/*.sh
if [ `echo $a | wc -w` == 1 ]; then
        if [ -f $a ];then
                dobackup=1
        fi
else
        dobackup=1
fi
if [ $dobackup == 1 ]; then
        # Create /var/opt/%OEM%%Q%, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%" ]; then
                mkdir /var/opt/%OEM%%Q%
        fi

        # Create /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%/%OEM%%Q%$VERSION" ]; then
                mkdir /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        fi

        # if any prior .sh files exist in save directory, nukem
        b=/var/opt/%OEM%%Q%/%OEM%%Q%${VERSION}/*.sh
	if [ `echo $b | wc -w` == 1 ]; then
		if [ -f $b ];then
                	/bin/rm -f $b
        	fi
	else
               	/bin/rm -f $b
	fi

        echo Moving %OEM%%Q% Shell files to /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        for i in $a
        do
                # We do not want to backup and restore the dtc_Linux_bootdrive_post_failover script
                # as it may change from one release to another
                echo $i | /bin/grep dtc_Linux_bootdrive_post_failover 1> /dev/null 2>&1
                if [ $? -ne 0 ]
                then
                    /bin/mv $i /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION/
                fi
        done
fi

# Check if any .lic files exist
a=/etc/opt/%OEM%%Q%/%CAPQ%.lic
if [ -f $a ]; then
        # Create /var/opt/%OEM%%Q%, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%" ]; then
                mkdir /var/opt/%OEM%%Q%
        fi

        # Create /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%/%OEM%%Q%$VERSION" ]; then
                mkdir /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        fi

        # if any prior %CAPQ%.lic file exist in save directory, nukem
        b=/var/opt/%OEM%%Q%/%OEM%%Q%${VERSION}/%CAPQ%.lic

        if [ -f $b ]; then
                /bin/rm -f $b
        fi

        echo Moving %OEM%%Q% License files to /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        for i in $a
        do
                /bin/mv $i /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION/
        done
fi

# Backup pxxx_migration_tracking csv and chk files
# Check if any pxxx_migration_tracking.csv files exist (product usage tracking files)
dobackup=0
a=/var/opt/%OEM%%Q%/p???_migration_tracking.csv
if [ `echo $a | wc -w` == 1 ]; then
        if [ -f $a ];then
                dobackup=1
        fi
else
        dobackup=1
fi
if [ $dobackup == 1 ]; then
        # Create /var/opt/%OEM%%Q%, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%" ]; then
                mkdir /var/opt/%OEM%%Q%
        fi

        # Create /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%/%OEM%%Q%$VERSION" ]; then
                mkdir /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        fi

    	date_suffix=`/bin/date +\%Y\%m\%d-\%H\%M`
        echo Moving %OEM%%Q% Product usage statistics files to /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        for i in $a
        do
                /bin/mv $i $i.$date_suffix
                /bin/mv $i.$date_suffix /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION/
        done
fi

# Check if any pxxx_migration_tracking.chk files exist (product usage checksum files)
dobackup=0
a=/var/opt/%OEM%%Q%/p???_migration_tracking.chk
if [ `echo $a | wc -w` == 1 ]; then
        if [ -f $a ];then
                dobackup=1
        fi
else
        dobackup=1
fi
if [ $dobackup == 1 ]; then
        # Create /var/opt/%OEM%%Q%, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%" ]; then
                mkdir /var/opt/%OEM%%Q%
        fi

        # Create /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%/%OEM%%Q%$VERSION" ]; then
                mkdir /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        fi

        echo Moving %OEM%%Q% Product usage checksum files to /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
    	date_suffix=`/bin/date +\%Y\%m\%d-\%H\%M`
        for i in $a
        do
                /bin/mv $i $i.$date_suffix
                /bin/mv $i.$date_suffix /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION/
        done
fi

# Check if any global product_usage_stats csv files exist (product usage global tracking files of this server)
dobackup=0
a=/var/opt/%OEM%%Q%/*_product_usage_stats_*.csv
if [ `echo $a | wc -w` == 1 ]; then
        if [ -f $a ];then
                dobackup=1
        fi
else
        dobackup=1
fi
if [ $dobackup == 1 ]; then
        # Create /var/opt/%OEM%%Q%, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%" ]; then
                mkdir /var/opt/%OEM%%Q%
        fi

        # Create /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%/%OEM%%Q%$VERSION" ]; then
                mkdir /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        fi

        echo Moving %OEM%%Q% global product_usage_stats files to /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        for i in $a
        do
                /bin/mv $i /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION/
        done
fi

# Check if devlist.conf files exist
a=/etc/opt/%OEM%%Q%/devlist.conf
if [ -f $a ]; then
        # Create /var/opt/%OEM%%Q%, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%" ]; then
                mkdir /var/opt/%OEM%%Q%
        fi

        # Create /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%/%OEM%%Q%$VERSION" ]; then
                mkdir /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        fi

        # if any prior devlist.conf file exist in save directory, nukem
        b=/var/opt/%OEM%%Q%/%OEM%%Q%${VERSION}/devlist.conf

        if [ -f $b ]; then
                /bin/rm -f $b
        fi

        echo Moving %OEM%%Q% device list file to /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        for i in $a
        do
                /bin/mv $i /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION/
        done
fi

# Check if the %Q%.conf file exists
if [ -f /etc/opt/%OEM%%Q%/driver/%Q%.conf ]; then
        # Create /var/opt/%OEM%%Q%, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%" ]; then
                mkdir /var/opt/%OEM%%Q%
        fi

        # Create /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION, if necessary
        if [ ! -d "/var/opt/%OEM%%Q%/%OEM%%Q%$VERSION" ]; then
                mkdir /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        fi

        # if any prior %CAPQ%.conf file exist in save directory, nukem
        b=/var/opt/%OEM%%Q%/%OEM%%Q%${VERSION}/%Q%.conf
        if [ -f $b ]; then
                /bin/rm -f $b
        fi

        echo Moving the %Q%.conf file to /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
        /bin/mv /etc/opt/%OEM%%Q%/driver/%Q%.conf /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION/.
fi

# Check if %Q%Agent.cfg file exist
if [ -f /etc/opt/%OEM%%Q%/%Q%Agent.cfg ]; then
    # Create /var/opt/%OEM%%Q%, if necessary
    if [ ! -d "/var/opt/%OEM%%Q%" ]; then
        mkdir /var/opt/%OEM%%Q%
    fi

    # Create /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION, if necessary
    if [ ! -d "/var/opt/%OEM%%Q%/%OEM%%Q%$VERSION" ]; then
        mkdir /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
    fi

    # if any prior Agent config file exist in save directory, nukem
    b=/var/opt/%OEM%%Q%/%OEM%%Q%${VERSION}/%Q%Agent.cfg
    if [ -f $b ]; then
        /bin/rm -f $b
    fi

    echo Moving %OEM%%Q% Agent Config files to /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION
    /bin/mv /etc/opt/%OEM%%Q%/%Q%Agent.cfg /var/opt/%OEM%%Q%/%OEM%%Q%$VERSION/
fi

    PCRELEASE=`/bin/uname -r`
    case "$PCRELEASE" in
	    3.7.0-*|3.10.0-*|3.12.28-*|3.12.49-*)
            /sbin/chkconfig --del %OEM%%Q%-startagent
            /sbin/chkconfig --del %OEM%%Q%-chkpt_boot
		;;
	    *)
            /sbin/chkconfig --del %OEM%%Q%-startagent
            /sbin/chkconfig --del %OEM%%Q%-startdaemons
            /sbin/chkconfig --del %OEM%%Q%-chkpt_boot
		;;
    esac
if [ -f ${SUSE_DIST} ]
then
    /sbin/chkconfig --del %OEM%%Q%-scan
fi

# Disable and remove the systemd service files if they exist
if [ -f /etc/systemd/system/%OEM%%Q%-start.service ]
then
    echo Disabling and removing %OEM%%Q%-start.service
    /usr/bin/systemctl disable %OEM%%Q%-start.service
    /bin/rm /etc/systemd/system/%OEM%%Q%-start.service
fi
if [ -f /etc/systemd/system/%OEM%%Q%-stop.service ]
then
    echo Disabling and removing %OEM%%Q%-stop.service
    /usr/bin/systemctl disable %OEM%%Q%-stop.service
    /bin/rm /etc/systemd/system/%OEM%%Q%-stop.service
fi
if [ -f /etc/systemd/system/%OEM%%Q%-startdaemons.service ]
then
    echo Disabling and removing %OEM%%Q%-startdaemons.service
    /usr/bin/systemctl disable %OEM%%Q%-startdaemons.service
    /bin/rm /etc/systemd/system/%OEM%%Q%-startdaemons.service
fi
if [ -f /etc/systemd/system/%OEM%%Q%-startmaster.service ]
then
    echo Disabling and removing %OEM%%Q%-startmaster.service
    /usr/bin/systemctl disable %OEM%%Q%-startmaster.service
    /bin/rm /etc/systemd/system/%OEM%%Q%-startmaster.service
fi
if [ -f /etc/systemd/system/%OEM%%Q%-startpmds.service ]
then
    echo Disabling and removing %OEM%%Q%-startpmds.service
    /usr/bin/systemctl disable %OEM%%Q%-startpmds.service
    /bin/rm /etc/systemd/system/%OEM%%Q%-startpmds.service
fi

exit 0

%postun
BASEDIR=/opt
PKGINST=%OEM%%Q%
SERVICES=/etc/services
MODCONF=/etc/modprobe.d/sftkdtc.conf

REDHAT_DIST=/etc/redhat-release
SUSE_DIST=/etc/SuSE-release

if [ -f ${REDHAT_DIST} ]
then
SYSINIT=/etc/rc.d/rc.sysinit
elif [ -f ${SUSE_DIST} ]
then
LOCALFS=/etc/init.d/boot.localfs
fi

HALT=/etc/init.d/halt

killproc() {            # kill the named process(es)
    pid=`/bin/ps -e |
             /bin/grep -w $1 |
             /bin/sed -e 's/^  *//' -e 's/ .*//'`
        [ "$pid" != "" ] && /bin/kill $2 $pid
}

compare_copy() {
    if ! cmp -s $1 $2
    then
        echo "ERROR: Backup copy of \"$1\" to \"$2\" failed."
	exit 1;
    fi
    return 0
}


#
# remove soft link for dtcautomount
#
rm -f ${BASEDIR}/%OEM%%Q%/bin/%Q%automount

FTDLINKS="%Q%checkpoint %Q%configtool %Q%debugcapture %Q%hostinfo %Q%info %Q%init %Q%killpmd %Q%killrefresh %Q%killrmd %Q%licinfo %Q%monitortool %Q%override %Q%perftool %Q%pmd %Q%refresh %Q%rmdreco %Q%set %Q%start %Q%stop in.%Q% in.pmd in.rmd kill%Q%master killpmds killrefresh killrmds launch%Q%master launchpmds launchrefresh throtd %Q%modfs %Q%automount in.%QAGN% %Q%agentset launchagent killagent %Q%psmigrate %Q%panalyze launchnetanalysis %Q%netanalysis %Q%stopnetanalysis %Q%psmigrate272"
FTDLINKS="$FTDLINKS %Q%monitortty %Q%hrdb_maths %Q%failover %Q%sendfailover"
# Note: launchbackfresh, killbackfresh, %Q%backfresh, %Q%killbackfresh deactivated as of TDMF IP 2.8.0
#
# dynamic volume expansion feature disabled
# %Q%reconfig %Q%expand %Q%limitsize
#
# delete the links in /usr/local/bin
#
if [ -d "/usr/local/bin" -a -w "/usr/local/bin" ]; then
    echo "Removing %PRODUCTNAME% Symbolic Links from /usr/local/bin"
    for LINK in $FTDLINKS
    do
        /bin/rm -f /usr/local/bin/${LINK}
    done
fi
echo "Removing %PRODUCTNAME% device tree: /dev/%Q%"
/bin/rm -rf /dev/%Q%

/bin/sleep 5
# delete /var/opt/%OEM%%Q%/Agn_tmp directory
b=/var/opt/%OEM%%Q%/Agn_tmp
if [ -d $b ]; then
    /bin/rm -fr ${b} > /dev/null 2>&1
fi

echo "Removing temporary files from /var/opt/%OEM%%Q%"
/bin/rm -f "/var/opt/%OEM%%Q%/*" > /dev/null 2>&1

if [ "$BASEDIR" != "/opt" ]; then
        /bin/rm -f /opt/$PKGINST
fi

echo "Removing core files from /var/run/%OEM%%Q%"
/bin/rm -fr "/var/run/%OEM%%Q%" > /dev/null 2>&1

echo "Removing %PRODUCTNAME% master daemon from $SERVICES"
/bin/cp -p $SERVICES $SERVICES.%Q%
compare_copy $SERVICES $SERVICES.%Q%
if [ $? -eq 0 ]; then
    egrep -v 'in\.%Q%' $SERVICES.%Q% > $SERVICES
fi

grep %MODULENAME% ${MODCONF} > /dev/null 2>&1
if [ $? -eq 0 ]
then
    # save current copy off where it can be restored in case of failure
    echo "Saving current $MODCONF to ${MODCONF}.pre_%Q%_remove"
    /bin/cp -p $MODCONF ${MODCONF}.pre_%Q%_remove
    compare_copy $MODCONF ${MODCONF}.pre_%Q%_remove
    #
    lineno=`/bin/cat ${MODCONF} | /bin/sed -n -e '/%MODULENAME%/='`
    if [ -z "${lineno}" ]
    then
        echo "${MODCONF} context to modify not found";
    else
        echo "Removing %PRODUCTNAME% modifications from ${MODCONF}"
        /bin/cat ${MODCONF} | /bin/sed "${lineno} d" > ${MODCONF}.tmp
        /bin/mv ${MODCONF}.tmp ${MODCONF}
    fi
fi

if [ -f ${REDHAT_DIST} ]
then
grep "%PKGNM%-scan" ${SYSINIT} > /dev/null 2>&1
if [ $? -eq 0 ]
then
    # save current copy off where it can be restored in case of failure
    echo "Saving current $SYSINIT to ${SYSINIT}.pre_%Q%_remove"
    /bin/cp -p $SYSINIT ${SYSINIT}.pre_%Q%_remove
    compare_copy $SYSINIT ${SYSINIT}.pre_%Q%_remove
    /bin/gzip -f -S .gz.save ${SYSINIT}.pre_%Q%_remove
    #
    lineno=`/bin/cat ${SYSINIT} | /bin/sed -n -e '/%PKGNM%-scan/=' | tail -1`
    if [ -z "${lineno}"  ]
    then
        echo "${SYSINIT} context to modify not found";
    else
	PCRELEASE=`/bin/uname -r`
	
	case "$PCRELEASE" in
	    2.6.9-*)
		beginline=`expr $lineno - 1`
		endline=`expr $lineno + 2`
		;;
	    2.6.18-*)
		beginline=`expr $lineno - 11`
		endline=`expr $lineno + 3`
		;;
	    2.6.32-*)
		beginline=`expr $lineno - 11`
		endline=`expr $lineno + 3`
		;;
	    *)
		echo "ERROR: Unknown REDHAT kernel ${PCRELEASE} - ${SYSINIT} not reset"
		echo -n "ERROR: "; head -1 /etc/redhat-release
		begline=""
		;;
	esac

	if [ -n "${beginline}" ]
	then
	    echo "Removing %PRODUCTNAME% modifications from ${SYSINIT}"
	    /bin/cat ${SYSINIT} | /bin/sed "${beginline},${endline} d" > ${SYSINIT}.tmp
	    /bin/mv ${SYSINIT}.tmp ${SYSINIT}
	    /bin/chmod 755 ${SYSINIT}
	fi
    fi
fi

elif [ -f ${SUSE_DIST} ]
then

/bin/grep "%PKGNM%-scan" ${LOCALFS}  > /dev/null 2>&1
if [ $? -eq 0 ]
then
    # save current copy off where it can be restored in case of failure
    echo "Saving current $LOCALFS to ${LOCALFS}.pre_%Q%_remove"
    /bin/cp -p $LOCALFS ${LOCALFS}.pre_%Q%_remove
    compare_copy $LOCALFS ${LOCALFS}.pre_%Q%_remove
    /bin/gzip -f -S .gz.save ${LOCALFS}.pre_%Q%_remove
    #
    echo "Removing %PRODUCTNAME% modifications from ${LOCALFS}"
    /bin/cat ${LOCALFS} | /bin/sed "s/ %PKGNM%-scan//g" > ${LOCALFS}.tmp
    /bin/mv ${LOCALFS}.tmp ${LOCALFS}
    /bin/chmod 755 ${LOCALFS}
fi
fi

/bin/grep "/sbin/%Q%stop" ${HALT} > /dev/null 2>&1
if [ $? -eq 0 ]
then
    # save current copy off where it can be restored in case of failure
    echo "Saving current $HALT to ${HALT}.pre_%Q%_remove"
    /bin/cp -p $HALT ${HALT}.pre_%Q%_remove
    compare_copy $HALT ${HALT}.pre_%Q%_remove
    /bin/gzip -f -S .gz.save ${HALT}.pre_%Q%_remove
    #
    lineno=`/bin/cat ${HALT} | /bin/sed -n -e '/sbin\/%Q%stop/=' | tail -1`
    if [ -z "${lineno}" ]
    then
        echo "${HALT} context to modify not found";
    else
        beginline=`expr $lineno - 4`
        endline=`expr $lineno + 2`
        echo "Removing %PRODUCTNAME% modifications from ${HALT}"
        /bin/cat ${HALT} | /bin/sed "${beginline},${endline} d" > ${HALT}.tmp
        /bin/mv ${HALT}.tmp ${HALT}
	/bin/chmod 755 ${HALT}
    fi
fi

# WR PROD7010: cleanup /etc/opt/%OEM%%Q% and /opt/%OEM%%Q% directories
    echo "Cleaning up /etc/opt/%OEM%%Q% and /opt/%OEM%%Q%"
    /bin/rm -fr	/etc/opt/%OEM%%Q%
    /bin/rm -fr	/opt/%OEM%%Q%

exit 0

%files
#startup scripts
%dir %attr(0755,root,root) /etc/opt/%OEM%%Q%
%attr(0444,root,root) /etc/opt/%OEM%%Q%/errors.msg
%attr(0444,root,root) /etc/opt/%OEM%%Q%/readme.txt
%attr(0644,root,root) /etc/opt/%OEM%%Q%/DTC.lic.perm
%attr(0666,root,root) /etc/opt/%OEM%%Q%/%Q%Tracking_Resolutions.txt
%attr(0766,root,root) /etc/opt/%OEM%%Q%/dtc_pre_failover_pxxx.sh
%attr(0766,root,root) /etc/opt/%OEM%%Q%/dtc_post_failover_pxxx.sh
%attr(0766,root,root) /etc/opt/%OEM%%Q%/dtc_post_failover_sxxx.sh
%attr(0766,root,root) /etc/opt/%OEM%%Q%/dtc_Linux_bootdrive_post_failover.sh
%attr(0555,root,root) /etc/init.d/%OEM%%Q%-startdaemons
%attr(0555,root,root) /etc/init.d/%OEM%%Q%-chkpt_boot
%attr(0555,root,root) /etc/init.d/%OEM%%Q%-startagent
%attr(0555,root,root) /etc/init.d/%OEM%%Q%-scan
%attr(0444,root,root) /etc/init.d/%OEM%%Q%-start.service
%attr(0444,root,root) /etc/init.d/%OEM%%Q%-stop.service
%attr(0444,root,root) /etc/init.d/%OEM%%Q%-startdaemons.service
%attr(0444,root,root) /etc/init.d/%OEM%%Q%-startmaster.service
%attr(0444,root,root) /etc/init.d/%OEM%%Q%-startpmds.service
#pmd rmd  in.%Q% daemons
%dir %attr(0755,root,root) /var/run/%OEM%%Q%
%dir %attr(0755,root,root) /var/opt/%OEM%%Q%
%dir %attr(0755,root,root) /var/opt/%OEM%%Q%/Agn_tmp
%attr(0555,root,root) /opt/%OEM%%Q%/bin/in.pmd
%attr(0555,root,root) /opt/%OEM%%Q%/bin/in.rmd
%attr(0555,root,root) /opt/%OEM%%Q%/bin/in.%Q%
#mirroring commands
%dir %attr(0755,root,root) /opt/%OEM%%Q%
%dir %attr(0755,root,root) /opt/%OEM%%Q%/bin
# %attr(0555,root,root) /opt/%OEM%%Q%/bin/killbackfresh
%attr(0555,root,root) /opt/%OEM%%Q%/bin/kill%Q%master
%attr(0555,root,root) /opt/%OEM%%Q%/bin/killpmds
%attr(0555,root,root) /opt/%OEM%%Q%/bin/killrefresh
%attr(0555,root,root) /opt/%OEM%%Q%/bin/killrmds
# %attr(0555,root,root) /opt/%OEM%%Q%/bin/launchbackfresh
%attr(0555,root,root) /opt/%OEM%%Q%/bin/launch%Q%master
%attr(0555,root,root) /opt/%OEM%%Q%/bin/launchpmds
%attr(0555,root,root) /opt/%OEM%%Q%/bin/launchrefresh
%attr(0555,root,root) /opt/%OEM%%Q%/bin/launchnetanalysis
#%attr(0555,root,root) /opt/%OEM%%Q%/bin/updatepmds 
%attr(0555,root,root) /opt/%OEM%%Q%/bin/throtd
# %attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%backfresh
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%configtool
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%info
# %attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%killbackfresh
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%killpmd
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%killrmd
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%killrefresh
#%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%updatepmd
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%monitortool
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%monitortty
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%hrdb_maths
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%failover
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%perftool
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%pmd
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%psmigrate
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%psmigrate272
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%panalyze
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%refresh
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%netanalysis
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%stopnetanalysis
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%rmdreco
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%debugcapture
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%hostinfo
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%init
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%licinfo
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%override
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%start
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%stop
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%set
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%checkpoint
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%sendfailover
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%modfs
#%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%reconfig
#%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%expand
#%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%limitsize
%attr(0555,root,root) /opt/%OEM%%Q%/bin/in.%QAGN%
%attr(0555,root,root) /opt/%OEM%%Q%/bin/%Q%agentset
%attr(0555,root,root) /opt/%OEM%%Q%/bin/killagent
%attr(0555,root,root) /opt/%OEM%%Q%/bin/launchagent
#pdf's removed from makefile build process
#%dir %attr(0755,root,root) /opt/%OEM%%Q%/doc
#%dir %attr(0755,root,root) /opt/%OEM%%Q%/doc/pdf
#%doc %attr(0644,root,root) /opt/%OEM%%Q%/doc/pdf/AdminGuide.pdf
#%doc %attr(0644,root,root) /opt/%OEM%%Q%/doc/pdf/InstallGuide.pdf
#%doc %attr(0644,root,root) /opt/%OEM%%Q%/doc/pdf/MessageGuide.pdf
%dir %attr(0755,root,root) /opt/%OEM%%Q%/lib
#lib/%Q%%REV%
%dir %attr(0755,root,root) /opt/%OEM%%Q%/lib/%Q%%REV%
%attr(0444,root,root) /opt/%OEM%%Q%/lib/%Q%%REV%/%Q%confutil.tcl
%attr(0444,root,root) /opt/%OEM%%Q%/lib/%Q%%REV%/%CAPQ%logo202.gif
%attr(0444,root,root) /opt/%OEM%%Q%/lib/%Q%%REV%/%CAPQ%logo47.gif
%attr(0444,root,root) /opt/%OEM%%Q%/lib/%Q%%REV%/%CAPQ%logo47r.gif
#%attr(0444,root,root) /opt/%OEM%%Q%/lib/%Q%%REV%/copyright.txt
%attr(0444,root,root) /opt/%OEM%%Q%/lib/%Q%%REV%/%Q%migratecfg.tcl
#%dir %attr(0755,root,root) /opt/%OEM%%Q%/libexec
#%attr(0555,root,root) /opt/%OEM%%Q%/libexec/%Q%wish
%dir %attr(0755,root,root) /opt/%OEM%%Q%/samples
%attr(0444,root,root) /opt/%OEM%%Q%/samples/p000.cfg.sample
%attr(0444,root,root) /opt/%OEM%%Q%/samples/%Q%.conf
%attr(0444,root,root) /opt/%OEM%%Q%/samples/chlink
%dir %attr(0755,root,root) /etc/opt/%OEM%%Q%/driver
<<driver modules>>
%config %attr(0444,root,root) /etc/opt/%OEM%%Q%/driver/%Q%.conf
%attr(0555,root,root) /sbin/%Q%start
%attr(0555,root,root) /sbin/%Q%stop
%attr(0555,root,root) /usr/sbin/%Q%start
%attr(0555,root,root) /usr/sbin/%Q%stop

%changelog
