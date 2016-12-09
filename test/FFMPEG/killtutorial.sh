#!/bin/bash
echo `ps -A  | grep -i tutorial | awk '{print $1}'`
pid=`ps -A | grep -i tutorial | awk '{print $1}'`
echo ${pid}
kill -9 ${pid}
