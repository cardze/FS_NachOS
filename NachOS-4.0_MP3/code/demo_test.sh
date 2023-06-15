#rebuild all
cd build.linux
make distclean
make depend
make
cd ../test
make clean
make

# sh FS_partIII.sh
# run the test bash script
# clear
# echo "***************************************"
# sh FS_partII_a.sh
# echo "***************************************"
# sh FS_partII_b.sh
# echo "***************************************"
# sh FS_partII_c.sh
# echo "***************************************"
# sh FS_partIII_b.sh
echo "***************************************"
sh FS_partIII.sh
echo "***************************************"
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
