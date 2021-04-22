mujs has identifiers conflicting with quickjs
quick fix
	cd ./src/mujs&&ls|while read FILE;do cat $FILE|sed "s/js_/mjs_/g" > /tmp/a;mv /tmp/a $FILE;done

mongoose settings
MG_MAX_HTTP_SEND_MBUF
MG_MAX_HTTP_HEADERS
MG_ENABLE_DIRECTORY_LISTING
MG_ENABLE_FILESYSTEM
MG_ENABLE_HTTP_CGI
MG_MAX_HTTP_REQUEST_SIZE
MG_MAX_HTTP_SEND_MBUF
MG_ENABLE_BROADCAST
CS_ENABLE_STDIO
SOMAXCONN
