#! /bin/sh

#check time diff of centoslvslv0
remote_time=`ssh -l root -i ~/.ssh/id_rsa_lvs 192.168.1.11 date -u +%s`
local_time=`date -u +%s`
diff=`expr $remote_time - $local_time`
echo $diff

