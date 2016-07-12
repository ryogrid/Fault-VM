#! /bin/sh

while true
do
  is_alive=`ps ax | grep "qemu-dm" | wc -l`
  if [ $is_alive = 2 ]; then
      telnet localhost 10000 >> "guest_serial_outputs.log"
  fi
done
