#/bin/bash

cat << EOF  >$O/$1.sh
#!/bin/bash

if [ ! -L $O/libratjs.so.$SO_MAJOR_VERSION ]; then
    cd $O;
    rm -f libratjs.so.$SO_MAJOR_VERSION;
    ln -s libratjs.so libratjs.so.$SO_MAJOR_VERSION;
    cd ..;
fi
LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$O $O/$1 \$@
EOF

chmod a+x $O/$1.sh