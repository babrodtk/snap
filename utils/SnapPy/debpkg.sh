# results appear under dist
VERSION=1.6.8
dch -v ${VERSION}-1 -U "initial upload"
dch -r ''
VERSION=$VERSION python3 setup.py sdist
cd dist
tar xvfz Snappy-${VERSION}.tar.gz 
cd Snappy-${VERSION}
cp -r ../../debian .
mv ../Snappy-${VERSION}.tar.gz ../snap-py_${VERSION}.orig.tar.gz
debuild -us -uc -sa



