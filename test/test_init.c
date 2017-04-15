
/**
 * Copyright (c) 2017 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
//#include "test_common.h"
#include <stdio.h>
#include <unistd.h>

#include "scon.h"
#include "scon_common.h"

#include "src/util/output.h"
#include "src/util/name_fns.h"
static bool create = false;
static scon_handle_t handle;
/** calback function declarations **/
void create_cbfunc (scon_status_t status,
                    scon_handle_t scon_handle,
                    void *cbdata);
void delete_cbfunc (scon_status_t status,
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

int main(int argc, char **argv)
{
    int rc;
    scon_info_t *info;
    size_t  ninfo = 1;
    size_t nqueries = 1;
    unsigned int nmembers = 0;
    printf( "\n initializing SCON lib");
    /* call SCON init */
    if(SCON_SUCCESS != (rc = scon_init(NULL,0))) {
        printf("\n scon_init returned error %d", rc);
        return -1;
    }
    printf("\n initialized SCON lib, creating SCON ");
    handle = scon_create(NULL,0,
                     NULL,
                     0,
                     create_cbfunc,
                     NULL);
    /* wait until create completes */
    while(!create) {
      sleep(1);
   // printf("scon create returned handle = %d create =%d \n", handle, create);
    }
    SCON_INFO_CREATE(info, ninfo);
    SCON_INFO_LOAD(&info[0], SCON_NUM_MEMBERS, &nmembers, SCON_UINT32);
    /* get num numbers */
    scon_get_info(handle, &info, &nqueries);
    /* to do: use unload value api instead of direct copy */
    nmembers = info[0].value.data.uint32;

    scon_output(0, "%s scon %d created with %d members done",
               SCON_PRINT_PROC(SCON_PROC_MY_NAME), handle, info[0].value.data.uint32 );
    /* delete the SCON */
    scon_delete(handle, delete_cbfunc, NULL, NULL, 0);
    /* wait for delete to complete */
    while(create) {
        sleep(1);
    }
    scon_output(0, "%s SCON %d deleted, calling finalize",
                SCON_PRINT_PROC(SCON_PROC_MY_NAME), handle);
    scon_finalize();
    SCON_INFO_FREE(info, ninfo);
    printf("\n test success: exiting");
    return 0;
}
