make clean
make
if not exist build md build else del build\*.* /q
cp "TN.BIN" "build\TN.BIN"
cp "FLASH0.TN" "build\FLASH0.TN"
make clean