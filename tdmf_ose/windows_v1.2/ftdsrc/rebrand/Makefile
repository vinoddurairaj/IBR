#
# execute OEM rebrand scripts 
#

#
# default build sequence number
#
seqnum=000

dtc: 	
	echo "Making DTC brand"
	brand_mk.sh -b DTCBRAND -n ${seqnum}
	brand_wish.sh -b DTCBRAND
	brand_installp.sh -b DTCBRAND
	brand_pkginst.sh 

stk:	
	echo "Making STK brand"
	brand_mk.sh -b STKBRAND -n ${seqnum}
	brand_wish.sh -b STKBRAND
	brand_installp.sh -b STKBRAND
	brand_pkginst.sh 

mti:	
	echo "Making MTI brand"
	brand_mk.sh -b MTIBRAND -n ${seqnum}
	brand_wish.sh -b MTIBRAND
	brand_installp.sh -b MTIBRAND
	brand_pkginst.sh 
