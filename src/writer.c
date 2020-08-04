#include "writer.h"
#include "client-server.h"
#include "defs.h"

/*
 * TODO
 *  [ ] document the buffer, first 64bytes for flag and pathname, rest for content
 */

void prepend_flag(int flag, char *buf) {
    // assumes buf is null terminated
    // https://stackoverflow.com/questions/2328182/prepending-to-a-string
    // tried countlessly but it doesn't like + '0' when memmoving

    char tmp = flag + '0';
    size_t len = sizeof(tmp);
    memmove(buf + len, buf, strlen(buf) + 1);
    buf[0] = tmp; // feels dirtier than mmove but works
}

void append_path(const char *path, char *buf, int padding) {

    size_t len = strlen(path);
    snprintf(&buf[padding], len + 1, "%s", path);
}

void append_content(char *content, char *buf, int padding) {

    size_t len = strlen(content);
    snprintf(&buf[padding], len + 1, "%s", content);
}

void fetch_from_server(char *filebuf, const char *fp, char *buf, int flag, int serversock, int pipe) {
    char tmpbuf[MAX] = {0};
    char reply[MAX] = {0};

    prepend_flag(flag, tmpbuf);
    append_path(fp, tmpbuf, 1);
    append_content(filebuf, tmpbuf, MAX_PATH);

    write(serversock, tmpbuf, strlen(tmpbuf));
    read(pipe, &reply, sizeof(reply)); // wait for the final file from reader process

    snprintf(buf, strlen(reply), "%s", reply);
}


