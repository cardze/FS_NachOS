#rebuild all

cd build.linux
make clean
make depend
make
cd ../test
make distclean
make

test Part1
cd test
echo "***************************************"
../build.linux/nachos -e consoleIO_test1
echo "***************************************"
../build.linux/nachos -e consoleIO_test2

echo "***************************************"
#test Part2
rm file1.test
../build.linux/nachos -e fileIO_test1
cat file1.test
echo ""
echo "***************************************"
../build.linux/nachos -e fileIO_test2
