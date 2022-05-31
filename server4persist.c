#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

#include "debug.h"
#include "ib.h"
#include "setup_ib.h"
#include "config.h"
#include "server.h"

void *server4persist_thread (void *arg)
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

    ret = post_send (0, lkey, 0, MSG_CTL_START, qp, buf_base); //set wr_id to 0 for start send wr
    while((n=ibv_poll_cq (cq, num_wc, wc))==0){}

    // post recv for complete flag
    ret = post_srq_recv (msg_size, lkey, get_wr_id(), srq, buf_ptr);
    check(ret==0,"server post recv error");
    while (!stop){
        n=ibv_poll_cq (cq, num_wc, wc);
        for(i = 0 ; i < n ; i++){
            if(wc[i].opcode == IBV_WC_RECV){
                check(ntohl(wc[i].imm_data) == MSG_CTL_STOP,"Expect to recv stop flag");
                printf("The value read from buf is: str_len:%ld content:%s\n",strlen(buf_ptr+msg_size),buf_ptr+msg_size);
                stop = true;
                break;
            }
        }
    }

    // post recv for complete flag
    ret = post_srq_recv (msg_size, lkey, get_wr_id(), srq, buf_ptr);
    check(ret==0,"server post recv error");
    stop = false;
    while (!stop){
        n=ibv_poll_cq (cq, num_wc, wc);
        for(i = 0 ; i < n ; i++){
            if(wc[i].opcode == IBV_WC_RECV){
                check(ntohl(wc[i].imm_data) == MSG_CTL_STOP,"Expect to recv stop flag");
                printf("The value read from buf is: str_len:%ld content:%s\n",strlen(buf_ptr+2*msg_size),buf_ptr+2*msg_size);
                stop = true;
                break;
            }
        }
    }

    ret = post_send (0, lkey, 0, MSG_CTL_STOP, qp, buf_base); //set wr_id to 0 for start send wr
    while((n=ibv_poll_cq (cq, num_wc, wc))==0){}

    free (wc);
    pthread_exit ((void *)0);

 error:
    if (wc != NULL) {
    	free (wc);
    }
    pthread_exit ((void *)-1);
}

int run_server4persist ()
{
    int   ret         = 0;
    long  num_threads = 1;
    long  i           = 0;

    pthread_t           *threads = NULL;
    void                *status;

    threads = (pthread_t *) calloc (num_threads, sizeof(pthread_t));
    check (threads != NULL, "Failed to allocate threads.");

    for (i = 0; i < num_threads; i++) {
        ret = pthread_create (&threads[i], NULL, server4persist_thread, (void *)i);
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

    free (threads);

    return 0;

 error:
    if (threads != NULL) {
        free (threads);
    }
    
    return -1;
}
