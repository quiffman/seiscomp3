

# To install by hand, onto webdc:
git status

webdcdir=geofon-open2:/srv/www/webdc/eida/reqlogstats
diffs_found=0
pushd www
mkdir -p webdc
for f in * ; do
	scp $webdcdir/$f webdc
	diff -q $f webdc
	if [ $? -ne 0 ] ; then
		echo "diff www/$f www/webdc"
		diffs_found=1
	fi
done

if [ $diffs_found -eq 0 ] ; then
	echo "Identical code at $webdcdir; nothing to update"
	rm -r webdc
else
	rsync -av * --exclude webdc $webdcdir
fi
popd

