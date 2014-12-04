/*
 * Copyright © 2012 Benjamin Franzke
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <poll.h>
#include <errno.h>

#include <error.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <weston-launcher.h>

static void
help(const char *name)
{
    fprintf(stderr, "Usage: %s [args...] [-- [orbital args..]]\n", name);
    fprintf(stderr, "  -u, --user      Start session as specified username\n");
    fprintf(stderr, "  -t, --tty       Start session on alternative tty\n");
    fprintf(stderr, "  -v, --verbose   Be verbose\n");
    fprintf(stderr, "  -h, --help      Display this help message\n");
}

int
main(int argc, char *argv[])
{
    struct weston_launcher *wl;
    int i, c, ret;
    char *tty = NULL;
    char *new_user = NULL;
    int verbose = 0;

    struct option opts[] = {
        { "user",    required_argument, NULL, 'u' },
        { "tty",     required_argument, NULL, 't' },
        { "verbose", no_argument,       NULL, 'v' },
        { "help",    no_argument,       NULL, 'h' },
        { 0,         0,                 NULL,  0  }
    };

    while ((c = getopt_long(argc, argv, "u:t::vh", opts, &i)) != -1) {
        switch (c) {
        case 'u':
            new_user = optarg;
            break;
        case 't':
            tty = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            help("orbital-launch");
            exit(EXIT_FAILURE);
        }
    }

    wl = weston_launcher_create(new_user, tty, verbose);
    if (wl == NULL)
        exit(EXIT_FAILURE);

    ret = weston_launcher_run_compositor(wl, BIN_PATH "/orbital",
                         argc - optind, argv + optind);

    weston_launcher_destroy(wl);
    return ret;
}
