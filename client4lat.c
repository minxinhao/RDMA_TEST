#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

#include "debug.h"
#include "config.h"
#include "setup_ib.h"
#include "ib.h"
#include "client4lat.h"
#include "latency.h"

void *client4lat_thread_func (void *arg)
{
    int         ret		 = 0, n = 0, i = 0, j = 0;
    long	thread_id	 = (long) arg;
    int         msg_size	 = config_info.msg_size;
    int         num_concurr_msgs = config_info.num_concurr_msgs;

    pthread_t   self;
    cpu_set_t   cpuset;

    int                  num_wc		= 20;
    struct ibv_qp	*qp		= ib_res.qp;
    struct ibv_cq       *cq		= ib_res.cq;
    struct ibv_srq      *srq            = ib_res.srq;
    struct ibv_wc       *wc		= NULL;
    uint32_t             lkey           = ib_res.mr->lkey;

    char		*buf_ptr	= ib_res.ib_buf;
    char		*buf_base	= ib_res.ib_buf;
    int			 buf_offset	= 0;
    size_t               buf_size	= ib_res.ib_buf_size;
    uint64_t wr_id ;

    uint32_t		imm_data	= 0;
    int			num_acked_peers = 0;
    bool		start_sending	= false;
    bool		stop		= false;
    struct timeval      start, end;
    // struct timespec start, end;
    long                ops_count	= 0;
    double              duration	= 0.0;
    double              throughput	= 0.0;
    struct Latency lat;

    /* pre-post recvs for start */    
    wc = (struct ibv_wc *) calloc (num_wc, sizeof(struct ibv_wc));
    check (wc != NULL, "thread[%ld]: failed to allocate wc.", thread_id);
    ret = post_srq_recv (msg_size, lkey, (uint64_t)buf_ptr, srq, buf_ptr);

    /* wait for start signal */
    while ((n=ibv_poll_cq (cq, num_wc, wc))==0){
        // log_message("poll empty cq for recv start");
    }
    check(ntohl(wc[i].imm_data)==MSG_CTL_START,"Expect to recv START from client");
    log ("thread[%ld]: ready to send", thread_id);

    //Inital Latency Counter
    ResetLat(&lat);
    
    // clock_gettime(CLOCK_REALTIME,&start); there 250ns deviation in our machine
    gettimeofday(&start,NULL);
    
    /* write server */
    for(i = 0 ; i < num_concurr_msgs ; i++){
        wr_id = get_wr_id();
        // set_msg(buf_ptr,msg_size,wr_id%10);
        ret = post_write (msg_size, lkey, wr_id , (uint32_t)wr_id, ib_res.rkey,ib_res.remote_addr+i*msg_size,qp, buf_ptr);
        // check (ret == 0, "thread[%ld]: failed to post write", thread_id);
    }

    /* wait for write complete */
    while (stop != true) {
        n = ibv_poll_cq (cq, num_wc, wc);
        for (i = 0; i < n; i++) {
            if(unlikely(++ops_count>=num_concurr_msgs)){
                gettimeofday (&end, NULL);
                stop = true;
                break;
            }
        } 
    }
    
    gettimeofday(&end,NULL);
    long total = (end.tv_sec-start.tv_sec)*1000000 + end.tv_usec - start.tv_usec;
    printf("write with latency:%ld us\n",total);

    // send complete flag to server
	ret = post_send (0, lkey, IB_WR_ID_STOP, MSG_CTL_STOP, qp, ib_res.ib_buf);
	check (ret == 0, "thread[%ld]: failed to signal the client to stop", thread_id);
    while ((n = ibv_poll_cq (cq, num_wc, wc))==0){
        // log_message("poll empty cq for send stop");
    }


    free (wc);
    pthread_exit ((void *)0);

 error:
    if (wc != NULL) {
    	free (wc);
    }
    pthread_exit ((void *)-1);
}

int run_client4lat ()
{
    int		ret	    = 0;
    long	num_threads = 1;
    long	i	    = 0;
    
    pthread_t	   *client_threads = NULL;
    void	   *status;

    log (LOG_SUB_HEADER, "Run Client");
    
 
    client_threads = (pthread_t *) calloc (num_threads, sizeof(pthread_t));
    check (client_threads != NULL, "Failed to allocate client_threads.");

    for (i = 0; i < num_threads; i++) {
        ret = pthread_create (&client_threads[i], NULL, client4lat_thread_func, (void *)i);
        check (ret == 0, "Failed to create client_thread[%ld]", i);
    }

    bool thread_ret_normally = true;
    for (i = 0; i < num_threads; i++) {
        ret = pthread_join (client_threads[i], &status);
        check (ret == 0, "Failed to join client_thread[%ld].", i);
        if ((long)status != 0) {
            thread_ret_normally = false;
            log ("thread[%ld]: failed to execute", i);
        }
    }

    if (thread_ret_normally == false) {
        goto error;
    }

    free (client_threads);
    return 0;

 error:
    if (client_threads != NULL) {
        free (client_threads);
    }
    
    return -1;
}
