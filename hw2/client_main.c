#include <argp.h>
#include <stdio.h>
#include <stdlib.h>

const char *argp_program_version = "ttt-server 0.0.1";
const char *argp_program_bug_address = "<TODO>";

/* Program documentation. */
static char doc[] = "TODO";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options we understand. */
static struct argp_option options[] = {
    {"host", 'h', "localhost", 0, "TTT server host", OPTION_ARG_OPTIONAL},
    {"port", 'p', "6666", 0, "TTT server port", OPTION_ARG_OPTIONAL},
    {0}};

/* Used by main to communicate with parse_opt. */
struct arguments {
    char *args[1];
    char *host;
    int port;
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case 'h':
            arguments->host = arg;
            break;
        case 'p':
            arguments->port = atoi(arg);
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
    arguments.host = "localhost";
    arguments.port = 6666;

    /* Parse our arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    printf("%s:%d\n", arguments.host, arguments.port);
    exit(0);
}
