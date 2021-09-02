make clean
make test
echo 1 | sudo tee  /proc/sys/vm/drop_caches
sleep 10s
./test 0 1 4 #seq write

echo 1 | sudo tee  /proc/sys/vm/drop_caches
sleep 10s
./test 2 1 1024 #seq read

echo 1 | sudo tee  /proc/sys/vm/drop_caches
sleep 10s
./test 2 0 16 # rand read

make clean
make test
echo 1 | sudo tee  /proc/sys/vm/drop_caches
sleep 10s
./test 0 0 1024 #rand write
