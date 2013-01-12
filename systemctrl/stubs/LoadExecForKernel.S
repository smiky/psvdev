	.set noreorder

#include "pspstub.s"

	STUB_START	"LoadExecForKernel",0x00090011,0x00030005
	STUB_FUNC	0x08F7166C,sceKernelExitVSHVSH660
	STUB_FUNC	0xB57D0DEC,sceKernelCheckExitCallback660
	STUB_FUNC	0x1F08547A,sceKernelInvokeExitCallback660
	STUB_END
