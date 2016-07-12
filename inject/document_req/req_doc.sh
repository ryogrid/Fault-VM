#! /bin/sh

count=0
while [ $count -lt 10000 ]
do
  echo "req $count times"
  wget http://192.168.1.1/index.html
  count=`expr $count + 1`
done
