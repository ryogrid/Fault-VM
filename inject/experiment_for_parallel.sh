#! /bin/sh

if [ $# -ne 3 ]; then
    echo "usage: ./experiment_for_parallel.sh <domain_name> <experiment_count> <ip_addr>"
    exit 1
fi

count=0
dom_name=$1
ex_count=$2
ip_addr=$3
dom_id_past="hogehoge"
# sleep_to_restore_seconds=15

#boot guest only first time
xm restore /root/work/xen/centos_hvm/$dom_name".snapshot"
# sleep $sleep_to_restore_seconds

while [ $count -lt $ex_count ]
do
  echo "experiment $count times"
  dom_id_new=`xm list | grep $dom_name | ruby -e 'ARGF.each{ |line| puts line.split(/\s+/)[1] }'`
  if [ -n $dom_id_new ]; then
      echo "dom_id_past=$dom_id_past"
      echo "dom_id_past=$dom_id_new"
      if [ "$dom_id_past" != "$dom_id_new" ]; then
	  echo "execute test_random_access.out from domain $dom_id_new for $dom_name"
	  ./test_random_access.out $dom_name $dom_id_new $ip_addr >> $dom_name".log" 2>&1
	  count=`expr $count + 1`
	  dom_id_past=$dom_id_new
      else
	  echo "dom_id is same with past one"
	  exit 1
      fi
  else
      echo "can't find dom_id"
      exit 1
  fi
done
