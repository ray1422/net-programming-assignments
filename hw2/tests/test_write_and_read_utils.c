#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "lib/utils.h"
const uint32_t N = 48763;
int main() {
    int fds[2];
    pipe(fds);
    if (fork()) {
        close(fds[0]);
        write_uint32_to_net(fds[1], N);
        char *str = "hello world!";
        write_uint32_to_net(fds[1], strlen(str));
        write(fds[1], str, strlen(str));
        close(fds[1]);
    } else {
        close(fds[1]);
        uint32_t n;
        read_uint32_from_net(fds[1], &n);
        assert(n == N);
        char buf[8192];
        read_n_and_string(fds[1], buf, 8192);
        assert(!strcmp(buf, "hello world!"));
        close(fds[0]);
    }
}