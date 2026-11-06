#include <uhd.h>

#include <stdio.h>


int main(void) {
    int r;

    // Create USRP
    uhd_usrp_handle usrp;
    printf("Creating USRP ...\n");

    r = uhd_usrp_make(&usrp, "type=n3xx,addr=192.168.21.109,master_clock_rate=125e6");
    if (r != UHD_ERROR_NONE) {
        fprintf(stderr, "Error creating USRP. r = %d\n", r);
        return 1;
    }

    return 0;
}
