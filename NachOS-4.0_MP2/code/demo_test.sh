#rebuild all

cd build.linux
make distclean
make depend
make
cd ../test
make clean
make


../build.linux/nachos -f
../build.linux/nachos -cp file1.test /file1
# ../build.linux/nachos -cp FS_test1 /FS_test1
# ../build.linux/nachos -d tf -e /FS_test1
# ../build.linux/nachos -l /
../build.linux/nachos -p /file1
../build.linux/nachos -cp FS_test2 /FS_test2
../build.linux/nachos -e /FS_test2

# run the test bash script
# echo "***************************************"
# sh FS_partII_a.sh
# echo "***************************************"
# sh FS_partII_b.sh
# echo "***************************************"
# sh FS_partII_c.sh
# echo "***************************************"

# ../build.linux/nachos -d dt -cp file1.test /

# test how the file system format virtual disk.
# ../build.linux/nachos -d f -f

# test Part1
# cd test
# echo "***************************************"
# ../build.linux/nachos -e consoleIO_test1
# echo "***************************************"
# ../build.linux/nachos -e consoleIO_test2

# echo "***************************************"
# #test Part2
# rm file1.test
# ../build.linux/nachos -e fileIO_test1
# cat file1.test
# echo ""
# echo "***************************************"
# ../build.linux/nachos -e fileIO_test2
