cmd="multis_admin -m 239.1.2.3 -j "

for k in $( seq 1 128 )
do
    cmd=$cmd"192.168.0.$k,"
done

cmd=$cmd"192.168.0.129"
echo $cmd
