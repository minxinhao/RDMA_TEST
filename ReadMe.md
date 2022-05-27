# RDMA_TEST

Examples for using RDMA and testing performance of RDMA communication.

Reference codes in  https://github.com/jcxue/RDMA-Tutorial. Fix some bugs and unused function(in my view).

## Usage

```
make

# At node 1
./rdma_test server ip_of_server port_of_server

# At node 2
./rdma_test client ip_of_server port_of_server

#Output will be in client.log && server.log
```




## bugs

### ibv_create_cq
For call of ibv_create_cq, the initial code set the number of cq to max_cq in dev_attr. It will cuase collapse and lead the processing of cq to be very slow.
Set it to 512 makes the ultimate performance to be approaching to perftest.

### ibv_poll_cq

1. ibv_poll_cq may return with zero cq polled.
2. Remote Write will also generate wc into server's cq.
