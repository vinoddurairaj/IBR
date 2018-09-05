// DO NOT Edit this file w/o consulting with the IE build lab (mailto:iebld)

// Change VER_PRODUCTBUILD and VER_PRODUCTBUILD_QFE as appropriate.

#define VER_MAJOR_PRODUCTVER		5
#define VER_MINOR_PRODUCTVER		00
#define VER_PRODUCTBUILD                /* Win9x */  3502

//
// Use the ntverp.h define of VER_PRODUCTBUILD_QFE
//
#ifndef VER_PRODUCTBUILD_QFE
#define VER_PRODUCTBUILD_QFE            /* Win9x */  1000
#endif

#define VER_PRODUCTVERSION		VER_MAJOR_PRODUCTVER,VER_MINOR_PRODUCTVER,VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE
#define VER_PRODUCTVERSION_W		(0x0500)
#define VER_PRODUCTVERSION_DW		(0x05000000 | VER_PRODUCTBUILD)


// READ THIS

// Do not change VER_PRODUCTVERSION_STRING.
//
//       Again
//
// Do not change VER_PRODUCTVERSION_STRING.
//
//       One more time
//
// Do not change VER_PRODUCTVERSION_STRING.
//
// ntverp.h will do the right thing wrt the minor version #'s by stringizing
// the VER_PRODUCTBUILD and VER_PRODUCTBUILD_QFE values and concatenating them to
// the end of VER_PRODUCTVERSION_STRING.  VER_PRODUCTVERSION_STRING only needs
// is the major product version #'s. (currently, 5.00)

#define VER_PRODUCTBETA_STR		/* Win9x */  ""
#define VER_PRODUCTVERSION_STRING	"5.00"

