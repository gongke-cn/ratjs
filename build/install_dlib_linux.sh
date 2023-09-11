#/bin/bash

if [ "x$1" = "x-u" ]; then
    rm -f $INSTALL_PREFIX/lib/libratjs.so
	rm -f $INSTALL_PREFIX/lib/libratjs.so.$SO_MAJOR_VERSION
	rm -f $INSTALL_PREFIX/lib/libratjs.so.$SO_MAJOR_VERSION.$SO_MINOR_VERSION
	rm -f $INSTALL_PREFIX/lib/libratjs.so.$SO_MAJOR_VERSION.$SO_MINOR_VERSION.$SO_MICRO_VERSION
else
    install -m 644 -T -s $O/libratjs.so $INSTALL_PREFIX/lib/libratjs.so.$SO_MAJOR_VERSION.$SO_MINOR_VERSION.$SO_MICRO_VERSION
    cd $INSTALL_PREFIX/lib
    ln -s libratjs.so.$SO_MAJOR_VERSION.$SO_MINOR_VERSION.$SO_MICRO_VERSION libratjs.so.$SO_MAJOR_VERSION.$SO_MINOR_VERSION
    ln -s libratjs.so.$SO_MAJOR_VERSION.$SO_MINOR_VERSION.$SO_MICRO_VERSION libratjs.so.$SO_MAJOR_VERSION
    ln -s libratjs.so.$SO_MAJOR_VERSION.$SO_MINOR_VERSION.$SO_MICRO_VERSION libratjs.so
fi