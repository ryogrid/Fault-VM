#! /bin/sh

if [ $# -ne 2 ]; then
    echo "usage: ./logging_master.sh <domain_prefix> <guest_counts>"
    exit 1
fi

count=0
dom_prefix=$1
guest_counts=$2

while [ $count -lt $guest_counts ]
do
  ./logging_for_parallel.sh $dom_prefix $count &
  count=`expr $count + 1`
done
