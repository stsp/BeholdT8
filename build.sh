run ()
{
	echo $@
	$@
	if [ "$?" != "0" ]; then
		echo "*** ERROR. Aborting ***"
		exit -1
	fi
}

echo "***********************************************************"
echo "* This script will download the latest tarball and build it"
echo "* Assuming that your kernel is compatible with the latest  "
echo "* drivers. If not, you'll need to add some extra backports,"
echo "* ./backports/<kernel> directory.                          "
echo "***********************************************************"
sleep 5
run make -C linux/ download
run make -C linux/ untar
run make

echo "**********************************************************"
echo "* Compilation finished. Use "make install" to install them"
echo "**********************************************************"

