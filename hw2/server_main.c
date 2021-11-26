#include <argp.h>
#include <stdio.h>
#include <stdlib.h>

#include "ttt_server.h"
const char *argp_program_version = "ttt-server 0.0.1";
const char *argp_program_bug_address = "<TODO>";

/* Program documentation. */
static char doc[] = "TODO";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options we understand. */
static struct argp_option options[] = {
    {"port", 'p', "6666", 0, "listening port", OPTION_ARG_OPTIONAL},
    {"addr", 'a', "0.0.0.0", 0, "listening address", OPTION_ARG_OPTIONAL},
    {0}};

/* Used by main to communicate with parse_opt. */
struct arguments {
    char *args[1];
    int port;
    char *addr;
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case 'p':
            arguments->port = atoi(arg);
            break;

        case 'a':
            arguments->addr = arg;
            break;

        case ARGP_KEY_END:
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* Our argp parser. */
static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char **argv) {
    struct arguments arguments;

    /* Default values. */
    arguments.port = 6666;
    arguments.addr = "0.0.0.0";

    /* Parse our arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    printf("Start TTT Server on %s:%d\n", arguments.addr, arguments.port);
    return ttt_server(arguments.addr, arguments.port);
}
