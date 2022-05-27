exec=rdma_test
msg="#bytes     #iterations    BW peak[MB/sec]    BW average[MB/sec]   MsgRate[Mpps]"
# msg="#bytes #iterations    t_min[usec]    t_max[usec]  t_typical[usec]    t_avg[usec]    t_stdev[usec]   99% percentile[usec]   99.9% percentile[usec]"

log=MSG_SIZE.txt
echo $msg>$log
for((msg_size=1;msg_size<=2<<14;msg_size=msg_size*2));
do
    echo $msg_size
    $IBV_CMD -n 10000000 -q 1 -s $msg_size | tail -2 >>$log  &
    sshpass -p 'mxh'  ssh mxh@192.168.1.22 "$IBV_CMD 192.168.1.11 -n 10000000 -q 1 -s  $msg_size "> /dev/null
done
