#!/bin/bash

cat << EOF  >$O/njsgen
#!/bin/bash

$INSTALL_PREFIX/bin/ratjs.exe $INSTALL_PREFIX/lib/ratjs/module/njsgen/njsgen.js "\$@"
EOF

install -m 755 $O/njsgen $INSTALL_PREFIX/bin