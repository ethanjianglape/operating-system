#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    puts("Hello from musl userspace!");

    /* sleep(10); */

    return 0;
    /*
    DIR* dir = opendir("/dev");

    if (dir != NULL) {
        printf("dir = %d\n", dir);

        struct dirent* entry;

        while (1) {
            entry = readdir(dir);

            if (entry == NULL) {
                break;
            }

            printf("%s\n", entry->d_name);
        }

    } else {
        puts("Failed to opendir('/dev')");
    }

    sleep(1);

    return 0;
    */
}
