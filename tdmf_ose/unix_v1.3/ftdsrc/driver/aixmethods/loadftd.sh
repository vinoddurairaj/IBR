#!/bin/sh

# create/show installation/configuration
# states of the driver device object class.
# load and configure the driver or unconfigure 
# and unload the driver

methodsdir=.
#methodsdir=/etc/ftd/methods
devclass="ftd_class"
devsubclass="ftd_subclass"
devtype="ftd"
objtyp=${devclass}/${devsubclass}/${devtype}
line="--------------------------------------------------------------"

Usg() {
echo "Usage: $0 [ -l | -u ]";
echo " -l load driver"
echo " -u unload driver"
}

mode=""
while getopts ul name
do
        case ${name} in
        l)      mode="Load";;
        u)      mode="Unload";;
        ?)      Usg; exit 2;;
        esac
done
if [ "xx${mode}" = "xx" ]
then
	Usg;
	exit 1;
fi
echo "${mode}ing ${objtyp}"

if [ "${mode}" = "Load" ]
then

	# make the predefined device object class if it doesn't exist.
	echo ${line}
	stat=`lsdev -P -c ${devclass} -s ${devsubclass} -t ${devtype} |\
	      wc -c | awk '{print $1}'`
	if [ ${stat} -eq 0 ]
	then
	  echo "Predefined device object class"
	  echo "  ${objtyp} not found:"
	  echo ""
	  echo "  Creating ${objtyp} device object class."
	  odmadd ${methodsdir}/ftd.add
	else
	  echo "Predefined driver object class"
	  echo "  ${objtyp} found:"
	  lsdev -P -H -c ${devclass} -s ${devsubclass} -t ${devtype} 
	fi
	echo ""
	echo "Class definition:"
	${methodsdir}/ftd.get
	echo ${line}

	# make an instance of the predefined device object class one doesn't exist.
	stat=`lsdev -C -c ${devclass} -s ${devsubclass} -t ${devtype} | wc -c`
	if [ ${stat} -eq 0 ]
	then
	  echo "Custom device object class"
	  echo "  ${devclass}/${devsubclass}/${devtype} not found."
	  echo ""
	  echo "Defining an instance:"
	  if [ -x ${methodsdir}/defftd ]
	  then
	    inst=`${methodsdir}/defftd -c ${devclass} \
		                       -s ${devsubclass} \
		                       -t ${devtype}`
	    echo "created custom device instance: ${inst}"
	  else
		"${methodsdir}/defftd not found or incorrect modes."
		exit 1;
	
	  fi
	else
		inst=`lsdev -F "name" -C -c ftd_class -s ftd_subclass -t ftd`
		echo "Custom driver object class"
		echo "  ${devclass}/${devsubclass}/${devtype} found:"
		echo "  instances:"
	fi
	echo ""
	lsdev -H -C -c ${devclass} -s ${devsubclass} -t ${devtype} 

	# now attempt to configure the driver
	echo "Configuring ${inst}."
	if [ -x ${methodsdir}/cfgftd ]
	then
		${methodsdir}/cfgftd -l ${inst} -2
	else
		"${methodsdir}/cfgftd not found or incorrect modes."
		exit 1;
	fi
fi
exit 0;
