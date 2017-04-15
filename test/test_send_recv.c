
/**
 * Copyright (c) 2017 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
//#include "test_common.h"
#include "scon.h"
#include "scon_common.h"
#include "src/util/name_fns.h"
#include "src/buffer_ops/buffer_ops.h"
#include "src/buffer_ops/types.h"
#include <stdio.h>
#include <unistd.h>
static bool create = false;
static scon_handle_t handle;
static bool msg_rcvd = false;
static bool msg_sent = false;
#define NUM_MSG_CYCLES 2
/** callback func declarations **/
void create_cbfunc (scon_status_t status,
                    scon_handle_t scon_handle,
                    void *cbdata);
void delete_cbfunc (scon_status_t status,
                    void *cbdata);
void xcast_cbfunc (scon_status_t status,
                   scon_handle_t scon_handle,
                   scon_proc_t procs[],
                   size_t nprocs,
                   scon_buffer_t *buf,
                   scon_msg_tag_t tag,
                   scon_info_t info[],
                   size_t ninfo,
                   void *cbdata);
void send_cbfunc (scon_status_t status,
                  scon_handle_t scon_handle,
                  scon_proc_t *peer,
                  scon_buffer_t *buf,
                  scon_msg_tag_t tag,
                  void *cbdata);
void recv_cbfunc (scon_status_t status,
                  scon_handle_t scon_handle,
                  scon_proc_t *peer,
                  scon_buffer_t *buf,
                  scon_msg_tag_t tag,
                  void *cbdata);
/* create complete callback */
void create_cbfunc (scon_status_t status,
                    scon_handle_t scon_handle,
                    void *cbdata)
{
    printf("\n*****************************create callback : status=%d, sconhandle= %d \n", status, scon_handle);
    handle = scon_handle;
    create = true;
}

/* delete complete callback */
void delete_cbfunc (scon_status_t status,
                    void *cbdata)
{
    printf("\n %d********* succesfully deleted SCON ****** \n", handle);
    create =false;

}

/* msg send complete callback */
void send_cbfunc (scon_status_t status,
                  scon_handle_t scon_handle,
                  scon_proc_t *peer,
                  scon_buffer_t *buf,
                  scon_msg_tag_t tag,
                  void *cbdata)
{
    msg_sent = true;
    scon_output(0, " %s **** send_cbfunc : sent msg to %s on scon %d tag %d",
                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                SCON_PRINT_PROC(peer),
                scon_handle, tag);
    free(buf);
}

/* msg recv callback function */
void recv_cbfunc (scon_status_t status,
                  scon_handle_t scon_handle,
                  scon_proc_t *peer,
                  scon_buffer_t *buf,
                  scon_msg_tag_t tag,
                  void *cbdata)
{
    msg_rcvd = true;
    scon_output(0, " %s **** recv_cbfunc : recvd msg from %s on scon %d tag %d",
                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                SCON_PRINT_PROC(peer),
                scon_handle, tag);
}

int main(int argc, char **argv)
{
    int rc;
    scon_info_t *info;
    size_t  ninfo = 1;
    scon_proc_t  send_peer, recv_peer;
    unsigned int nmembers = 2;
    size_t nqueries = 1;
    unsigned int i;
    uint32_t msg;
    scon_msg_tag_t tag = 1000;
    scon_buffer_t *buf;
    /* init scon lib */
    printf("\n initializing SCON library \n");
    if(SCON_SUCCESS != (rc = scon_init(NULL,0))) {
        printf( "scon_init returned error %d", rc);
        return -1;
    }
    /* create SCON */
    handle = scon_create(NULL,0,
                         NULL,
                         0,
                         create_cbfunc,
                         NULL);
    /* wait for create to complete */
    while(!create) {
        sleep(1);
        // printf("scon create returned handle = %d create =%d \n", handle, create);
    }
    SCON_INFO_CREATE(info, ninfo);
    SCON_INFO_LOAD(&info[0], SCON_NUM_MEMBERS, &nmembers, SCON_UINT32);
    scon_get_info(handle, &info, &nqueries);
    nmembers = info[0].value.data.uint32;
    scon_output(0,  " %s testing send/recv on scon %d nmembers = %d", SCON_PRINT_PROC(SCON_PROC_MY_NAME),
           handle, info[0].value.data.uint32);
    /* set up my send and recv peers */
    send_peer.rank = (SCON_PROC_MY_NAME->rank + 1) %  nmembers;
    recv_peer.rank = (SCON_PROC_MY_NAME->rank -1);
    if( 0 == SCON_PROC_MY_NAME->rank)
        recv_peer.rank = nmembers - 1;
    strncpy(send_peer.job_name,SCON_PROC_MY_NAME->job_name, SCON_MAX_JOBLEN);
    strncpy(recv_peer.job_name,SCON_PROC_MY_NAME->job_name, SCON_MAX_JOBLEN);
    for(i = 0; i < NUM_MSG_CYCLES; i++)
    {
        msg_rcvd = false;
        // post a  recv for this tag from my prev proc
        scon_recv_nb (handle,
                      &recv_peer, tag, false,
                      recv_cbfunc,
                      NULL,
                      NULL, 0);
        msg = i + SCON_PROC_MY_NAME->rank + nmembers;
        buf = malloc(sizeof(scon_buffer_t));
        scon_buffer_construct(buf);
        scon_bfrop.pack(buf, &msg, 1, SCON_UINT32);
        if( 0 == SCON_PROC_MY_NAME->rank)
        {
            scon_output(0, "%s sending messsage : %d to %s", SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                  msg, SCON_PRINT_PROC(&send_peer));
            scon_send_nb(handle, &send_peer, buf, tag, send_cbfunc, NULL, NULL, 0);
            /* wait until we get our message back*/
            while(!msg_rcvd)
                sleep(1);
        }
        else {
            /* wait for msg from predeccesor then send */
            while(!msg_rcvd)
                sleep(1);
            msg_sent = false;
            scon_output(0, "%s rcvd messsage from %d  %s forwarding it to %s", SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                   msg, SCON_PRINT_PROC(&recv_peer), SCON_PRINT_PROC(&send_peer));
            scon_send_nb(handle, &send_peer, buf, tag, send_cbfunc, NULL, NULL, 0);
            while(!msg_sent)
                sleep(1);
        }
    }
    scon_delete(handle, delete_cbfunc, NULL, NULL, 0);
    while(create) {
        sleep(1);
    }
    scon_finalize();
    SCON_INFO_FREE(info, ninfo);
    //TEST_ERROR (("\n scon init returned %d", rc));
    return 0;
}
