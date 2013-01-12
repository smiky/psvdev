	.set noreorder

#include "pspstub.s"

	STUB_START	"SysMemForKernel",0x00010011,0x00050005
	STUB_FUNC	0x58148F07,sceKernelCreateHeap660
	STUB_FUNC	0x23D81675,sceKernelAllocHeapMemory660
	STUB_FUNC	0x87C2AB85,sceKernelFreeHeapMemory660
	STUB_FUNC	0x83B5226D,sceKernelSetDdrMemoryProtection660
	STUB_FUNC	0x07C586A1,sceKernelGetModel660
	STUB_END
