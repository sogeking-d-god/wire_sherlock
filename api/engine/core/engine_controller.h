#ifndef ENGINE_CONTROLLER_H
#define ENGINE_CONTROLLER_H

#include <stdint.h>

void run_engine_loop(int client_sock, const char *pcap_path);

#endif