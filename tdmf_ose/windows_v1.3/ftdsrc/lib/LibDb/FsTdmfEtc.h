// FsTdmfEtc.h
// Fujitsu Software DataBase Tdmf
//
//	This is an API to access the Tdmf DataBase
//
//	See Eof for developer's manual
//

#ifndef _FS_TDMF_ETC_H_
#define _FS_TDMF_ETC_H_

// Fujitsu Software Exeption handler for DataBase
class FsExDb
{
public:
    FsExDb (){};
    ~FsExDb (){};
    const char *ShowReason() const { return "Exception in Tdmf Db"; }

}; // class FsExDb

class FsNvp // Name Value Pair
{
public:
    //assignment operator
    FsNvp operator=(const FsNvp& aFsNvp)
    {
        mszN = aFsNvp.mszN;
        mszV = aFsNvp.mszV;
		mcpV = aFsNvp.mcpV;
        return *this;
    };


	CString mszN;
	CString mszV;
	//char    mcaV [ 1024 ];
	char*   mcpV;

}; // struct FsNvp

class FsFld // Fields descriptors
{
public:
	CString lszName;
	CString lszType;


}; // class FsFld

//class FsIpV6
//{
//	int Ip1;
//	int Ip2;
//	int Ip3;
//	int Ip4;

//public:

//	FsIpV6 ( CString pszIp = "" ); 
	// Either "111.222.333.444" or "int.int.int:111.222.333.444"
//	FsIpV6 ( int piIp1, int piIp2, int piIp3, int piIp4 );

//	int     GetIpInt   ( int piRank ); // Ip1 or Ip2 or Ip3 or Ip4
//	CString GetIpStr   ( int piRank );
//	CString GetIpStrV6 ();

//	BOOL SetIpInt   ( CString pszIp );

//}; // class FsIpV6

#endif

