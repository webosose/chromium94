#!/bin/bash

while read bin package
do
    [[ -z `which $bin` ]] && {
        echo >&2 "$bin is not installed: try 'sudo apt-get install $package'"
        exit 1
    }
done <<EOF
uwsgi_python3 uwsgi-plugin-python3
EOF

PORT=${1:-8887}
ROOT=${2:-`pwd`}

echo "Serving on port $PORT..."
exec uwsgi -M --die-on-term --plugin python3 --wsgi-file tests_server.py \
--http-socket :$PORT --pyargv $ROOT --static-map /media=../tests/media \
--honour-range --workers 2 --add-header "Accept-Ranges: bytes"
