#!/bin/bash -e

TOOL_ARCHIVE=/tmp/coverity.tar.gz
TOOL_BASE=/tmp/coverity-scan-analysis
TOOL_URL=https://scan.coverity.com/download/`uname`

if [ ! -z "`find $TOOL_BASE -type -d -name 'cov-analysis*'`" ]; then
	echo -e "\033[33;1mUsing cached Coverity Scan Analysis Tools\033[0m"
	exit 0
fi

echo -n | openssl s_client -connect scan.coverity.com:443 | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' | sudo tee -a /etc/ssl/certs/ca-

pushd /tmp
echo -e "\033[33;1mDownloading Coverity Scan Analysis Tool...\033[0m"
wget -nv $TOOL_URL --post-data "project=$COVERITY_PROJECT_NAME&token=$COVERITY_SCAN_TOKEN" -O coverity.tar.gz

echo -e "\033[33;1mExtracting Coverity Scan Analysis Tool...\033[0m"
mkdir -p $TOOL_BASE
pushd $TOOL_BASE
tar xzf $TOOL_ARCHIVE
popd

rm coverity.tar.gz
popd

TOOL_BASE=/tmp/coverity-scan-analysis
TOOL_DIR=`find $TOOL_BASE -type d -name 'cov-analysis*'`
export PATH=$TOOL_DIR/bin:$PATH

cov-configure --template --comptype gcc --compiler gcc-9
