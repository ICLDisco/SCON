
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
static bool create = false;
void create_cbfunc (scon_status_t status,
                   scon_handle_t scon_handle,
                   void *cbdata)
{
    printf("create callback : status=%d, sconhandle= %d \n", status, scon_handle);
    create = true;
}

int main(int argc, char **argv)
{
    int rc;
    scon_info_t *info;
    size_t sz, ninfo = 2;
    scon_proc_t *my_name, *check_name;
    SCON_PROC_CREATE(my_name,1);
    my_name->rank = 0;
    unsigned int nmembers = 2;
    strncpy(my_name->job_name, "testscon",9);
    SCON_INFO_CREATE(info, ninfo);
   // TEST_ERROR(("\n first test initializing SCON "));
    printf("set my name = %s, my rank = %d", my_name->job_name, my_name->rank);
    SCON_INFO_LOAD(&info[0], SCON_MY_ID, my_name, SCON_PROC);
    printf(" loaded my name space %s, my rank %d into info[0]",
           info[0].value.data.proc->job_name, info[0].value.data.proc->rank);
    scon_value_unload(&info[0].value, (void **)&check_name,
                      &sz,  SCON_PROC);
    printf(" unloaded my name space %s, my rank %d into checkname",
           check_name->job_name, check_name->rank);
   SCON_INFO_LOAD(&info[1], SCON_JOB_RANKS, &nmembers, SCON_UINT32);
   rc = scon_init(info,ninfo);
   /*rc = scon_create(NULL,0,
                     NULL,
                     0,
                     create_cbfunc,
                     NULL);
    printf("scon create returned rc = %d \n", rc);
    if (SCON_SUCCESS != rc) {
       while(!create)
          printf("waiting for create lets sleep for a bit \n ");
    }*/
    //SCON_INFO_FREE(info, ninfo);
   // SCON_PROC_FREE(my_name, 1);
   // TEST_ERROR (("\n scon init returned %d", rc));
    return 0;
}

