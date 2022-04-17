make all
echo
echo
echo "@@@@@@@@ Http-download test starting ...... @@@@@@@@"
./http-download -u http://www.gnu.org/software/make/manual/make.pdf -f make.pdf -j 5 -m make.md5
echo "&&&&&&&& Http-download test end ...... &&&&&&&&"
echo
echo
make clean-all
rm make.pdf -f
