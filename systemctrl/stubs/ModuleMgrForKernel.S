	.set noreorder

#include "pspstub.s"

	STUB_START	"ModuleMgrForKernel",0x00090011,0x00030005
	STUB_FUNC	0x2B7FC10D,sceKernelLoadModuleWithApitype2660
	STUB_FUNC	0x939E4270,sceKernelLoadModule660
	STUB_FUNC	0x3FF74DF1,sceKernelStartModule660
	STUB_END
