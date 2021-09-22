#!/bin/bash
 
### BEGIN INIT INFO
# Provides:     test
# Required-Start:  $remote_fs $syslog
# Required-Stop:   $remote_fs $syslog
# Default-Start:   2 3 4 5
# Default-Stop:   0 1 6
# Short-Description: start test
# Description:    start test
### END INIT INFO
 
#此处编写脚本内容
cd /home/ubuntu/Desktop
# gcc -Wall -o "./client/gtk" "./client/gtk.c" -lpthread  `pkg-config --cflags --libs gtk+-2.0`
cd client
./gtk
exit 0
