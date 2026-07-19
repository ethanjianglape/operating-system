#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    DIR* dir = opendir("./");

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    return 0;
}
