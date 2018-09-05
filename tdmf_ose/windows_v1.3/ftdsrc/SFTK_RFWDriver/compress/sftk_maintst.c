#include <ntddk.h>
#include "sftk_comp.h"

unsigned char src[4096];
unsigned char trgt[4096];

NTSTATUS 
DriverEntry(IN PDRIVER_OBJECT   DriverObject, 
            IN PUNICODE_STRING  RegistryPath 
            )
/**/
{
	NTSTATUS        ret = STATUS_SUCCESS;
	comp_t          *p_compression;
	size_t          trg_len=0, src_len=0, comp_len = 0;

#ifdef DBG
	__asm { int 3 };
#endif

	RtlCopyMemory(src, "Il fait beau aujourd'hui Il fait beau aujourd'hui Il fait beau aujourd'hui", 74);

	p_compression = comp_create(LZHL);

	comp_len = comp_compress(trgt, &trg_len, src, 74, p_compression);
	trg_len = 74;
	src_len = 74;
	RtlZeroMemory(src, src_len);
	comp_len = comp_decompress(src, &src_len, trgt, &trg_len, p_compression);
	comp_delete(p_compression);
	p_compression = comp_create(PRED);

	RtlZeroMemory(trgt, trg_len);
	comp_len = comp_compress(trgt, &trg_len, src, 74, p_compression);
	
	trg_len = 74;
	src_len = 74;
	RtlZeroMemory(src, src_len);
	comp_len = comp_decompress(src, &src_len, trgt, &trg_len, p_compression);
	
	comp_delete(p_compression);

return(STATUS_SUCCESS);
} /*DriverEntry*/
