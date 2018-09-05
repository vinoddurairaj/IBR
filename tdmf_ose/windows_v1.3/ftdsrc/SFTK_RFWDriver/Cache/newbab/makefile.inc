$(PASS0_HEADERDIR)\CacheMsg.h $(PASS0_HEADERDIR)\CacheDriver.rc $(PASS0_HEADERDIR)\msg00001.bin:CacheMsg.mc	
	mc -c -h $(PASS0_HEADERDIR) -r $(PASS0_CLIENTDIR) -x $(PASS0_SERVERDIR) -a -A CacheMsg.mc

nsym: 
!IF "$(LINKONLY)" == "1"
      ..\..\..\Tools\Bin\nmsym.exe /TRANSLATE:ALWAYS,SOURCE,PACKAGE /SOURCE:.;$(DDK_INC_PATH);$(SDK_INC_PATH) $(TARGETPATH)\$(PROCESSOR_LABEL)\$(TARGETNAME).sys
!ELSE
	@echo
!ENDIF
	copy FSTCache.reg $(TARGETPATH)\$(PROCESSOR_LABEL)\FSTCache.reg

clean:
	del /q $(PASS0_HEADERDIR)
	pushd .
	cd ..\..\..\Intermediate\$(DDKBUILDENV)\$(DDK_TARGET_OS)\SSPCORE\$(TARGETNAME)\$(PROCESSOR_LABEL)
	del /q $(MY_OBJ)
	popd 
	echo "deleting ..\..\..\Intermediate\$(DDKBUILDENV)\$(DDK_TARGET_OS)\SSPCORE\" $(MY_FILES)
