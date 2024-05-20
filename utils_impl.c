#include "scales.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "app.h"

size_t readLine(char* buffer, size_t bufsize, FILE* fp) {
    char ch; size_t i = 0;
    while ((ch = fgetc(fp)) != EOF && i < bufsize-1) {
        buffer[i++] = ch;
        if (ch == '\n') break;
    }
    buffer[i] = '\0';
    return i;
}

// Read file into a heap-allocated char buffer.
char* readFile(const char* filename) {
    FILE* file = fopen(filename, "r"); // Open file for reading
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        return NULL;
    }

    // Allocate initial buffer size
    size_t buffer_size = 1024; // Initial buffer size
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        fclose(file);
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    size_t length = 0; // Current length of string in buffer

    // Read file line by line
    char line[1024]; // Buffer for reading lines
    size_t line_length = 0;
    while (( line_length = readLine(line, sizeof(line), file) ) != 0) {
        // Resize buffer if necessary 
        while (length + line_length + 1 > buffer_size) {
            buffer_size += buffer_size * 3 / 4;
            char* new_buffer = (char*)realloc(buffer, buffer_size);
            if (new_buffer == NULL) {
                fclose(file);
                free(buffer);
                fprintf(stderr, "Memory reallocation failed\n");
                return NULL;
            }
            buffer = new_buffer;
        }
        // Concatenate line to buffer
        strcpy(buffer + length, line);
        length += line_length;
    }

    fclose(file);

    // Null-terminate the buffer
    buffer[length] = '\0';

    return buffer;
}

// Unclean macros -- but works for now..
#define READ_FLOAT(name) if (strcmp(key, #name) == 0) {float v = atof(value); cfg -> name = v; continue; }
#define READ_UINT8(name) if (strcmp(key, #name) == 0) {uint8_t v = (uint8_t) atoi(value); cfg -> name = v; continue; }
#define READ_LONG(name) if (strcmp(key, #name) == 0) {long v = atoll(value); cfg -> name = v; continue; }

// Try to load app config from an INI file.
bool app_config_from_ini(AppConfig* cfg, const char* fpath) {
    FILE* file = fopen(fpath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s.\n", fpath);
        return false;
    }

    char line[256];
    char key[64];
    char value[192];

    while (fgets(line, sizeof(line), file)) {
        // Remove leading whitespace
        char *trimmed_line = line + strspn(line, "\r\n\t ");
        
        if (trimmed_line == NULL || trimmed_line[0] == ';' || trimmed_line[0] == '#') {
            // Ignore comments and empty lines
            continue;
        }
        
        // We don't need structuring like sections..
        if (trimmed_line[0] == '[' && trimmed_line[strlen(trimmed_line) - 1] == ']') {
            // Section found, ignore for now
            continue;
        }

        // Parse key-value pairs
        char *delimiter = strchr(trimmed_line, '=');
        if (delimiter == NULL) {
            printf("Invalid line: %s\n", trimmed_line);
            continue;
        }
        *delimiter = '\0'; // Split key and value
        strncpy(key, trimmed_line, 64);
        strncpy(value, delimiter + 1, 192);

        printf("debug [ini]: key: %s, value: %s", key, value);
        
        // Now..
        READ_FLOAT(dispersion_lower_bound);
        READ_FLOAT(dispersion_upper_bound);
        READ_FLOAT(d2_threshold);
        READ_FLOAT(index_rate);
        READ_UINT8(max_volume);
        READ_UINT8(program);
        READ_UINT8(let_ring);
        READ_LONG(cooldown_millis);

        if (strcmp(key, "scale") == 0) {
            char* delim = strchr(value, ':');
            *delim = '\0';
            printf("debug [ini]: scale_id = %s, root = %s\n", value, delim + 1);
            uint16_t i = atoi(value);
            tone_t t = midi_key_by_name(delim + 1);
            cfg -> scale = scale_from_id(i, t);
            continue;
        }

    }
    fclose(file);
    return true;
}