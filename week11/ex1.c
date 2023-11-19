#include <stdio.h>
#include <dirent.h>

typedef struct dirent dirent;

int main() {
    DIR* const dir = opendir("/");

    for (dirent* e = readdir(dir);; e = readdir(dir)) {
        if (e == NULL)
            break;

        puts(e->d_name);
    }

    closedir(dir);
    return 0;
}