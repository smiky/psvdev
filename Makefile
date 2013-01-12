all: clean make

clean:
	make -C tn clean
	make -C systemctrl clean

	-rm "flash0/kd/systemctrl.prx"

	-rm "FLASH0.TN"
	-rm "TN.BIN"

make:
	make -C tn
	make -C systemctrl

	cp "tn/tn.bin" "TN.BIN"

	psp-packer systemctrl/systemctrl.prx
	cp "systemctrl/systemctrl.prx" "flash0\kd\systemctrl.prx"
	package_maker