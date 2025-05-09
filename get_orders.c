#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define ORDERS_FILE "C:\\Apache24\\data\\orders.json"
#define DEBUG_LOG "C:\\Apache24\\logs\\debug.log"
#define MAX_BUFFER_SIZE 10240

void write_debug(const char* message) {
    FILE *debug = fopen(DEBUG_LOG, "a");
    if (debug) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(debug, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec, message);
        fclose(debug);
    }
}

// Helper function to validate JSON
int is_valid_json(const char* json_str) {
    if (!json_str || *json_str == '\0') return 0;
    
    // Skip whitespace
    while (isspace((unsigned char)*json_str)) json_str++;
    
    // Must start with { or [
    if (*json_str != '{' && *json_str != '[') return 0;
    
    // Simple bracket matching check
    int braces = 0, brackets = 0;
    char in_string = 0;
    char last_char = 0;
    
    while (*json_str) {
        if (!in_string) {
            if (*json_str == '{') braces++;
            else if (*json_str == '}') braces--;
            else if (*json_str == '[') brackets++;
            else if (*json_str == ']') brackets--;
        }
        
        if (*json_str == '"' && last_char != '\\') {
            in_string = !in_string;
        }
        
        last_char = *json_str;
        json_str++;
    }
    
    return (braces == 0 && brackets == 0);
}

int main() {
    char debug_msg[512];
    write_debug("Starting get_orders.cgi");
    
    // Send headers first
    printf("Content-Type: application/json\r\n");
    printf("Cache-Control: no-cache, no-store, must-revalidate\r\n");
    printf("Pragma: no-cache\r\n");
    printf("Expires: 0\r\n\r\n");
    
    FILE *fp = fopen(ORDERS_FILE, "r");
    if (!fp) {
        sprintf(debug_msg, "Error opening orders file: %s", ORDERS_FILE);
        write_debug(debug_msg);
        printf("[]");
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (fileSize == 0) {
        write_debug("Orders file is empty");
        printf("[]");
        fclose(fp);
        return 0;
    }
    
    // Limit file size for safety
    if (fileSize > MAX_BUFFER_SIZE) {
        fileSize = MAX_BUFFER_SIZE;
        write_debug("Warning: File size exceeds buffer limit, truncating");
    }
    
    char *buffer = (char*)malloc(fileSize + 1);
    if (!buffer) {
        write_debug("Memory allocation failed");
        printf("[]");
        fclose(fp);
        return 1;
    }
    
    size_t bytesRead = fread(buffer, 1, fileSize, fp);
    buffer[bytesRead] = '\0';
    fclose(fp);
    
    // Clean up the buffer (remove comments, trim whitespace)
    char *p = buffer;
    char *q = buffer;
    int in_string = 0;
    
    // Simple cleanup - strip non-JSON content
    while (*p) {
        if (*p == '"' && (p == buffer || *(p-1) != '\\')) {
            in_string = !in_string;
        }
        
        if (in_string || (*p >= ' ' && *p != 127)) {
            *q++ = *p;
        }
        p++;
    }
    *q = '\0';
    
    // Validate JSON
    if (is_valid_json(buffer)) {
        printf("%s", buffer);
        write_debug("Successfully served valid JSON");
    } else {
        write_debug("Invalid JSON detected, serving empty array");
        printf("[]");
    }
    
    free(buffer);
    return 0;
}