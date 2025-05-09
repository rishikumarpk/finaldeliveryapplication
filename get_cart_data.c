#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PRODUCTS 500  // Increased for multiple categories
#define MAX_CART_ITEMS 100
#define MAX_LINE_LENGTH 512
#define DEBUG_LOG "C:\\Apache24\\logs\\debug.log"

typedef struct Product //holds product data from their respective text files
{
    char name[64];
    char image[256];
    int price;
    int stock;
} Product;

typedef struct CartItem //hold product data from cart.txt
{
    char name[64];
    int quantity;
    int price;
} CartItem;

// Debug logging function
void write_debug(const char* message) {
    FILE *debug = fopen(DEBUG_LOG, "a");
    if (debug) {
        fprintf(debug, "%s\n", message);
        fclose(debug);
    }
}

// Function to escape JSON strings
void json_escape(FILE *out, const char *str) {
    fputc('"', out);
    for (; *str; str++) {
        switch (*str) {
            case '"':  fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            case '\b': fputs("\\b", out);  break;
            case '\f': fputs("\\f", out);  break;
            case '\n': fputs("\\n", out);  break;
            case '\r': fputs("\\r", out);  break;
            case '\t': fputs("\\t", out);  break;
            default:
                if (iscntrl((unsigned char)*str))
                    fprintf(out, "\\u%04x", (unsigned char)*str);
                else
                    fputc(*str, out);
        }
    }
    fputc('"', out);
}

// Load products from multiple category files
int load_all_products(Product products[], int max_products) {
    const char *categories[] = {"fruits", "vegetables", "snacks", "dairy", "cleaning", "beverages"};
    int num_categories = sizeof(categories) / sizeof(categories[0]);
    int total_products = 0;
    
    for (int c = 0; c < num_categories; c++) {
        char path[512];
        snprintf(path, sizeof(path), "C:\\Apache24\\data\\%s.txt", categories[c]);
        
        FILE *fp = fopen(path, "r");
        if (!fp) {
            char debug_msg[512];
            snprintf(debug_msg, sizeof(debug_msg), "Failed to open category file: %s", path);
            write_debug(debug_msg);
            continue; // Skip if file not found
        }
        
        char line[MAX_LINE_LENGTH];
        while (total_products < max_products && fgets(line, sizeof(line), fp)) {
            // Remove newline characters
            line[strcspn(line, "\r\n")] = '\0';
            
            // Skip empty lines
            if (line[0] == '\0') continue;
            
            // Parse the line
            char *token = strtok(line, ",");
            if (!token) continue;
            
            // Copy product name
            strncpy(products[total_products].name, token, sizeof(products[total_products].name)-1);
            products[total_products].name[sizeof(products[total_products].name)-1] = '\0';
            
            // Copy image path
            token = strtok(NULL, ",");
            if (!token) continue;
            strncpy(products[total_products].image, token, sizeof(products[total_products].image)-1);
            products[total_products].image[sizeof(products[total_products].image)-1] = '\0';
            
            // Get price
            token = strtok(NULL, ",");
            if (!token) continue;
            products[total_products].price = atoi(token);
            
            // Get stock
            token = strtok(NULL, ",");
            if (!token) continue;
            products[total_products].stock = atoi(token);
            
            total_products++;
        }
        
        fclose(fp);
    }
    
    char debug_msg[128];
    snprintf(debug_msg, sizeof(debug_msg), "Loaded %d products successfully", total_products);
    write_debug(debug_msg);
    
    return total_products;
}

// Find a cart item by name
int find_cart_item(CartItem cart_items[], int count, const char *name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(cart_items[i].name, name) == 0) {
            return i;
        }
    }
    return -1; // Not found
}

// Load cart items from file - format: name,quantity
int load_cart(const char *path, CartItem cart_items[], int max_items, Product products[], int product_count) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        write_debug("Failed to open cart file. Cart may be empty.");
        return 0; // Return 0 instead of -1 to indicate empty cart
    }
    
    int count = 0;
    char line[MAX_LINE_LENGTH];
    
    while (count < max_items && fgets(line, sizeof(line), fp)) {
        // Remove newline characters
        line[strcspn(line, "\r\n")] = '\0';
        
        // Skip empty lines
        if (line[0] == '\0') continue;
        
        // Parse the line
        char *token = strtok(line, ",");
        if (!token) continue;
        
        // Get product name
        char name[64];
        strncpy(name, token, sizeof(name)-1);
        name[sizeof(name)-1] = '\0';
        
        // Get quantity
        token = strtok(NULL, ",");
        if (!token) continue;
        int quantity = atoi(token);
        
        // Skip items with zero quantity
        if (quantity <= 0) continue;
        
        // Check if item already exists in cart
        int existing_idx = find_cart_item(cart_items, count, name);
        
        if (existing_idx >= 0) {
            // Update existing item quantity
            cart_items[existing_idx].quantity += quantity;
        } else {
            // Add new item to cart
            strncpy(cart_items[count].name, name, sizeof(cart_items[count].name)-1);
            cart_items[count].name[sizeof(cart_items[count].name)-1] = '\0';
            cart_items[count].quantity = quantity;
            
            // Set price from products data
            cart_items[count].price = 0; // Default if product not found
            for (int i = 0; i < product_count; i++) {
                if (strcmp(products[i].name, name) == 0) {
                    cart_items[count].price = products[i].price;
                    break;
                }
            }
            
            count++;
        }
    }
    
    fclose(fp);
    
    char debug_msg[128];
    snprintf(debug_msg, sizeof(debug_msg), "Loaded %d cart items (consolidated)", count);
    write_debug(debug_msg);
    
    return count;
}

int main() {
    write_debug("Starting get_cart_data.cgi");
    
    // Print HTTP headers for JSON output
    printf("Content-Type: application/json\r\n\r\n");
    
    // Define file paths
    const char *cart_file = "C:\\Apache24\\data\\cart.txt";
    
    // Initialize data structures
    Product products[MAX_PRODUCTS];
    CartItem cart_items[MAX_CART_ITEMS];
    
    // Load products data from all categories
    int product_count = load_all_products(products, MAX_PRODUCTS);
    if (product_count <= 0) {
        write_debug("No products loaded");
        printf("{\"items\":[]}");
        return 1;
    }
    
    // Load cart data with prices
    int cart_count = load_cart(cart_file, cart_items, MAX_CART_ITEMS, products, product_count);
    
    // Generate JSON output for the cart that matches what checkout.html expects
    printf("{\n");
    printf("  \"items\": [\n");
    
    for (int i = 0; i < cart_count; i++) {
        printf("    {\n");
        printf("      \"name\": ");
        json_escape(stdout, cart_items[i].name);
        printf(",\n");
        printf("      \"qty\": %d,\n", cart_items[i].quantity);
        printf("      \"price\": %d\n", cart_items[i].price);
        printf("    }%s\n", (i < cart_count - 1) ? "," : "");
    }
    
    printf("  ]\n");
    printf("}\n");
    
    write_debug("Finished get_cart_data.cgi");
    return 0;
}