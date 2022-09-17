#pragma once

void cli_start(void);

// Register all system functions
void register_system(void);

// Register common system functions: "version", "restart", "free", "heap", "tasks"
void register_system_common(void);
