# run.sh port
./rdma_test server 192.168.1.11 $1  \
& sshpass -p 'mxh' ssh mxh@192.168.1.22 "~/RDMA_TEST/rdma_test client 192.168.1.11 $1"