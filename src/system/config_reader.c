#include "src/locallibs/dibhash.h"

#include "src/misc/epm_includes.h"
#include "src/system/dir.h"

#include "src/system/config_reader.h"

//#define VERBOSITY
#include "src/locallibs/verbosity.h"

#undef LOG_LABEL
#define LOG_LABEL "SYS.CONFIG"

#define lower_c "abcdefghijklmnopqrstuvwxyz"
#define upper_c "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define alpha_c "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define digit_c "0123456789"
#define xdigit_c "0123456789ABCDEFabcdef"
#define print_c " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
#define graph_c "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
#define punct_c "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define alnum_c "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

char group_name_chars[] = alnum_c ".";
char key_chars[] = alnum_c "-_";
char val_chars[] = print_c;
char space_chars[] = " \t\r\v\f"; // no newline

static bool is_class(char const *valid_chars_str, int const ch) {
    size_t len = strlen(valid_chars_str);
    for (size_t i = 0; i < len; i++) {
        if (ch == valid_chars_str[i]) {
            return true;
        }
    }

    return false;
}

// line consisting of whitespace only
#define EMPTY_LINE 0
// line starting with optional whitespace followed by '['
#define GROUP_LINE 1
// line starting with optional whitespace followed by a char in
// label_name_chars[]
#define ENTRY_LINE 2
// line starting with '#'
#define COMMENT_LINE 3
// a line consisting of optional whitespace followed by EOF
#define EOF_LINE 5
// line which cannot be classified
#define ERROR_LINE 4 

static int config_classify_line(FILE *in_fp) {
    long int init_fpos = ftell(in_fp);
    int res;
    int ch;
    
    while (is_class(space_chars, ch = fgetc(in_fp)));
    // ch is now first non-space character on line, or EOF

    if (ch == EOF) {
        res = EOF_LINE;
    }
    else if (ch == '#') {
        res = COMMENT_LINE;
    }
    else if (ch == '[') {
        res = GROUP_LINE;
    }
    else if (ch == '\n') {
        res = EMPTY_LINE;
    }
    else if (is_class(key_chars, ch)) {
        res = ENTRY_LINE;
    }
    else {
        res = ERROR_LINE;
    }

    fseek(in_fp, init_fpos, SEEK_SET);
    return res;
}

typedef struct ConfigGroup ConfigGroup;
typedef struct ConfigEntry ConfigEntry;

struct ConfigGroup {
    ConfigGroup *next_group;
    ConfigEntry *first_entry;

    size_t num_entries;
    char *name;
};

struct ConfigEntry {
    ConfigGroup *category;
    ConfigEntry *next_entry;

    char *key;
    char *val;
};

typedef struct Config {
    size_t num_groups;
    ConfigGroup *first_group;

    size_t num_entries;
} Config;

static Config g_config = {0};
static size_t num_dicts = 0;
static Dict *dicts[64]; // one dict per group, max 64 groups 

extern void print_config(Config *config);
extern epm_Result write_config(char const *configfile);
extern epm_Result destroy_config(void);

static int read_ConfigEntry(ConfigEntry *e, FILE *in_fp) {
    char keybuf[256] = {'\0'};
    char valbuf[256] = {'\0'};
    char *p_keybuf = keybuf, *p_valbuf = valbuf;
    
    int ch;
    //char *label = e->key;
    //char *value = e->val;
    
    // read entry label
    while (is_class(key_chars, ch = fgetc(in_fp))) {
        *p_keybuf = (char)ch;
        p_keybuf++;
    }
    *p_keybuf = '\0';

    // skip whitespace
    while (is_class(space_chars, ch = fgetc(in_fp)));

    // confirm equal sign
    dibassert(ch == '=');

    // skip whitespace
    while (is_class(space_chars, ch = fgetc(in_fp)));

    // read entry value
    do {
        *p_valbuf = (char)ch;
        p_valbuf++;
    } while (is_class(val_chars, ch = fgetc(in_fp)));
    *p_valbuf = '\0';


    e->key = zgl_Malloc((strlen(keybuf) + 1)*sizeof(*e->key));
    strcpy(e->key, keybuf);
    e->val = zgl_Malloc((strlen(valbuf) + 1)*sizeof(*e->val));
    strcpy(e->val, valbuf);
    
    return 0;
}

static int read_ConfigGroup_name(ConfigGroup *g, FILE *in_fp) {
    char buf[256] = {'\0'};
    char *p_buf = buf;
    int ch;

    ch = fgetc(in_fp);
    dibassert(ch == '[');

    //char *name = g->name;
            
    while (is_class(group_name_chars, ch = fgetc(in_fp))) {
        *p_buf = (char)ch;
        p_buf++;
    }
    *p_buf = '\0';

    g->name = zgl_Malloc((strlen(buf) + 1)*sizeof(*g->name));
    strcpy(g->name, buf);
    
    if (ch != ']') {
        epm_Log(LT_ERROR, "Group name must be followed by ']'; got '%c' instead.", ch);
        return -1;
    }
    while (is_class(space_chars, ch = fgetc(in_fp)));
    if (ch != '\n') {
        epm_Log(LT_ERROR, "Only whitespace may follow a bracketed group name; found '%c' instead.", ch);
        return -1;
    }
    
    return 0;
}

epm_Result read_config(char const *configfile) {
    char path[256] = {'\0'};
    dibassert(256 > strlen(DIR_DATA));
    memcpy(path, DIR_DATA, strlen(DIR_DATA));
    dibassert(strlen(path) + strlen(configfile) < 256);
    strcat(path, configfile);
    
    FILE *in_fp = fopen(path, "rb");
    if ( ! in_fp) {
        epm_Log(LT_ERROR, "Could not open file %s.", path);
        return EPM_ERROR;
    }
    
    Config config = {0};
    ConfigGroup *curr_group = NULL;
    ConfigEntry *curr_entry = NULL;
    int line_type;
    
    for (;;) {
        // beginning of group
        int ch;
        line_type = config_classify_line(in_fp);
        
        switch (line_type) {
        case EMPTY_LINE:
            while ('\n' != (ch = fgetc(in_fp)));
            continue;
            break;
        case COMMENT_LINE:
            while ('\n' != (ch = fgetc(in_fp)));
            continue;
            break;
        case GROUP_LINE:
            if (curr_group == NULL) {
                config.first_group =
                    zgl_Calloc(1, sizeof(*config.first_group));
                curr_group = config.first_group;
            }
            else {
                curr_group->next_group =
                    zgl_Calloc(1, sizeof(*curr_group->next_group));
                curr_group = curr_group->next_group;
            }

            curr_group->num_entries = 0;
            curr_entry = NULL;
            config.num_groups++;
            
            if (-1 == read_ConfigGroup_name(curr_group, in_fp)) {
                goto END;
            }
            break;
        case EOF_LINE:
            goto END;
            break;
        case ENTRY_LINE:
        case ERROR_LINE:
            epm_Log(LT_ERROR, "Expected empty, comment, EOF, or group line; expectation was wrong.\n");
            goto END;
            break;
        }
        
        line_type = config_classify_line(in_fp);
        bool keep_looping = true;
        
        while (keep_looping) {
            line_type = config_classify_line(in_fp);

            switch (line_type) {
            case EMPTY_LINE:
                while ('\n' != (ch = fgetc(in_fp)));
                continue;
                break;
            case COMMENT_LINE:
                while ('\n' != (ch = fgetc(in_fp)));
                continue;
                break;
            case ENTRY_LINE:
                if (curr_entry == NULL) {
                    curr_group->first_entry =
                        zgl_Calloc(1, sizeof(*curr_group->first_entry));
                    curr_entry = curr_group->first_entry;
                }
                else {
                    curr_entry->next_entry =
                        zgl_Calloc(1, sizeof(*curr_entry->next_entry));
                    curr_entry = curr_entry->next_entry;
                }
                curr_group->num_entries++;
                config.num_entries++;
                
                if (-1 == read_ConfigEntry(curr_entry, in_fp)) {
                    goto END;
                }
                break;
            case EOF_LINE:
                goto END;
                break;
            case GROUP_LINE:
                keep_looping = false;
                continue;
                break;
            case ERROR_LINE:
                epm_Log(LT_ERROR, "Expected empty, comment, EOF, or entry line; expectation was wrong.\n");
                goto END;
                break;
            }
        }
    }

 END:
    fclose(in_fp);

#ifdef VERBOSITY
    print_config(&config);
#endif
    g_config = config;

    write_config(configfile);

    for (ConfigGroup *g = config.first_group; g; g = g->next_group) {
        char grpname[256] = "Config Group: ";
        strcat(grpname, g->name);
        dicts[num_dicts] = create_Dict(0, NULL, grpname, 0);
        for (ConfigEntry *e = g->first_entry; e; e = e->next_entry) {
            Dict_insert(dicts[num_dicts], (StrStr){e->key, e->val});
        }
        num_dicts++;
    }
    
    return EPM_SUCCESS;
}

char *get_config_str(char const *group, char *key) {
    char *val;
    size_t index = 0;
    
    for (ConfigGroup *grp = g_config.first_group; grp; grp = grp->next_group) {
        if (0 == strcmp(grp->name, group)) {
            val = Dict_lookup(dicts[index], key);
            //fprintf(stdout, "%s : %s\n", key, val);
            return val;
        }
        index++;
    }

    return NULL;
}


void print_config(Config *config) {
    printf("(%zu groups with %zu entries)\n\n", config->num_groups, config->num_entries);
    
    for (ConfigGroup *g = config->first_group; g; g = g->next_group) {
        printf("[%s] (%zu entries)\n", g->name, g->num_entries);
        for (ConfigEntry *e = g->first_entry; e; e = e->next_entry) {
            printf("%s = %s\n", e->key, e->val);
        }

        putchar('\n');
    }
}


#include "zigil/zigil_keycodes.h"

/*
void read_keymap(char const *group, size_t mapsize, char *strs[], zgl_LongKeyCode *map) {
    char *str;
    zgl_LongKeyCode LK;

    for (size_t ka = 0; ka < mapsize; ka++) {
        str = get_config_str(group, strs[ka]);
        if (str != NULL) {
            LK = LK_str_to_code(str);
            if (LK != LK_NONE) {
                map[ka] = LK;
            }
        }
        puts(LK_strs[map[ka]]);
    }
}
*/

extern void read_config_data
(char const *group,
 size_t num_queries,
 ConfigMapEntry const queries[],
 void (*str_to_var)(void *, char const *)) {
    char const *val_str;
    
    for (size_t i = 0; i < num_queries; i++) {
        val_str = get_config_str(group, queries[i].query);
        if (val_str != NULL) {
            str_to_var(queries[i].var, val_str);
        }
    }
}

epm_Result write_config(char const *configfile) {
    FILE *out_fp;

    if (configfile == NULL) {
        out_fp = stdout;
    }
    else {
        char path[256] = {'\0'};
        dibassert(256 > strlen(DIR_DATA));
        memcpy(path, DIR_DATA, strlen(DIR_DATA));
        strcat(path, ".out.");
        dibassert(strlen(path) + strlen(configfile) < 256);
        strcat(path, configfile);

        out_fp = fopen(path, "wb");
        if ( ! out_fp) {
            epm_Log(LT_ERROR, "Could not open config file %s; writing to stdout instead.\n", path);
            out_fp = stdout;
        }
    }

    for (ConfigGroup *p_grp = g_config.first_group; p_grp; p_grp = p_grp->next_group) {
        fprintf(out_fp, "[%s]\n", p_grp->name);
        for (ConfigEntry *p_ent = p_grp->first_entry; p_ent; p_ent = p_ent->next_entry) {
            fprintf(out_fp, "%s = %s\n", p_ent->key, p_ent->val);
        }
        fputc('\n', out_fp);
    }
    fputc('\n', out_fp);

    if (out_fp != stdout) {
        fclose(out_fp);
    }
    
    return EPM_SUCCESS;
}

epm_Result destroy_config(void) {
    for (ConfigGroup *p_grp = g_config.first_group; p_grp;) {
        // delete entries
        for (ConfigEntry *p_ent = p_grp->first_entry; p_ent;) {
            zgl_Free(p_ent->key);
            zgl_Free(p_ent->val);
            
            ConfigEntry *p_nextent = p_ent->next_entry;
            zgl_Free(p_ent);
            p_ent = p_nextent;
            
        }

        zgl_Free(p_grp->name);
        
        ConfigGroup *p_nextgrp = p_grp->next_group;
        zgl_Free(p_grp);
        p_grp = p_nextgrp;
    }

    for (size_t index = 0; index < num_dicts; index++) {
        destroy_Dict(dicts[index]);
    }
    

    return ZR_SUCCESS;
}




epm_Result epm_InitConfig(void) {    
    if (read_config("config.ini") != EPM_SUCCESS) {
        _epm_Log("INIT.SYS", LT_ERROR, "Failed to read config file.");
        return EPM_FAILURE;
    }

    return EPM_SUCCESS;
}
    
epm_Result epm_TermConfig(void) {
    // TODO: Save with any new data.
    
    destroy_config();

    return EPM_SUCCESS;
}
    
