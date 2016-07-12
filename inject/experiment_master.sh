#! /bin/sh

if [ $# -ne 3 ]; then
    echo "usage: ./experiment_master.sh <guest_prefix> <guest_num> <experiment_count>"
    exit 1
fi

guest_prefix=$1
guest_num=$2
experiment_count=$3
count=0

guest_ip[0]="172.16.100.170"
guest_ip[1]="172.16.100.84"
guest_ip[2]="172.16.100.172"

while [ $count -lt $guest_num ]
do
  echo "start a experiment thread using $geust_prefix$count"
  ./experiment_for_parallel.sh $guest_prefix$count $experiment_count ${guest_ip[$count]} &
  count=`expr $count + 1`
done
exit 1
