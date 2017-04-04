
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
#include <stdio.h>
#include <unistd.h>
static bool create = false;
static scon_handle_t handle;

void create_cbfunc (scon_status_t status,
                    scon_handle_t scon_handle,
                    void *cbdata);
void create_cbfunc (scon_status_t status,
                   scon_handle_t scon_handle,
                   void *cbdata)
{
    printf("\n*****************************create callback : status=%d, sconhandle= %d \n", status, scon_handle);
    handle = scon_handle;
    create = true;
}

int main(int argc, char **argv)
{
    int rc;
    scon_info_t *info;
    size_t  ninfo = 2;
    scon_proc_t *my_name;
    SCON_PROC_CREATE(my_name,1);
    my_name->rank = 0;
    unsigned int nmembers = 2;
    strncpy(my_name->job_name, "testscon",9);
    SCON_INFO_CREATE(info, ninfo);
   // TEST_ERROR(("\n first test initializing SCON "));
    printf("set my name = %s, my rank = %d", my_name->job_name, my_name->rank);
    SCON_INFO_LOAD(&info[0], SCON_MY_ID, my_name, SCON_PROC);
    SCON_INFO_LOAD(&info[1], SCON_JOB_RANKS, &nmembers, SCON_UINT32);
    if(SCON_SUCCESS != (rc = scon_init(info,ninfo))) {
        printf(0, "scon_init returned error %d", rc);
        return -1;
    }
    handle = scon_create(NULL,0,
                     NULL,
                     0,
                     create_cbfunc,
                     NULL);
    printf("scon create returned handle = %d create =%d \n", handle, create);
    while(!create) {
      sleep(1);
   // printf("scon create returned handle = %d create =%d \n", handle, create);
    }
    SCON_INFO_FREE(info, ninfo);
    SCON_PROC_FREE(my_name, 1);
   //TEST_ERROR (("\n scon init returned %d", rc));
    return 0;
}
