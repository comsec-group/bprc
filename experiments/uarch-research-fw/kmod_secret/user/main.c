// SPDX-License-Identifier: GPL-3.0-only
#include <stdint.h>
#include <stdio.h>

#include "../secret_ioctl.h"

int main(int argc, char const *argv[])
{
    secret_init();

    // get the address of the secret data for testing an exploit
    uint64_t secret_ptr = 0;
    if (secret_get_addr(&secret_ptr))
        err(1, "failed to get secret addr\n");

    // retrieve the secret data to verify results
    uint8_t secret_reference[SECRET_DATA_SIZE];
    if (secret_get_data((void *)secret_reference, SECRET_DATA_SIZE))
        err(1, "failed to get secret addr\n");

    printf("secret: 0x%hhx, 0x%hhx, %0xhhx, 0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx\n",
           secret_reference[0],
           secret_reference[1],
           secret_reference[2],
           secret_reference[3],
           secret_reference[4],
           secret_reference[5],
           secret_reference[6],
           secret_reference[7]);

    /* code */
    return 0;
}
