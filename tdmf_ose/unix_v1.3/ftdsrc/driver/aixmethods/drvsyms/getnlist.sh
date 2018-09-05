#!/bin/sh

# kernel namelist for our driver's text symbols ...

# file references
drv=../../ftd
Tnm=./T0
Bnm=./B0
Dnm=./D0
Unm=./U0
Tnm_lcl=./T0_lcl
Bnm_lcl=./B0_lcl
Dnm_lcl=./D0_lcl
Unm_lcl=./U0_lcl
nlistprg=./drvnlist.c
nlistprghdr=./drvnlist.h
cleanf="${Tnm} ${Bnm} ${Dnm} ${Unm} ${Tnm_lcl} ${Bnm_lcl} ${Dnm_lcl} ${Unm_lcl}"

# reap text bss and data syms
nm -e ${drv} | awk ' $2 == "T" {printf "%s\n", $1} ' 1> ${Tnm} 2>&1
nm -e ${drv} | awk ' $2 == "B" {printf "%s\n", $1} ' 1> ${Bnm} 2>&1
nm -e ${drv} | awk ' $2 == "D" {printf "%s\n", $1} ' 1> ${Dnm} 2>&1
nm -e ${drv} | awk ' $2 == "U" {printf "%s\n", $1} ' 1> ${Unm} 2>&1

# driver local syms will not be found undefined ...
egrep -v -f ${Unm} ${Tnm} 1> ${Tnm_lcl} 2>&1
egrep -v -f ${Unm} ${Bnm} 1> ${Bnm_lcl} 2>&1
egrep -v -f ${Unm} ${Dnm} 1> ${Dnm_lcl} 2>&1

# gen a C nlist struct for found lcl text syms ...
rm -f ${nlistprg} ${nlistprghdr}

echo "#include <stdio.h>" 1>> ${nlistprg} 2>&1
echo "#include <nlist.h>" 1>> ${nlistprg} 2>&1
echo "#include \"${nlistprghdr}\"" 1>> ${nlistprg} 2>&1

echo "" 1>> ${nlistprg} 2>&1

echo "#if defined(XCOFF32)" 1>> ${nlistprg} 2>&1
echo "struct nlist drvtxtnlist [] = {"  1>> ${nlistprg} 2>&1
echo "#else /* defined(XCOFF32) */" 1>> ${nlistprg} 2>&1
echo "struct nlist64 drvtxtnlist [] = {"  1>> ${nlistprg} 2>&1
echo "#endif /* defined(XCOFF32) */" 1>> ${nlistprg} 2>&1

# build the structs and code
nmcnt=0
cat ${Tnm_lcl} |\
while read rsym
do
	nmcnt=`expr ${nmcnt} + 1`
	sym=`echo ${rsym} | sed -e '/^\./s///'`
	echo "{\"${sym}\",0,0,0,0,0}," 1>> ${nlistprg} 2>&1
	#echo "{\"${rsym}\",0,0,0,0,0}," 1>> ${nlistprg} 2>&1
done
echo "};" 1>> ${nlistprg} 2>&1

cat >> ${nlistprg} << EONLISTPROG


main()
{
	int i;
	int errs;
	int nlisterr;
	
	errs=0;
	for(i=0;i<NDRVSYMS;i++){
		nlisterr = knlist(&drvtxtnlist[i], 1, NLISTSTRUT_SIZ);
		if (nlisterr != 0)
			errs++;
	}

	exit(0);

}
EONLISTPROG

echo "#define NDRVSYMS ${nmcnt}"  1>> ${nlistprghdr} 2>&1
echo "#if defined(XCOFF32)" 1>> ${nlistprghdr} 2>&1
echo "#define NLISTSTRUT_SIZ sizeof(struct nlist)" 1>> ${nlistprghdr} 2>&1
echo "#else /* defined(XCOFF32) */" 1>> ${nlistprghdr} 2>&1
echo "#define NLISTSTRUT_SIZ sizeof(struct nlist64)" 1>> ${nlistprghdr} 2>&1
echo "#endif /* defined(XCOFF32) */" 1>> ${nlistprghdr} 2>&1

rm -f ${cleanf}
