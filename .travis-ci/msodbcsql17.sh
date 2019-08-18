wget https://packages.microsoft.com/keys/microsoft.asc -O - | sudo apt-key add -
sudo wget https://packages.microsoft.com/config/ubuntu/14.04/prod.list -O /etc/apt/sources.list.d/mssql-release.list
sudo apt-get update
sudo ACCEPT_EULA=Y apt-get install msodbcsql17 unixodbc-dev

export MSSQL_DRIVER="ODBC Driver 17 for SQL Server"
