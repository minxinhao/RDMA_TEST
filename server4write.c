#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

#include "debug.h"
#include "ib.h"
#include "setup_ib.h"
#include "config.h"
#include "server4write.h"

void *server4write_thread (void *arg)
{
    int         ret		 = 0, i = 0, j = 0, n = 0;
    long        thread_id	 = (long) arg;
    int         num_concurr_msgs = config_info.num_concurr_msgs;
    int         msg_size	 = config_info.msg_size;

    pthread_t   self;
    cpu_set_t   cpuset;

    int                  num_wc		= 20;
    struct ibv_qp       *qp		= ib_res.qp;
    struct ibv_cq       *cq		= ib_res.cq;
    struct ibv_srq      *srq            = ib_res.srq;
    struct ibv_wc       *wc             = NULL;
    uint32_t             lkey           = ib_res.mr->lkey;
    
    char                *buf_ptr	= ib_res.ib_buf;
    char                *buf_base	= ib_res.ib_buf;
    int                  buf_offset	= 0;
    int                  buf_pos = 0 ;
    size_t               buf_size	= ib_res.ib_buf_size;
    uint64_t wr_id;

    uint32_t            imm_data	= 0;
    int			        num_acked_peers = 0;
    bool                stop            = false;
    struct timeval      start, end;
    long                ops_count	= 0;
    double              duration	= 0.0;
    double              throughput	= 0.0;

    wc = (struct ibv_wc *) calloc (num_wc, sizeof(struct ibv_wc));
    check (wc != NULL, "thread[%ld]: failed to allocate wc.", thread_id);

    /* set thread affinity */
    // CPU_ZERO (&cpuset);
    // CPU_SET  ((int)thread_id, &cpuset);
    // self = pthread_self ();
    // ret  = pthread_setaffinity_np (self, sizeof(cpu_set_t), &cpuset);
    // check (ret == 0, "thread[%ld]: failed to set thread affinity", thread_id);

    /* pre-post recvs */
    wc = (struct ibv_wc *) calloc (num_wc, sizeof(struct ibv_wc));
    check (wc != NULL, "thread[%ld]: failed to allocate wc.", thread_id);


    /* signal the client to start */
    ret = post_send (0, lkey, 0, MSG_CTL_START, qp, buf_base); 
    check (ret == 0, "thread[%ld]: failed to signal the client to start", thread_id);
    while ((n = ibv_poll_cq (cq, num_wc, wc))==0){
        // log_message("poll empty cq for send start");
    }

    // wait for client to end
    post_srq_recv (msg_size, lkey, wr_id, srq, buf_ptr);
    stop = false;
    while (!stop){
        n = ibv_poll_cq (cq, num_wc, wc);
        for(i = 0 ; i < n ; i++){
            if(ntohl(wc[i].imm_data)==MSG_CTL_STOP){
                stop=true;
                break;
            }
            post_srq_recv (msg_size, lkey, wr_id, srq, buf_ptr);
        }
    }
    // check(ntohl(wc[0].imm_data)==MSG_CTL_STOP,"Expect to recv stop from client");
    
    /* dump statistics */
    // duration   = (double)((end.tv_sec - start.tv_sec) * 1000000 +
    //                       (end.tv_usec - start.tv_usec));
    // throughput = (double)(ops_count) / duration;
    // log ("thread[%ld]: throughput = %f (Mops/s)",  thread_id, throughput);

    free (wc);
    pthread_exit ((void *)0);

 error:
    if (wc != NULL) {
    	free (wc);
    }
    pthread_exit ((void *)-1);
}

int run_server4write ()
{
    int   ret         = 0;
    long  num_threads = 1;
    long  i           = 0;

    pthread_t           *threads = NULL;
    pthread_attr_t       attr;
    void                *status;

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);

    threads = (pthread_t *) calloc (num_threads, sizeof(pthread_t));
    check (threads != NULL, "Failed to allocate threads.");

    for (i = 0; i < num_threads; i++) {
        ret = pthread_create (&threads[i], &attr, server4write_thread, (void *)i);
        check (ret == 0, "Failed to create server_thread[%ld]", i);
    }

    bool thread_ret_normally = true;
    for (i = 0; i < num_threads; i++) {
        ret = pthread_join (threads[i], &status);
        check (ret == 0, "Failed to join thread[%ld].", i);
        if ((long)status != 0) {
            thread_ret_normally = false;
            log ("server_thread[%ld]: failed to execute", i);
        }
    }

    if (thread_ret_normally == false) {
        goto error;
    }

    pthread_attr_destroy    (&attr);
    free (threads);

    return 0;

 error:
    if (threads != NULL) {
        free (threads);
    }
    pthread_attr_destroy    (&attr);
    
    return -1;
}
