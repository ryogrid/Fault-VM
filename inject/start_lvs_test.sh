#! /bin/sh

if [ $# -ne 4 ]; then
    echo "usage: ./start_lvs_test.sh <domain_name> <inject_bits> <bits_per_once> <wait_sec_between_inject>"
    exit 1
fi

dom_name=$1
inject_bits=$2
bits_per_once=$3
wait_sec_between_inject=$4

#write diff in time to file
#this value is time diff of remote from local's perspective
./check_time_diff.sh > time_diff_$dom_name.txt

#start analyze throughput
ruby document_req/request_analyzer.rb &

#start injection
dom_id=`xm list | grep $dom_name | ruby -e 'ARGF.each{ |line| puts line.split(/\s+/)[1] }'`
./experiment_once_lvs.out $dom_name $dom_id $inject_bits $bits_per_once $wait_sec_between_inject
