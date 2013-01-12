	.set noreorder

#include "pspstub.s"

	STUB_START	"LoadCoreForKernel",0x00010011,0x00030005
	STUB_FUNC	0xF6B1BF0F,sceKernelFindModuleByName660
	STUB_FUNC	0xBC99C625,sceKernelFindModuleByAddress660
	STUB_FUNC	0x40972E6E,sceKernelFindModuleByUID660
	STUB_END