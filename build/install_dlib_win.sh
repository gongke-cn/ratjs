#/bin/bash

if [ "x$1" = "x-u" ]; then
    rm -f $INSTALL_PREFIX/bin/libratjs.dll
    rm -f $INSTALL_PREFIX/lib/libratjs.dll.a
else
    install -m 755 -T -s $O/libratjs.dll $INSTALL_PREFIX/bin/libratjs.dll
    install -m 644 -T -s $O/libratjs.dll.a $INSTALL_PREFIX/lib/libratjs.dll.a
fi