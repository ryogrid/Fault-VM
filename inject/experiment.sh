#! /bin/sh

count=0
while [ $count -lt $2 ]
do
  echo "experiment $count times"
  echo "start from domain "`expr $1 + $count`
  ./random_with_pause.out `expr $1 + $count`
  count=`expr $count + 1`
done
