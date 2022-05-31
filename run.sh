# run.sh port
make
./sync_code.sh
echo "mxh" |  sudo -S ./rdma_test server 192.168.1.11 $1 /dev/dax0.0  \
& sshpass -p 'mxh' ssh mxh@192.168.1.22 " echo "mxh" | sudo -S ~/RDMA_TEST/rdma_test client 192.168.1.11 $1 /dev/dax0.2"
# & sudo ./rdma_test client 192.168.1.11 $1 ib
