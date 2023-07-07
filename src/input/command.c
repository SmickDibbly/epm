#include "src/locallibs/dibhash.h"

#include "src/input/command.h"
#include "src/input/command_registry.h"

static Index *cmd_index;

static epm_Command const *cmds[] = {
    &CMD_setfps,
    
    &CMD_quit,
    &CMD_log,

    &CMD_wiregeo,
    &CMD_wire_mode,
    &CMD_light_mode,
    &CMD_proj_mode,
    &CMD_worldfromobj,
    
    &CMD_print_commands,
    &CMD_measure_BSPTree,
    
    &CMD_dumpvars,
    &CMD_var_add,
    &CMD_var_set,
    &CMD_op,
    &CMD_typify,

    &CMD_saveworld,
    &CMD_loadworld,
    &CMD_unloadworld,
    &CMD_readworld,
    &CMD_rebuild,
    &CMD_outputgeo,
    
    &CMD_apply_texture,
    &CMD_select_all_brush_faces,
    &CMD_loadtex,

    &CMD_brush_from_frame,
    &CMD_set_frame,
    &CMD_set_frame_point,
    &CMD_snap,
};
static size_t const num_cmds = sizeof(cmds)/sizeof(cmds[0]);

static int epm_TokenizeCommandString(char *cmd_str, char **argv);

epm_Result epm_InitCommand(void) {
    cmd_index = create_Index(0, NULL, "Commands", 0);

    for (size_t i_cmd = 0; i_cmd < num_cmds; i_cmd++) {
        Index_insert(cmd_index, (StrInt){cmds[i_cmd]->name, i_cmd});
    }
    
    return EPM_SUCCESS;
}

epm_Result epm_TermCommand(void) {
    destroy_Index(cmd_index);
    
    return EPM_SUCCESS;
}

#define MAX_CMD_ARGC 16

#define lower_c "abcdefghijklmnopqrstuvwxyz"
#define upper_c "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define alpha_c "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define digit_c "0123456789"
#define xdigit_c "0123456789ABCDEFabcdef"
#define print_c " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
#define graph_c "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
#define punct_c "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define alnum_c "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

//static char command_name_chars[] = alnum_c ".";
//static char space_chars[] = " \t\r\v\f"; // no newline

/*
static bool is_class(char const *valid_chars_str, char const ch) {
    size_t len = strlen(valid_chars_str);
    for (size_t i_ch = 0; i_ch < len; i_ch++) {
        if (ch == valid_chars_str[i_ch]) {
            return true;
        }
    }

    return false;
}
*/


static int epm_TokenizeCommandString(char *cmd_str, char **argv) {
    /* If cmd_str has no string literals within it, then this simply tokenizes
       the cmd_str using ' ' as the delimiter. Otherwise, string literals are
       parsed as normal. */
    
    int argc = 0;
    
    char const *space_delim = " ";
    char const *quote_delim = "\"";
    
    char *token;
    token = strtok(cmd_str, space_delim);
    while (token != NULL) {
        if (token[0] == '\"') {
            size_t len = strlen(token);
            token[len] = space_delim[0];

            token = strtok(token, quote_delim);
            len = strlen(token);
            *(token-1)   = '\"'; // TODO: Should already be a quote, but maybe
                                 // we should just check
            token[len]   = '\"';
            token[len+1] = '\0';
            token--;
        }
        argv[argc] = token;
        argc++;
        if (argc == MAX_CMD_ARGC) break;
        
        token = strtok(NULL, space_delim);
    }

    return argc;
}

void epm_SubmitCommandString(char const *in_cmd_str, char *output_str) {
    (void)output_str;
    
    char cmd_str[256];
    strcpy(cmd_str, in_cmd_str);
    
    int argc = 0;
    char *argv[MAX_CMD_ARGC] = {NULL};

    argc = epm_TokenizeCommandString(cmd_str, argv);

    if (argc < 1) return;    
    char *cmd_name = argv[0];

    size_t i_cmd;
    epm_Command const *p_cmd;
    if (Index_lookup(cmd_index, cmd_name, &i_cmd)) {
        p_cmd = cmds[i_cmd];
    }
    else {
        sprintf(output_str, "Unknown command \"%s\".", cmd_name);
        return;
    }

    if (argc < p_cmd->argc_min) {
        sprintf(output_str, "Command \"%s\" takes at least %i arguments, but only %i given.", cmd_name, p_cmd->argc_min-1, argc-1);
        return;
    }
    if (p_cmd->argc_max < argc) {
        sprintf(output_str, "Command \"%s\" takes at most %i arguments, but %i given.", cmd_name, p_cmd->argc_max-1, argc-1);
        return;
    }
    p_cmd->handler(argc, argv, output_str);
}




void print_commands(void) {
    size_t i_buf = 0;

    for (size_t i_cmd = 0; i_cmd < num_cmds; i_cmd++) {
        i_buf += snprintf(bigbuf + i_buf, BIGBUF_LEN - i_buf, "%s\n", cmds[i_cmd]->name);
    }

    puts(bigbuf);
}

static void CMDH_print_commands(int argc, char **argv, char *output_str) {
    print_commands();
    strcpy(output_str, bigbuf);
}

epm_Command const CMD_print_commands = {
    .name = "print_commands",
    .argc_min = 1,
    .argc_max = 1,
    .handler = CMDH_print_commands,
};
