dir=$(readlink -f `dirname ${BASH_SOURCE[0]*/}`)
mssqlPass=`$dir/makePassword.py`

docker pull microsoft/mssql-server-linux:latest
export MSSQL_CONTAINER=`docker run -e 'ACCEPT_EULA=Y' -e "SA_PASSWORD=$mssqlPass" -p 127.0.0.1:1433:1433/tcp \
	-d microsoft/mssql-server-linux:latest`

export MSSQL_HOST=127.0.0.1
export MSSQL_USERNAME=SA
export MSSQL_PASSWORD=$mssqlPass
unset mssqlPass

mysqlPass=`$dir/makePassword.py`

docker pull mariadb:latest
export MYSQL_CONTAINER=`docker run -e "MYSQL_ROOT_PASSWORD=$mysqlPass" -p 127.0.0.1:3306:3306/tcp \
	-d mariadb:latest

export MYSQL_HOST=127.0.0.1
export MYSQL_USERNAME=root
export MYSQL_PASSWORD=$mysqlPass
unset mysqlPass
