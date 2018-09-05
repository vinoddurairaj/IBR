#
# execute OEM rebrand scripts 
#

#
# default build sequence number
#

SYSTYPE:=${shell uname}
ifeq (SunOS,${SYSTYPE})
SHELL=/usr/bin/ksh
endif

rep: 	
	@echo "Making Replicator brand"
	brand_mk.sh -b REPLBRAND $${FIXNUM+-f "$${FIXNUM}"} $${BUILD+-n "$${BUILD}"} $${BUILDPREFIX+-p "$${BUILDPREFIX}"} $${BUILDSUFFIX+-s "$${BUILDSUFFIX}"}
	brand_wish.sh -b REPLBRAND
ifeq (AIX,${SYSTYPE})
	brand_installp.sh -b REPLBRAND
endif

tdmf: 	
	@echo "Making TDMF brand"
	brand_mk.sh -b TDMFBRAND $${FIXNUM+-f "$${FIXNUM}"} $${BUILD+-n "$${BUILD}"} $${BUILDPREFIX+-p "$${BUILDPREFIX}"} $${BUILDSUFFIX+-s "$${BUILDSUFFIX}"}
	brand_wish.sh -b TDMFBRAND
ifeq (AIX,${SYSTYPE})
	brand_installp.sh -b TDMFBRAND
endif
