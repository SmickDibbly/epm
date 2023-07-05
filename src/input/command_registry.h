#ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H

#include "src/input/command.h"

extern epm_Command const CMD_quit;
extern epm_Command const CMD_log;

extern epm_Command const CMD_dumpvars;
extern epm_Command const CMD_var_add;
extern epm_Command const CMD_var_set;
extern epm_Command const CMD_op;
extern epm_Command const CMD_typify;

extern epm_Command const CMD_wiregeo;
extern epm_Command const CMD_wire_mode;
extern epm_Command const CMD_light_mode;
extern epm_Command const CMD_proj_mode;

//extern epm_Command const CMD_print_hash_tables;
//extern epm_Command const CMD_print_keybindings;
extern epm_Command const CMD_print_commands;
extern epm_Command const CMD_measure_BSPTree;

/* -------------------------------------------------------------------------- */
// Saving and loading worlds

extern epm_Command const CMD_saveworld;
extern epm_Command const CMD_loadworld;
extern epm_Command const CMD_unloadworld;

extern epm_Command const CMD_worldfromobj;

extern epm_Command const CMD_outputgeo;

//extern epm_Command const CMD_writeworld;
extern epm_Command const CMD_readworld;

/* -------------------------------------------------------------------------- */
// Texture-and-face-related.

extern epm_Command const CMD_apply_texture;
extern epm_Command const CMD_select_all_brush_faces;


/* -------------------------------------------------------------------------- */
// Geometry-related.

extern epm_Command const CMD_set_frame;
extern epm_Command const CMD_set_frame_point;
extern epm_Command const CMD_rebuild;
extern epm_Command const CMD_snap;
extern epm_Command const CMD_brush_from_frame;

#endif /* COMMAND_REGISTRY_H */
