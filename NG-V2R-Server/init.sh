#!/bin/bash

CUR_PATH=$(readlink -e ./)
if [ "$WINDIR" ]; then
	CUR_PATH=$(echo "$CUR_PATH" | sed 's/^\///' | sed 's/\//\\\\/g' | sed 's/^./\0:/')
fi

# join_path Path1 Path2 IgNore:0/1
function join_path {
	if [ "$WINDIR" ]; then
		if [[ $3 == "1" ]]; then
			echo "$1\\\\$2"
		else
			echo "$1\\$2"
		fi
	else
		printf "$1/$2"
	fi
}

function dp_println() {
	var=("$@")
	for item in ${var[@]}; do
		echo $item
	done
}

function dp_print() {
	var=("$@")
	for item in ${var[@]}; do
		printf "$item "
	done
}

TEMP_PATH=$(join_path $CUR_PATH tmp 1)



CHECK_CONFIG_FILE="1"

# start_process process args pid_file
function start_process {
	if [ "$WINDIR" ]; then
		if [[ $1 == "*.exe" ]]; then
			BIN_PATH=$1
		else
			BIN_PATH="$1.exe"
		fi
		$BIN_PATH $2 2> /dev/null &
		printf $! > $3
		
	else
		start-stop-daemon --start --make-pidfile --background --pidfile $3 --exec $1 -- $2
	fi
}

function start_nginx {
	if [ "$WINDIR" ]; then
		start_process "$CUR_PATH\\nginx" "-c $CUR_PATH\\nginx.conf" "$TEMP_PATH\\log\\nginx.pid"
	else
		$CUR_PATH/nginx -c $CUR_PATH/nginx.conf
	fi
}

#kill_pid pid
function kill_pid {
	if [ "$WINDIR" ]; then
		kill $1
	else
		/bin/kill -s TERM $1
	fi
}

#kill_pid pid_file
function kill_process {
	if [ "$WINDIR" ]; then
		PID_ID=$(/bin/cat $1)
		kill_pid $PID_ID
		rm -v -f $1
	else
		start-stop-daemon --stop --pidfile "$1" --remove-pidfile
	fi
}

function stop_nginx {
	if [ "$WINDIR" ]; then
		"$CUR_PATH\\nginx.exe" -c "$CUR_PATH\nginx.conf" -s stop
	else
		PID_ID=$(/bin/cat $TEMP_PATH/log/nginx.pid)
		kill_pid $PID_ID
		rm -f -v "$TEMP_PATH/log/nginx.pid"
	fi
}

function f_start {
	PID_FILE=$(join_path "$TEMP_PATH" log)
	PID_FILE=$(join_path "$PID_FILE" nginx.pid)
	if [ -f "$PID_FILE" ]; then
		echo "Nginx allready running";
		exit 1
	fi
	start_nginx
			
	filelists=$(ls -d $CUR_PATH/v2ray/* | grep ".conf")
	while read -r line; do
		source $line
		echo "$V2RAY_REMOTE_HOST"
		start_process "$CUR_PATH/v2ray/v2ray" "-server -mode websocket -localAddr 127.0.0.1 -localPort $LOCAL_PORT -remoteAddr $V2RAY_REMOTE_HOST -remotePort $V2RAY_REMOTE_PORT" "$line.pid"
	done <<< "$filelists"
}

function f_stop {
	if [ ! -f "$TEMP_PATH/log/nginx.pid" ]; then
		echo "Nginx is not running";
		exit 1
	fi
	stop_nginx
			
	filelists=$(ls -d $CUR_PATH/v2ray/* | grep ".pid")
	while read -r line; do
		kill_process $line
	done <<< "$filelists"
}

function f_list {
	filelists=$(ls "$CUR_PATH/v2ray/" | grep ".conf")
	printf "Name\tAddress\t\tRemote Host\n"
	while read -r line; do
		source "$CUR_PATH/v2ray/$line"
		print_data=(
			"$V2RAY_NAME\t"
			"$HASHE_ADDRESS\t"
			"http://$V2RAY_REMOTE_HOST:$V2RAY_REMOTE_PORT"
		)
		printf $(dp_print ${print_data[@]})
	done <<< "$filelists"
}

function f_remove {
	printf "Write V2Ray name:"
	read V2RAY_NAME
	
	if [ ! -f "$CUR_PATH/v2ray/$V2RAY_NAME.conf" ]; then
		echo "Can't find v2ray config"
		exit 1
	fi
	
	source "$CUR_PATH/v2ray/$V2RAY_NAME.conf"
	rm -v -f "$CUR_PATH/v2ray$V2RAY_NAME.service"
	rm -v -f "$CUR_PATH/v2ray/$V2RAY_NAME.conf"
	
	sed -i -s "/\#begin_${V2RAY_NAME}:/,/\#end_${V2RAY_NAME}:/d" "$CUR_PATH/nginx.conf"
}

function f_add {
	if [ ! -f "$CUR_PATH/nginx.conf" ]; then
		echo "Service is not created"
		exit 1
	fi
	printf "Write V2Ray name:"
	read V2RAY_NAME
	printf "Write remote host:"
	read V2RAY_REMOTE_HOST
	printf "Write v2ray remote port:"
	read V2RAY_REMOTE_PORT

	LOCAL_TMP=$(ls "$CUR_PATH" | wc -l)
	#LOCAL_PORT=$(expr 12000 + $LOCAL_TMP)
	LOCAL_PORT=$(cat "$CUR_PATH/last_port.txt" | xargs)
	NEXT_PORT=$(expr $LOCAL_PORT + 1)
	
	printf "$NEXT_PORT" > "$CUR_PATH/last_port.txt"
	
	unit=$(cat <<EOF
[Unit]
Description=V2Ray for $V2RAY_NAME
After=network.target
		
[Service]
Type=simple
User=$USER
ExecStart=$CUR_PATH/v2ray/v2ray -server -mode websocket -localAddr 127.0.0.1 -localPort $LOCAL_PORT -remoteAddr $V2RAY_REMOTE_HOST -remotePort $V2RAY_REMOTE_PORT
Restart=always
		
[Install]
WantedBy=multi-user.target
EOF
	)
	
	printf "${unit}" > "$CUR_PATH/v2ray$V2RAY_NAME.service"

	HASHE_ADDRESS=$(md5sum "$CUR_PATH/v2ray$V2RAY_NAME.service")
	if [ "$WINDIR" ]; then
		HASHE_ADDRESS=${HASHE_ADDRESS:1:10}
	else
		HASHE_ADDRESS=${HASHE_ADDRESS:0:10}
	fi
	conf_data=$(cat <<EOF
V2RAY_NAME=$V2RAY_NAME
V2RAY_REMOTE_HOST=$V2RAY_REMOTE_HOST
V2RAY_REMOTE_PORT=$V2RAY_REMOTE_PORT
HASHE_ADDRESS=$HASHE_ADDRESS
LOCAL_PORT=$LOCAL_PORT
EOF
)
	printf "$conf_data" > "$CUR_PATH/v2ray/$V2RAY_NAME.conf"

	nginx_data=$(cat <<EOF
		\#begin_${V2RAY_NAME}:
		location \/$HASHE_ADDRESS {
			proxy_redirect off;
			proxy_buffering off;
			proxy_http_version 1.1;
			proxy_connect_timeout 3600;
			proxy_send_timeout 3600;
			proxy_read_timeout 3600;
			proxy_pass http:\/\/127.0.0.1:$LOCAL_PORT\/;
			proxy_set_header Host \$http_host;
			proxy_set_header Upgrade \$http_upgrade;
			proxy_set_header Connection "upgrade";
		}
		\#end_${V2RAY_NAME}:
EOF
)
	nginx_data=${nginx_data//$'\n'/'\n'}
	
	sed -i -s "s/\#V2Ray/\#V2Ray\n$nginx_data/" "$CUR_PATH/nginx.conf"

	printf "Remote address if $HASHE_ADDRESS\n"
}

while [ -n "$1" ]; do
	case "$1" in
		start)
			f_start
			exit 0
			shift ;;
		stop)
			f_stop
			exit 0
			shift;;
		list)
			f_list
			exit 0
			shift;;
		add)
			f_add
			exit 0
			shift;;
		remove)
			f_remove
			exit 0
			shift;;
		force)
			CHECK_CONFIG_FILE="5555"
			shift;;
		clean)
			$CUR_PATH/init.sh stop
			find "$CUR_PATH" -name "*.service" | xargs -i rm -v -f {}
			rm -v -f "$CUR_PATH/nginx.conf"
			rm -v -f "$CUR_PATH/last_port.txt"
			rm -v -rf "$TEMP_PATH"
			find "$CUR_PATH/v2ray" -name "*.conf" | xargs -i rm -v -f {}
			
			exit 0
			shift;;
		help)
			printf "start - start server\nstop - stop server\nhelp - write this text\n"
			printf "force - Force init\n"
			echo "clean - Clean all data"
			echo "list - List all added data"
			echo "add - Add New V2Ray Host"
			echo "remove - Remove V2Ray Host"
			exit 0
			shift;;
		*)
			echo "Unknow parametr $1"
			esac
			shift
done

if [[ "$CHECK_CONFIG_FILE" == "1" ]]; then
	if [ -f "$CUR_PATH/nginx.conf" ]; then
		echo "Config is exists"
		exit 1
	fi
fi

chmod +x "$CUR_PATH/nginx"
chmod +x "$CUR_PATH/v2ray/v2ray"
mkdir -p "$TEMP_PATH"
mkdir -p "$CUR_PATH/v2ray"
mkdir -p "$TEMP_PATH/log"
mkdir -p "$TEMP_PATH/cache"
if [ -f "$CUR_PATH/plugin" ]; then
	chmod +x "$CUR_PATH/plugin"
fi

SERVER_PORT=80
CERT_DATA=

if [ -d "$CUR_PATH/ssl" ]; then
	CERT_FILE=$(find "$CUR_PATH/ssl" -name "*.crt")
	CERT_KEY_FILE=$(find "$CUR_PATH/ssl" -name "*.key")
	if [ "$WINDIR" ]; then
		CERT_FILE=$(echo "$CERT_FILE" | sed 's/\//\\\\/g' | sed 's/\\/\\\\/g')
		CERT_KEY_FILE=$(echo "$CERT_KEY_FILE" | sed 's/\//\\\\/g' | sed 's/\\/\\\\/g')
	fi
	if [ ! -f "$CERT_FILE" ]; then
		echo "Can't find cert file";
		exit 1
	fi
	if [ ! -f "$CERT_KEY_FILE" ]; then
		echo "Can't find cert file";
		exit 1
	fi
	SERVER_PORT="443 ssl http2 reuseport; #__DONT_EDIT__"
	CERT_DATA=$(cat <<EOF
		ssl_certificate $CERT_FILE;
		ssl_certificate_key $CERT_KEY_FILE;
		ssl_session_timeout 1d;
		ssl_session_tickets off;
		ssl_protocols TLSv1.3 TLSv1.2;
EOF
)
fi

ERROR_PAGE_401=""

if [ -f "$CUR_PATH/www/401.html" ]; then
	ERROR_PAGE_401="error_page   401 /401.html;"
fi

echo "#user  hacker;
worker_processes  1;

error_log  $TEMP_PATH/log/error.log;
#error_log  $TEMP_PATH/log/error.log  debug;

pid        $TEMP_PATH/log/nginx.pid;


events {
    worker_connections  1024;
}


http {
    include       $CUR_PATH/mime.types;
    client_body_temp_path $TEMP_PATH/cache;
    proxy_temp_path $TEMP_PATH/cache;
    fastcgi_temp_path $TEMP_PATH/cache;
    uwsgi_temp_path $TEMP_PATH/cache;
    scgi_temp_path $TEMP_PATH/cache;

    default_type  application/octet-stream;

    #log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
    #                  '$status $body_bytes_sent "$http_referer" '
    #                  '"$http_user_agent" "$http_x_forwarded_for"';

    access_log  $TEMP_PATH/log/access.log;

    sendfile        on;
    #tcp_nopush     on;

    #keepalive_timeout  0;
    keepalive_timeout  65;

    #gzip  on;

    server {
        listen       $SERVER_PORT;
        server_name  localhost;
		#    server_name  somename  alias  another.alias;
		$CERT_DATA
		# Hide server version
		server_tokens off;

		add_header X-Rotots-Tag \"noindex, nofollow\" always;

        #charset koi8-r;

        #access_log  logs/host.access.log  main;
		if ( \$request_method !~ ^(GET)\$ ) {
			return 401;
		}

        location / {
            root   $CUR_PATH/www;
            index  index.html index.htm;
        }
	
		#V2Ray
		#\${V2RAY_CONFIG}

        error_page   500 502 503 504  /50x.html;
        location = /50x.html {
            root   $CUR_PATH/www;
        }
		$ERROR_PAGE_401

        # pass the PHP scripts to FastCGI server listening on 127.0.0.1:9000
        #
        #location ~ \.php$ {
        #    root           html;
        #    fastcgi_pass   127.0.0.1:9000;
        #    fastcgi_index  index.php;
        #    fastcgi_param  SCRIPT_FILENAME  /scripts$fastcgi_script_name;
        #    include        fastcgi_params;
        #}

        # deny access to .htaccess files, if Apache's document root
        # concurs with nginx's one
        #
        #location ~ /\.ht {
        #    deny  all;
        #}
    }
}" > "$CUR_PATH/nginx.conf"

echo "[Unit]
Description=nginx - high performance web server
Documentation=http://nginx.org/en/docs/
After=network-online.target remote-fs.target nss-lookup.target
Wants=network-online.target

[Service]
Type=forking
User=$USER
WorkingDirectory=$CUR_PATH
PIDFile=$TEMP_PATH/log/nginx.pid
ExecStart=$CUR_PATH/nginx -c $CUR_PATH/nginx.conf
ExecReload=/bin/sh -c \"/bin/kill -s HUP \$(/bin/cat $TEMP_PATH/log/nginx.pid)\"
ExecStop=/bin/sh -c \"/bin/kill -s TERM \$(/bin/cat $TEMP_PATH/log/nginx.pid)\"

[Install]
WantedBy=multi-user.target" > "$CUR_PATH/nginx.service"

echo "#!/bin/bash
CURPATH=\$(pwd)
cd ../../
data=\$(./init.sh list | awk '{ print \$2 }')
cd \$CURPATH

FIRST_LINE=\"yes\"
CMD_EXEC=\"\"

while read -r line; do
	if [[ \"\$FIRST_LINE\" == \"yes\" ]]; then
		FIRST_LINE=\"no\"
	else
		# strlen(CMD_EXEC) > 0
		if [ \"\${#CMD_EXEC}\" -gt \"0\" ]; then
			CMD_EXEC=\"\$CMD_EXEC | \"
		fi
		CMD_EXEC=\"\$CMD_EXEC grep -v \$line\"
	fi
done <<< \"\$data\"

CMD_EXEC=\"cat access.log | \$CMD_EXEC > clean_access.log\"
bash -c \"\$CMD_EXEC\"" > "$TEMP_PATH/log/cleanlog.sh"
chmod +x "$TEMP_PATH/log/cleanlog.sh"

printf "12000" > "$CUR_PATH/last_port.txt"

if [ "$WINDIR" ]; then
	echo "It is Windows. Can't add privilegue"
else
	echo "Add privilegue to start nginx on 0-1000 port:"
	sudo setcap CAP_NET_BIND_SERVICE=+eip "$CUR_PATH/nginx"
	if [ -f "$CUR_PATH/plugin" ]; then
	sudo setcap CAP_NET_BIND_SERVICE=+eip "$CUR_PATH/plugin"
fi

fi
