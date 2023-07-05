#include "src/locallibs/dibhash.h"
#include "src/misc/dyvar.h"
#include "src/input/command.h"

static Index *var_index;

epm_Result epm_InitDynamicVariables(void) {
    var_index = create_Index(0, NULL, "Dynamic Variables", 0);
    
    return EPM_SUCCESS;
}

epm_Result epm_TermDynamicVariables(void) {
    destroy_Index(var_index);
    
    return EPM_SUCCESS;
}

static size_t num_vars = 0;
static epm_DyVar vars[64] = {0};

static bool var_plus(epm_DyVar *in_var1, epm_DyVar *in_var2, epm_DyVar *out_var) {
    if (in_var1->type == TYPE_INT && in_var2->type == TYPE_INT) {
        out_var->type = TYPE_INT;
        out_var->data.i = in_var1->data.i + in_var2->data.i;
        return true;
    }
    else if (in_var1->type == TYPE_STRING && in_var2->type == TYPE_STRING) {
        out_var->type = TYPE_STRING;
        out_var->data.str = zgl_Malloc(1 + strlen(in_var1->data.str) + strlen(in_var2->data.str));
        strcpy(out_var->data.str, in_var1->data.str);
        strcat(out_var->data.str, in_var2->data.str);
        return true;
    }
    else {
        return false;
    }
}

static epm_DyVarType typify_data_str(char const *str) {
    if (str[0] == '\"') { // if the string starts with " it must end with " and
                          // have no other " in between.
        if (strlen(str) == 1) {
            return TYPE_UNKNOWN;
        }
        else if (str[strlen(str) - 1] != '\"') {
            return TYPE_UNKNOWN;
        }
        
        for (size_t i_ch = 1; i_ch < strlen(str) - 1; i_ch++) {
            if (str[i_ch] == '\"') {
                return TYPE_UNKNOWN;
            }
        }

        return TYPE_STRING;
    }
    
    return TYPE_INT;
}

static void CMDH_do_variable(int argc, char **argv, char *output_str) {
    (void)output_str;
    (void)argc;
    
    char *id = argv[1];
    char *data_str = argv[3];

    epm_DyVarType new_type = typify_data_str(data_str);
    if (new_type == TYPE_UNKNOWN) {
        epm_Log(LT_INFO, "String \"%s\" can't be translated to any type.", data_str);
        sprintf(output_str, "String \"%s\" can't be translated to any type.", data_str);
    }
    
    epm_DyVar *var;
    
    uint64_t i_var;
    if (Index_lookup(var_index, id, &i_var)) {
        var = vars + i_var;
        if (var->type == TYPE_STRING)
            zgl_Free(var->data.str);
    }
    else {
        i_var = num_vars;
        num_vars++;
        var = vars + i_var;
        strcpy(var->id, id);
        Index_insert(var_index, (StrInt){var->id, i_var});
    }

    var->type = new_type;
    switch (new_type) {
    case TYPE_STRING: {
        size_t trimmed_len = strlen(data_str)-2;
        var->data.str = zgl_Malloc(1 + trimmed_len);
        memcpy(var->data.str, data_str + 1, trimmed_len);
        var->data.str[trimmed_len] = '\0';
        sprintf(output_str, "Set variable %s to value \"%s\"", var->id, var->data.str);
    }
        break;
    case TYPE_INT:
        var->data.i = atoi(data_str);
        sprintf(output_str, "Set variable %s to value %li", var->id, var->data.i);
        break;
    case TYPE_FLOAT:
        var->data.f = atof(data_str);
        sprintf(output_str, "Set variable %s to value %lf", var->id, var->data.f);
        break;
    default:
        break;
    }
}



static void CMDH_add(int argc, char **argv, char *output_str) {
    (void)output_str;
    (void)argc;

    int64_t var1 = 0, var2 = 0;

    for (size_t i_var = 0; i_var < num_vars; i_var++) {
        if (strcmp(vars[i_var].id, argv[1]) == 0)
            var1 = vars[i_var].data.i;
        if (strcmp(vars[i_var].id, argv[2]) == 0)
            var2 = vars[i_var].data.i;
    }

    printf("%li + %li = %li\n", var1, var2, var1+var2);
}

static void CMDH_dumpvars(int argc, char **argv, char *output_str) {
    (void)output_str;
    (void)argc;
    (void)argv;

    size_t res;
   
    for (size_t i_var = 0; i_var < num_vars; i_var++) {
        epm_DyVar *var = vars + i_var;
        switch (var->type) {
        case TYPE_STRING:
            res = sprintf(output_str, "%s = \"%s\"\n", var->id, var->data.str);
            output_str += res;
            break;
        case TYPE_INT:
            res = sprintf(output_str, "%s = %li\n", var->id, var->data.i);
            output_str += res;
            break;
        default:
            break;
        }
    }
}

static void CMDH_varop(int argc, char **argv, char *output_str) {
    char *val1_str = argv[1];
    char *op_str = argv[2];
    char *val2_str = argv[3];
    epm_DyVar *var1, *var2, res;

    size_t i_var;
    if ( ! Index_lookup(var_index, val1_str, &i_var))
        return;
    var1 = &vars[i_var];
    
    if ( ! Index_lookup(var_index, val2_str, &i_var))
        return;
    var2 = &vars[i_var];
    
    if (strcmp(op_str, "+") == 0) {
        if (var_plus(var1, var2, &res)) {
            if (res.type == TYPE_INT) {
                sprintf(output_str, "%li + %li = %li\n", var1->data.i, var2->data.i, res.data.i);
            }
            else if (res.type == TYPE_STRING) {
                sprintf(output_str, "\"%s\" + \"%s\" = \"%s\"\n", var1->data.str, var2->data.str, res.data.str);
            }
        }
        else {
            sprintf(output_str, "Incompatible operands to binary \"+\" operation.");
        }
    }
}

epm_Command const CMD_dumpvars = {
    .name = "dumpvars",
    .argc_min = 1,
    .argc_max = 1,
    .handler = CMDH_dumpvars,
};

epm_Command const CMD_var_add = {
    .name = "add",
    .argc_min = 3,
    .argc_max = 3,
    .handler = CMDH_add,
};

epm_Command const CMD_var_set = {
    .name = "var",
    .argc_min = 4,
    .argc_max = 4,
    .handler = CMDH_do_variable,
};

epm_Command const CMD_op = {
    .name = "op",
    .argc_min = 4,
    .argc_max = 4,
    .handler = CMDH_varop,
};



static void CMDH_typify(int argc, char **argv, char *output_str) {
    epm_DyVarType type = typify_data_str(argv[1]);

    switch (type) {
    case TYPE_INT:
        strcpy(output_str, "INT");
        break;
    case TYPE_STRING:
        strcpy(output_str, "STRING");
        break;
    default:
        strcpy(output_str, "UNKNOWN");
        break;
    }
}

epm_Command const CMD_typify = {
    .name = "typify",
    .argc_min = 2,
    .argc_max = 2,
    .handler = CMDH_typify,
};
