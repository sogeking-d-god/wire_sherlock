#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socket_manager.h"
#include "engine_controller.h"

int main(int argc, char *argv[])
{
    char *pcap_path = NULL;
    char *socket_path = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--pcap") == 0 && i + 1 < argc) pcap_path = argv[++i];
        else if (strcmp(argv[i], "--socket") == 0 && i + 1 < argc) socket_path = argv[++i];
    }

    if (!pcap_path || !socket_path)
    {
        fprintf(stderr, "Usage: ./c_worker --pcap <file> --socket <path>\n");
        exit(EXIT_FAILURE);
    }

    int server_fd = create_unix_socket(socket_path);
    int client_sock = accept_client(server_fd);

    if (client_sock >= 0)
    {
        run_engine_loop(client_sock, pcap_path);
    }

    close_socket(client_sock, NULL);
    close_socket(server_fd, socket_path);
    return 0;
}