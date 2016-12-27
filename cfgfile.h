#define PERCORE_CLI 69

int cfg_checks( const yabauseinit_struct cfg );
size_t cfg_loadf( FILE *f, yabauseinit_struct *cfg );
size_t cfg_savef( FILE *f, const yabauseinit_struct cfg );
void cfg_inis( yabauseinit_struct *cfg );
