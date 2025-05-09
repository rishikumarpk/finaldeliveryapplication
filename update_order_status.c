#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define ORDERS_FILE "C:\\Apache24\\data\\orders.json"
#define DEBUG_LOG "C:\\Apache24\\logs\\debug.log"
#define MAX_POST_SIZE 10000
#define MAX_BUFFER_SIZE 102400

void write_debug(const char *msg) {
    FILE *f = fopen(DEBUG_LOG, "a");
    if (f) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec, msg);
        fclose(f);
    }
}

char *read_post_data() {
    char *len_str = getenv("CONTENT_LENGTH");
    if (!len_str) return NULL;
    int len = atoi(len_str);
    if (len <= 0 || len > MAX_POST_SIZE) return NULL;

    char *data = malloc(len + 1);
    if (!data) return NULL;
    
    size_t bytes_read = fread(data, 1, len, stdin);
    if (bytes_read != len) {
        free(data);
        return NULL;
    }
    
    data[len] = '\0';
    return data;
}

// Simple function to check if a string is a valid JSON object
int is_valid_json(const char *s) {
    if (!s) return 0;
    while (*s && isspace((unsigned char)*s)) s++;
    return *s == '{';
}

// Improved function to extract a value from JSON (handles both quoted and unquoted values)
char *extract_json_value(const char *json, const char *field) {
    if (!json || !field) return NULL;
    
    // Format the field name with quotes and colon
    char search_str[256];
    snprintf(search_str, sizeof(search_str), "\"%s\"", field);
    
    // Find the field in the JSON
    char *field_pos = strstr(json, search_str);
    if (!field_pos) return NULL;
    
    // Move past the field name
    field_pos += strlen(search_str);
    
    // Skip whitespace and find the colon
    while (*field_pos && isspace((unsigned char)*field_pos)) field_pos++;
    if (*field_pos != ':') return NULL;
    field_pos++; // Skip the colon
    
    // Skip whitespace before the value
    while (*field_pos && isspace((unsigned char)*field_pos)) field_pos++;
    
    // Check if the value is a string (starts with quote)
    int is_string = (*field_pos == '"');
    char *value_start = field_pos;
    
    if (is_string) {
        value_start++; // Skip opening quote
        
        // Find the closing quote (but not escaped quotes)
        char *value_end = value_start;
        int escaped = 0;
        while (*value_end) {
            if (*value_end == '\\') {
                escaped = !escaped;
            } else if (*value_end == '"' && !escaped) {
                break;
            } else {
                escaped = 0;
            }
            value_end++;
        }
        
        if (!*value_end) return NULL; // No closing quote found
        
        // Extract the string value
        int value_len = value_end - value_start;
        char *value = malloc(value_len + 1);
        if (!value) return NULL;
        
        strncpy(value, value_start, value_len);
        value[value_len] = '\0';
        
        return value;
    } else {
        // Handle non-string values (numbers, booleans, etc.)
        char *value_end = value_start;
        
        // Find the end of the value (comma, closing brace, or whitespace)
        while (*value_end && *value_end != ',' && *value_end != '}' && !isspace((unsigned char)*value_end)) {
            value_end++;
        }
        
        // Extract the value
        int value_len = value_end - value_start;
        char *value = malloc(value_len + 1);
        if (!value) return NULL;
        
        strncpy(value, value_start, value_len);
        value[value_len] = '\0';
        
        return value;
    }
}

// Improved function to update the status of an order in the orders.json file
int update_order_status(const char *order_id, const char *new_status) {
    if (!order_id || !new_status) {
        write_debug("Invalid parameters");
        return 0;
    }
    
    char debug_msg[512];
    snprintf(debug_msg, sizeof(debug_msg), "Trying to update order ID: %s with status: %s", order_id, new_status);
    write_debug(debug_msg);
    
    FILE *fp = fopen(ORDERS_FILE, "r");
    if (!fp) {
        write_debug("Failed to open orders file for reading");
        return 0;
    }
    
    // Read the entire orders file
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (fileSize <= 0 || fileSize > MAX_BUFFER_SIZE) {
        write_debug("Orders file is empty or too large");
        fclose(fp);
        return 0;
    }
    
    char *buffer = malloc(fileSize + 1);
    if (!buffer) {
        write_debug("Memory allocation failed");
        fclose(fp);
        return 0;
    }
    
    size_t bytesRead = fread(buffer, 1, fileSize, fp);
    buffer[bytesRead] = '\0';
    fclose(fp);
    
    if (bytesRead != fileSize) {
        write_debug("Failed to read entire orders file");
        free(buffer);
        return 0;
    }
    
    // First, check if we're dealing with a JSON array or a single object
    char *content = buffer;
    while (*content && isspace((unsigned char)*content)) content++;
    
    if (*content != '[' && *content != '{') {
        write_debug("Orders file does not contain valid JSON");
        free(buffer);
        return 0;
    }
    
    int is_array = (*content == '[');
    
    // Search pattern for the order ID
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"orderId\"\\s*:\\s*\"%s\"", order_id);
    
    char *order_pos = buffer;
    int found = 0;
    
    // Find the order with the matching ID
    while ((order_pos = strstr(order_pos, "\"orderId\"")) != NULL) {
        // Skip to the value after "orderId":
        char *value_start = order_pos + 9; // Length of "orderId"
        
        // Find the colon
        while (*value_start && *value_start != ':') value_start++;
        if (!*value_start) break;
        value_start++; // Skip the colon
        
        // Skip whitespace
        while (*value_start && isspace((unsigned char)*value_start)) value_start++;
        
        // Check if value is quoted
        int is_quoted = (*value_start == '"');
        char *id_start = value_start;
        
        if (is_quoted) {
            id_start++; // Skip opening quote
            
            // Find closing quote
            char *id_end = strchr(id_start, '"');
            if (!id_end) break;
            
            // Extract ID
            int id_len = id_end - id_start;
            char current_id[256] = {0};
            
            if (id_len < sizeof(current_id)) {
                strncpy(current_id, id_start, id_len);
                current_id[id_len] = '\0';
                
                if (strcmp(current_id, order_id) == 0) {
                    found = 1;
                    
                    // Find the object bounds for this order
                    char *obj_start = id_end;
                    // Go backward to find the opening brace
                    while (obj_start > buffer && *obj_start != '{') obj_start--;
                    
                    if (*obj_start != '{') {
                        write_debug("Could not find opening brace for order object");
                        free(buffer);
                        return 0;
                    }
                    
                    // Find closing brace - need to handle nested objects
                    char *obj_end = id_end;
                    int brace_count = 1;
                    while (*obj_end && brace_count > 0) {
                        obj_end++;
                        if (*obj_end == '{') brace_count++;
                        else if (*obj_end == '}') brace_count--;
                    }
                    
                    if (brace_count != 0) {
                        write_debug("Could not find matching closing brace for order object");
                        free(buffer);
                        return 0;
                    }
                    
                    // Look for a status field in this order
                    char *status_field = strstr(id_end, "\"status\"");
                    
                    // If status field exists within the order object bounds
                    if (status_field && status_field < obj_end) {
                        // Find the value start (after colon)
                        char *status_colon = strchr(status_field + 8, ':');
                        if (!status_colon || status_colon >= obj_end) {
                            write_debug("Invalid status field format");
                            free(buffer);
                            return 0;
                        }
                        
                        status_colon++; // Move past colon
                        
                        // Skip whitespace
                        while (*status_colon && isspace((unsigned char)*status_colon)) status_colon++;
                        
                        // Check if current status is quoted
                        int status_quoted = (*status_colon == '"');
                        char *status_value_start = status_colon;
                        char *status_value_end;
                        
                        if (status_quoted) {
                            status_value_start++; // Skip opening quote
                            status_value_end = strchr(status_value_start, '"');
                            if (!status_value_end || status_value_end >= obj_end) {
                                write_debug("Could not find closing quote for status value");
                                free(buffer);
                                return 0;
                            }
                            status_value_end++; // Include closing quote in replacement
                        } else {
                            // Find end of unquoted value
                            status_value_end = status_value_start;
                            while (*status_value_end && *status_value_end != ',' && 
                                   *status_value_end != '}' && !isspace((unsigned char)*status_value_end)) {
                                status_value_end++;
                            }
                        }
                        
                        // Create replacement with quoted new status
                        char new_status_quoted[256];
                        snprintf(new_status_quoted, sizeof(new_status_quoted), "\"%s\"", new_status);
                        
                        // Calculate new buffer size
                        size_t prefix_len = status_colon - buffer;
                        size_t old_value_len = status_value_end - status_colon;
                        size_t new_value_len = strlen(new_status_quoted);
                        size_t suffix_len = fileSize - (status_value_end - buffer);
                        
                        char *new_buffer = malloc(prefix_len + new_value_len + suffix_len + 1);
                        if (!new_buffer) {
                            write_debug("Memory allocation failed for new buffer");
                            free(buffer);
                            return 0;
                        }
                        
                        // Copy before status value
                        strncpy(new_buffer, buffer, prefix_len);
                        
                        // Copy new status
                        strcpy(new_buffer + prefix_len, new_status_quoted);
                        
                        // Copy after status value
                        strcpy(new_buffer + prefix_len + new_value_len, status_value_end);
                        
                        // Write back to file
                        fp = fopen(ORDERS_FILE, "w");
                        if (!fp) {
                            write_debug("Failed to open orders file for writing");
                            free(new_buffer);
                            free(buffer);
                            return 0;
                        }
                        
                        fputs(new_buffer, fp);
                        fclose(fp);
                        
                        write_debug("Successfully updated status field");
                        free(new_buffer);
                        free(buffer);
                        return 1;
                    } 
                    else {
                        // Status field doesn't exist, add it
                        // Find position to insert (right after the orderId field and its value)
                        char *insert_pos = id_end + 1; // Move past closing quote of orderId value
                        
                        // Skip whitespace and find comma or closing brace
                        while (*insert_pos && isspace((unsigned char)*insert_pos)) insert_pos++;
                        
                        // If comma found, insert after it
                        char *insert_after = NULL;
                        if (*insert_pos == ',') {
                            insert_after = insert_pos + 1;
                        } else {
                            // No comma after orderId, insert before closing brace with a comma
                            insert_after = insert_pos;
                            // In this case, we'll insert a comma first
                        }
                        
                        // Skip whitespace after insertion point
                        while (*insert_after && isspace((unsigned char)*insert_after)) insert_after++;
                        
                        // Prepare new status JSON
                        char status_json[256];
                        if (*insert_pos == ',') {
                            snprintf(status_json, sizeof(status_json), " \"status\": \"%s\",", new_status);
                        } else {
                            snprintf(status_json, sizeof(status_json), ", \"status\": \"%s\"", new_status);
                        }
                        
                        // Calculate new buffer size
                        size_t prefix_len = insert_after - buffer;
                        size_t status_len = strlen(status_json);
                        size_t suffix_len = fileSize - prefix_len;
                        
                        char *new_buffer = malloc(prefix_len + status_len + suffix_len + 1);
                        if (!new_buffer) {
                            write_debug("Memory allocation failed");
                            free(buffer);
                            return 0;
                        }
                        
                        // Copy before insertion point
                        strncpy(new_buffer, buffer, prefix_len);
                        
                        // Add status field
                        strcpy(new_buffer + prefix_len, status_json);
                        
                        // Copy rest of buffer
                        strcpy(new_buffer + prefix_len + status_len, insert_after);
                        
                        // Write back to file
                        fp = fopen(ORDERS_FILE, "w");
                        if (!fp) {
                            write_debug("Failed to open orders file for writing");
                            free(new_buffer);
                            free(buffer);
                            return 0;
                        }
                        
                        fputs(new_buffer, fp);
                        fclose(fp);
                        
                        write_debug("Successfully added new status field");
                        free(new_buffer);
                        free(buffer);
                        return 1;
                    }
                }
            }
            
            // Move past this ID for next iteration
            order_pos = id_end + 1;
        } else {
            // Handle numeric orderId (unquoted)
            char *id_end = id_start;
            while (*id_end && (*id_end == '_' || isalnum((unsigned char)*id_end))) id_end++;
            
            // Extract ID
            int id_len = id_end - id_start;
            char current_id[256] = {0};
            
            if (id_len < sizeof(current_id)) {
                strncpy(current_id, id_start, id_len);
                current_id[id_len] = '\0';
                
                if (strcmp(current_id, order_id) == 0) {
                    // Found matching order, process as above...
                    // Same logic as for quoted IDs (mostly duplicate code for clarity)
                    found = 1;
                    write_debug("Found matching order with unquoted ID");
                    
                    // Find the object bounds for this order
                    char *obj_start = id_end;
                    // Go backward to find the opening brace
                    while (obj_start > buffer && *obj_start != '{') obj_start--;
                    
                    if (*obj_start != '{') {
                        write_debug("Could not find opening brace for order object");
                        free(buffer);
                        return 0;
                    }
                    
                    // Find closing brace - handle nested objects
                    char *obj_end = id_end;
                    int brace_count = 1;
                    while (*obj_end && brace_count > 0) {
                        obj_end++;
                        if (*obj_end == '{') brace_count++;
                        else if (*obj_end == '}') brace_count--;
                    }
                    
                    if (brace_count != 0) {
                        write_debug("Could not find matching closing brace for order object");
                        free(buffer);
                        return 0;
                    }
                    
                    // Look for a status field in this order
                    char *status_field = strstr(id_end, "\"status\"");
                    
                    // If status field exists within the order object bounds
                    if (status_field && status_field < obj_end) {
                        // Almost same logic as above, but handle both quoted/unquoted status
                        char *status_colon = strchr(status_field + 8, ':');
                        if (!status_colon || status_colon >= obj_end) {
                            write_debug("Invalid status field format");
                            free(buffer);
                            return 0;
                        }
                        
                        status_colon++; // Move past colon
                        
                        // Skip whitespace
                        while (*status_colon && isspace((unsigned char)*status_colon)) status_colon++;
                        
                        // Check if current status is quoted
                        int status_quoted = (*status_colon == '"');
                        char *status_value_start = status_colon;
                        char *status_value_end;
                        
                        if (status_quoted) {
                            status_value_start++; // Skip opening quote
                            status_value_end = strchr(status_value_start, '"');
                            if (!status_value_end || status_value_end >= obj_end) {
                                write_debug("Could not find closing quote for status value");
                                free(buffer);
                                return 0;
                            }
                            status_value_end++; // Include closing quote in replacement
                        } else {
                            // Find end of unquoted value
                            status_value_end = status_value_start;
                            while (*status_value_end && *status_value_end != ',' && 
                                   *status_value_end != '}' && !isspace((unsigned char)*status_value_end)) {
                                status_value_end++;
                            }
                        }
                        
                        // Create replacement with quoted new status
                        char new_status_quoted[256];
                        snprintf(new_status_quoted, sizeof(new_status_quoted), "\"%s\"", new_status);
                        
                        // Calculate new buffer size
                        size_t prefix_len = status_colon - buffer;
                        size_t suffix_len = fileSize - (status_value_end - buffer);
                        size_t new_value_len = strlen(new_status_quoted);
                        
                        char *new_buffer = malloc(prefix_len + new_value_len + suffix_len + 1);
                        if (!new_buffer) {
                            write_debug("Memory allocation failed for new buffer");
                            free(buffer);
                            return 0;
                        }
                        
                        // Copy before status value
                        strncpy(new_buffer, buffer, prefix_len);
                        
                        // Copy new status
                        strcpy(new_buffer + prefix_len, new_status_quoted);
                        
                        // Copy after status value
                        strcpy(new_buffer + prefix_len + new_value_len, status_value_end);
                        
                        // Write back to file
                        fp = fopen(ORDERS_FILE, "w");
                        if (!fp) {
                            write_debug("Failed to open orders file for writing");
                            free(new_buffer);
                            free(buffer);
                            return 0;
                        }
                        
                        fputs(new_buffer, fp);
                        fclose(fp);
                        
                        write_debug("Successfully updated status field for unquoted ID");
                        free(new_buffer);
                        free(buffer);
                        return 1;
                    } else {
                        // Add status field (similar to previous code)
                        char *insert_pos = id_end;
                        
                        // Skip whitespace and find comma or closing brace
                        while (*insert_pos && isspace((unsigned char)*insert_pos)) insert_pos++;
                        
                        char *insert_after = NULL;
                        if (*insert_pos == ',') {
                            insert_after = insert_pos + 1;
                        } else {
                            insert_after = insert_pos;
                        }
                        
                        // Skip whitespace after insertion point
                        while (*insert_after && isspace((unsigned char)*insert_after)) insert_after++;
                        
                        // Prepare new status JSON
                        char status_json[256];
                        if (*insert_pos == ',') {
                            snprintf(status_json, sizeof(status_json), " \"status\": \"%s\",", new_status);
                        } else {
                            snprintf(status_json, sizeof(status_json), ", \"status\": \"%s\"", new_status);
                        }
                        
                        // Calculate new buffer size
                        size_t prefix_len = insert_after - buffer;
                        size_t status_len = strlen(status_json);
                        size_t suffix_len = fileSize - prefix_len;
                        
                        char *new_buffer = malloc(prefix_len + status_len + suffix_len + 1);
                        if (!new_buffer) {
                            write_debug("Memory allocation failed");
                            free(buffer);
                            return 0;
                        }
                        
                        // Copy before insertion point
                        strncpy(new_buffer, buffer, prefix_len);
                        
                        // Add status field
                        strcpy(new_buffer + prefix_len, status_json);
                        
                        // Copy rest of buffer
                        strcpy(new_buffer + prefix_len + status_len, insert_after);
                        
                        // Write back to file
                        fp = fopen(ORDERS_FILE, "w");
                        if (!fp) {
                            write_debug("Failed to open orders file for writing");
                            free(new_buffer);
                            free(buffer);
                            return 0;
                        }
                        
                        fputs(new_buffer, fp);
                        fclose(fp);
                        
                        write_debug("Successfully added new status field for unquoted ID");
                        free(new_buffer);
                        free(buffer);
                        return 1;
                    }
                }
            }
            
            // Move past this ID for next iteration
            order_pos = id_end;
        }
    }
    
    write_debug(found ? "Order found but update failed" : "Order ID not found");
    free(buffer);
    return 0;
}

int main() {
    printf("Content-Type: application/json\r\n\r\n");
    
    // Write debug info about this request
    write_debug("----- Update order status request received -----");
    
    char *post_data = read_post_data();
    char debug_msg[512];
    
    if (!post_data) {
        write_debug("No POST data received");
        printf("{\"success\":false,\"message\":\"No data received\"}");
        return 1;
    }
    
    // Log the received data for debugging
    snprintf(debug_msg, sizeof(debug_msg), "Received data: %s", post_data);
    write_debug(debug_msg);
    
    if (!is_valid_json(post_data)) {
        write_debug("Invalid JSON received");
        printf("{\"success\":false,\"message\":\"Invalid JSON data\"}");
        free(post_data);
        return 1;
    }
    
    // Extract order ID and new status from the POST data
    char *order_id = extract_json_value(post_data, "orderId");
    char *new_status = extract_json_value(post_data, "status");
    
    if (!order_id) {
        write_debug("Missing orderId in request");
        printf("{\"success\":false,\"message\":\"Missing orderId\"}");
        if (new_status) free(new_status);
        free(post_data);
        return 1;
    }
    
    if (!new_status) {
        write_debug("Missing status in request");
        printf("{\"success\":false,\"message\":\"Missing status\"}");
        free(order_id);
        free(post_data);
        return 1;
    }
    
    // Log the extracted values
    snprintf(debug_msg, sizeof(debug_msg), "Extracted orderId: %s, status: %s", order_id, new_status);
    write_debug(debug_msg);
    
    // Update the order status
    int success = update_order_status(order_id, new_status);
    
    if (success) {
        printf("{\"success\":true,\"message\":\"Order status updated\"}");
        write_debug("Order status updated successfully");
    } else {
        printf("{\"success\":false,\"message\":\"Failed to update order status\"}");
        write_debug("Failed to update order status");
    }
    
    free(order_id);
    free(new_status);
    free(post_data);
    return 0;
}