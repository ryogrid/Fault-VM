#! /bin/sh

if [ $# -ne 2 ]; then
    echo "usage: ./logging_for_parallel.sh <domain_prefix> <domain_num>"
    exit 1
fi

dom_prefix=$1
dom_num=$2

while true
do
  dom_id=`xm list | grep $dom_prefix$dom_num | ruby -e 'ARGF.each{ |line| puts line.split(/\s+/)[1] }'`
  if [ -n $dom_id ]; then
      is_alive=`ps ax | grep "qemu-dm -d $dom_id" | wc -l`
      if [ $is_alive = 2 ]; then
	  telnet localhost `expr 10001 + $dom_num` >> "guest_serial_outputs_"$dom_prefix$dom_num".log"
      fi
  fi
done
