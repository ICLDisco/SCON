
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
void recv_cbfunc (scon_status_t status,
                  scon_handle_t scon_handle,
                  scon_proc_t *peer,
                  scon_buffer_t *buf,
                  scon_msg_tag_t tag,
                  void *cbdata);
/* callback functions **/
/** create callback **/
void create_cbfunc (scon_status_t status,
                    scon_handle_t scon_handle,
                    void *cbdata)
{
    printf("\n*****************************create callback : status=%d, sconhandle= %d \n", status, scon_handle);
    handle = scon_handle;
    create = true;
}
/* delete callback */
void delete_cbfunc (scon_status_t status,
                    void *cbdata)
{
    printf("\n %d********* succesfully deleted SCON ****** \n", handle);
    create =false;

}
/* xcast send complete callback*/
void xcast_cbfunc (scon_status_t status,
                   scon_handle_t scon_handle,
                   scon_proc_t procs[],
                   size_t nprocs,
                   scon_buffer_t *buf,
                   scon_msg_tag_t tag,
                   scon_info_t info[],
                   size_t ninfo,
                   void *cbdata)
{
    msg_sent = true;
    scon_output(0, " %s **** xcast_cbfunc : sent xcast on scon %d tag %d",
                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
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
    unsigned int nmembers = 2;
    size_t nqueries = 1;
    unsigned int i;
    uint32_t msg;
    scon_msg_tag_t tag = 1000;
    scon_buffer_t *buf;
    if(SCON_SUCCESS != (rc = scon_init(NULL, 0))) {
        printf(0, "scon_init returned error %d", rc);
        return -1;
    }
    handle = scon_create(NULL,0,
                         NULL,
                         0,
                         create_cbfunc,
                         NULL);
    while(!create) {
        /* sleep until we get a callback */
        sleep(1);

    }
    /* get num of members using get_info*/
    SCON_INFO_CREATE(info, ninfo);
    SCON_INFO_LOAD(&info[0], SCON_NUM_MEMBERS, &nmembers, SCON_UINT32);
    scon_get_info(handle, &info, &nqueries);
    /* get the value directly for now */
    /* TO DO: use scon_value_unload */
    nmembers = info[0].value.data.uint32;
    scon_output(0, " %s testing xcast on scon %d nmembers = %d",
                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                handle, info[0].value.data.uint32);
    // now lets test xcast ith member broadcast on ith iteration and the rest
    // of the members recv it
    tag = 10000;
    for (i=0; i< nmembers; i++)
    {
        tag = tag +i;
        msg_rcvd = false;
        /* all members post a recv for the msg */
        scon_recv_nb(handle,
                     SCON_PROC_WILDCARD, tag, false,
                     recv_cbfunc,
                     NULL,
                     NULL, 0);
        if (i == SCON_PROC_MY_NAME->rank) {
            msg = i + SCON_PROC_MY_NAME->rank +nmembers;
            buf = malloc(sizeof(scon_buffer_t));
            scon_buffer_construct(buf);
            scon_bfrop.pack(buf, &msg, 1, SCON_UINT32);
            scon_xcast(handle, NULL, 0, buf, tag, xcast_cbfunc, NULL, NULL, 0);
        }
        while(!msg_rcvd)
            sleep(1);
    }
    /* delete the SCON */
    scon_delete(handle, delete_cbfunc, NULL, NULL, 0);
    while(create) {
        sleep(1);
    }
    /* finalize the lib */
    scon_finalize();
    SCON_INFO_FREE(info, ninfo);
    return 0;
}
