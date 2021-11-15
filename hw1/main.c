#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


void serve_img(FILE *fp) {
    char header[8192], header_fmt[] =
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Length: %ld\r\n"
                           "Content-Type: image/jpeg\r\n"
                           "\r\n";
    int img_fd = open("test.jpg", O_RDONLY);
    off_t fsize = lseek(img_fd, 0, SEEK_END);
    lseek(img_fd, 0, SEEK_SET);
    sprintf(header, header_fmt, fsize);
    fwrite(header, sizeof(char), strlen(header), fp);
    char buf[8192];

    for (int s = 0; (s = read(img_fd, buf, 1024)) > 0;) {
        fwrite(buf, sizeof(char), s, fp);
    }

    close(img_fd);
}

void serve_html(FILE *fp_send) {
    char header[8192], header_fmt[] =
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Length: %ld\r\n"
                           "Content-Type: text/html\r\n"
                           "\r\n";
    char content[] =
        "<!DOCTYPE html>"
        "<body>"
        "<form action='/upload' method='POST' enctype='multipart/form-data'>"
        "<input type='file' name='img' /><input type='submit' />"
        "<img width='500' src='/img'>"
        "</form>"
        "</body>\r\n";
    sprintf(header, header_fmt, strlen(content));
    fwrite(header, sizeof(char), strlen(header), fp_send);
    fwrite(content, sizeof(char), strlen(content), fp_send);
    fprintf(stderr, "serve html\n");
}

void serve_upload(FILE *fp_send, FILE *fp_recv, size_t content_length, const char boundary[]) {

    printf("fsize: %ld\n", content_length);
    int cnt = 0;
    char buf[8192];
    while (cnt < content_length) {
        fgets(buf, 8192, fp_recv);
        cnt += strlen(buf);
        fputs(buf, stderr);
        if (strstr(buf, "name=\"img\"") != NULL) {
            int l = 0;
            int out_fd = open("test.jpg", O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
            perror("out fd");
            for (char buf[8192]; cnt < content_length;) {
                int flg = 1;
                printf("%d %ld\n", cnt, content_length);
                fgets(buf, 8192, fp_recv);
                l = strlen(buf);
                cnt += l;
                printf("## %d %ld\n", cnt, content_length);
                if (flg && strcmp(buf, "\r\n")) {
                    fputs(buf, stderr);
                    continue;
                } else {
                    while (cnt < content_length) {
                        int read_len = content_length - cnt;
                        if (read_len > 8192) read_len = 8192;
                        l = fread(buf, sizeof(char), read_len, fp_recv);
                        cnt += l;
                        write(out_fd, buf, l);
                        // perror("write img");
                    }
                }
            }
            close(out_fd);
        }
    }

    char header_fmt[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %ld\r\n"
        "Content-Type: text/html\r\n"
        "\r\n";
    char content[8192] = "<h1>OK</h1>\n<img width='500' src='/img'>\r\n";
    fprintf(fp_send, header_fmt, strlen(content));
    fputs(content, fp_send);
}
int main() {
    int fd;
    unsigned val = 1;
    struct sockaddr_in sin;
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(8000);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("bind");
        exit(-1);
    }
    if (listen(fd, SOMAXCONN) < 0) {
        perror("listen");
        exit(-1);
    }

    signal(SIGCHLD, SIG_IGN);  // prevent child zombie
    while (1) {
        int p_fd = accept(fd, (struct sockaddr *)NULL, NULL);
        fprintf(stderr, "accept!\n");
        if (p_fd < 0) {
            perror("accept");
            continue;
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }
        if (pid == 0) {
            char boundary[8192];

            size_t fsize = 0;
            char buf_inp[8192];
            char method[8192], path[8192];
            FILE *fp_send = fdopen(dup(p_fd), "a+b");
            FILE *fp_recv = fdopen(dup(p_fd), "r+b");
            int br_cnt = 0;
            for (int i = 0; fgets(buf_inp, 8192, fp_recv); i++) {
                if (i == 0) {
                    sscanf(buf_inp, "%s %s", method, path);
                    printf("method: %s, path: %s\n", method, path);
                    continue;
                }
                char a[8192], b[8192];
                sscanf(buf_inp, "%s %s", a, b);
                fprintf(stderr, "a: %s, b: %s\n", a, b);
                if (!strcmp(a, "Content-Length:")) {
                    sscanf(b, "%ld", &fsize);
                    fprintf(stderr, "content length: %ld\n", fsize);
                }
                if ((!strcmp(a, "Content-Type:")) && (!(strcmp(b, "multipart/form-data;")))) {
                    char boundary_buf[8192];
                    sscanf(buf_inp, "%s %s %s", a, b, boundary_buf);
                    strcat(boundary, "--");
                    strcat(boundary, boundary_buf + strlen("boundary="));

                    printf("BOUND: %s\n", boundary);
                }
                printf("# ");
                fputs(buf_inp, stdout);
                if (!strcmp(buf_inp, "\r\n")) br_cnt++;
                if (br_cnt == 1) break;
            }
            if (!strcmp("/img", path)) {
                serve_img(fp_send);
            } else if (!strcmp("/upload", path)) {
                serve_upload(fp_send, fp_recv, fsize, boundary);
            } else {
                serve_html(fp_send);
            }
            // while (read(p_fd, buf_inp, 8192) > 0) {
            //     printf("%s", buf_inp);
            //     if (strstr(buf_inp, "\r\n\r\n") != NULL) break;
            // }
            fclose(fp_send);
            fclose(fp_recv);
            close(p_fd);
            puts("");
            exit(0);
        }
        close(p_fd);
    }
}
