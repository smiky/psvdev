	.set noreorder

#include "pspstub.s"

	STUB_START	"sceCtrl_driver",0x00090011,0x00010005
	STUB_FUNC	0x2BA616AF,sceCtrlPeekBufferPositive660
	STUB_END
