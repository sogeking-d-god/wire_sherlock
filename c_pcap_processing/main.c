#include <main.h>



int main(int argc, char** argv)
{
    main_return_codes_e ret_val = MAIN_SUCCESS;
    parser_return_codes_e parse_ret;

    char file_path [ FILENAME_MAX + 1];
    if (argc < 2)
    {
        printf("enter file path:\t");

        if (fgets(file_path, sizeof(file_path), stdin) != NULL)
        {
            file_path[strcspn(file_path, "\n")] = 0;
        }
        else
        {
            fprintf(stderr, "Error reading pcap file name\n");
            ret_val = MAIN_FILE_NAME_INPUT_ERROR;
        }
    }
    else
    {
        strncpy(file_path, argv[1], FILENAME_MAX);
        file_path[FILENAME_MAX] = '\0';
    }

    if (strlen(file_path) == 0) {
        printf("Error: Empty file path provided.\n");
        ret_val = MAIN_FILE_NAME_LENGTH_ZERO_ERROR;
    }

    if (ret_val == MAIN_SUCCESS)
    {

        printf("Starting analysis on: %s\n", file_path);
        parse_ret = parse_pcap_file(file_path);
        printf("Analysis completed with code: %d\n", parse_ret);
    }

    return ret_val;
}
