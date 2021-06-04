#define DEFAULT_SETTINGS {.N_THREAD_WORKERS=5, .MAX_STORAGE_SPACE=209715200, .STORAGE_BASE_SIZE= 2097152,.MAX_STORABLE_FILES=100, .SOCK_PATH=NULL, .PRINT_LOG=1}
#define ERROR_CONV 1
typedef struct{
    size_t MAX_STORAGE_SPACE;
    size_t STORAGE_BASE_SIZE;
    unsigned int N_THREAD_WORKERS;
    unsigned int MAX_STORABLE_FILES;
    int PRINT_LOG;
    char* SOCK_PATH;
} settings;

#ifndef CONFIG_PARSER_CONFIG_PARSER_H
#define CONFIG_PARSER_CONFIG_PARSER_H
void settings_free(settings* s);
void settings_load(settings* s, char* path);
void settings_default(settings* s);
void settings_print(settings s);
#endif //CONFIG_PARSER_CONFIG_PARSER_H
