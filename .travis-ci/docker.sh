#!/bin/bash

dir=$(readlink -f `dirname ${BASH_SOURCE[0]*/}`)
mssqlPass=`$dir/makePassword.py`

docker pull microsoft/mssql-server-linux:latest
export MSSQL_CONTAINER=`docker run -e 'ACCEPT_EULA=Y' -e "SA_PASSWORD=$mssqlPass" -p 127.0.0.1:1433:1433/tcp \
	--name mssql -d microsoft/mssql-server-linux:latest`

export MSSQL_HOST=127.0.0.1
export MSSQL_USERNAME=SA
export MSSQL_PASSWORD=$mssqlPass
unset mssqlPass
