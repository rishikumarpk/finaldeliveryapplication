#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define USER_FILE "users.txt"
#define ADDRESS_FILE "address.txt"
#define MAX_EMAIL_LEN 100
#define MAX_PASSWORD_LEN 100
#define MAX_NAME_LEN 100
#define MAX_PHONE_LEN 20
#define MAX_ADDR_LEN 200
#define MAX_PINCODE_LEN 20
#define HASH_TABLE_SIZE 101 // Prime number for better distribution

// Structure for user nodes in linked list (only username and password)
typedef struct UserNode {
    char email[MAX_EMAIL_LEN];
    char password[MAX_PASSWORD_LEN];
    struct UserNode* next;
} UserNode;

// Structure for address information in hash table
typedef struct AddressNode {
    char email[MAX_EMAIL_LEN];  // Email is stored as reference to user
    char name[MAX_NAME_LEN];
    char phone[MAX_PHONE_LEN];  // Used as hash key
    char addr1[MAX_ADDR_LEN];
    char addr2[MAX_ADDR_LEN];
    char pincode[MAX_PINCODE_LEN];
    struct AddressNode* next;  // For collision handling
} AddressNode;

// Global head pointer for the user linked list
UserNode* userListHead = NULL;

// Global hash table for address information
AddressNode* addressHashTable[HASH_TABLE_SIZE] = {NULL};

// Hash function for phone numbers
unsigned int hash_phone(const char* phone) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = *phone++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    
    return hash % HASH_TABLE_SIZE;
}

// Function to url decode the input strings
void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Function to save all addresses in hash table to file
void save_addresses_to_file() {
    FILE* fp = fopen(ADDRESS_FILE, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open address file for writing\n");
        return;
    }
    
    // Iterate through the hash table
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        AddressNode* current = addressHashTable[i];
        while (current != NULL) {
            fprintf(fp, "%s: %s | %s | %s, %s, %s\n", 
                current->email, current->name, current->phone, 
                current->addr1, current->addr2, current->pincode);
            current = current->next;
        }
    }
    
    fclose(fp);
}


// Function to create a new user node (only email and password)
UserNode* create_user_node(const char* email, const char* password) {
    UserNode* newNode = (UserNode*)malloc(sizeof(UserNode));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    strncpy(newNode->email, email, MAX_EMAIL_LEN - 1);
    newNode->email[MAX_EMAIL_LEN - 1] = '\0';
    
    strncpy(newNode->password, password, MAX_PASSWORD_LEN - 1);
    newNode->password[MAX_PASSWORD_LEN - 1] = '\0';
    
    newNode->next = NULL;
    return newNode;
}

// Function to create a new address node
AddressNode* create_address_node(const char* email, const char* name, const char* phone, 
                                const char* addr1, const char* addr2, const char* pincode) {
    AddressNode* newNode = (AddressNode*)malloc(sizeof(AddressNode));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    strncpy(newNode->email, email, MAX_EMAIL_LEN - 1);
    newNode->email[MAX_EMAIL_LEN - 1] = '\0';
    
    strncpy(newNode->name, name, MAX_NAME_LEN - 1);
    newNode->name[MAX_NAME_LEN - 1] = '\0';
    
    strncpy(newNode->phone, phone, MAX_PHONE_LEN - 1);
    newNode->phone[MAX_PHONE_LEN - 1] = '\0';
    
    strncpy(newNode->addr1, addr1, MAX_ADDR_LEN - 1);
    newNode->addr1[MAX_ADDR_LEN - 1] = '\0';
    
    strncpy(newNode->addr2, addr2, MAX_ADDR_LEN - 1);
    newNode->addr2[MAX_ADDR_LEN - 1] = '\0';
    
    strncpy(newNode->pincode, pincode, MAX_PINCODE_LEN - 1);
    newNode->pincode[MAX_PINCODE_LEN - 1] = '\0';
    
    newNode->next = NULL;
    return newNode;
}

// Function to add user to linked list
void add_user_to_list(UserNode* newNode) {
    if (userListHead == NULL) {
        userListHead = newNode;
    } else {
        UserNode* current = userListHead;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

// Function to find user in linked list by email
UserNode* find_user_by_email(const char* email) {
    UserNode* current = userListHead;
    while (current != NULL) {
        if (strcmp(current->email, email) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Function to add address to hash table
void add_address_to_hash_table(AddressNode* newNode) {
    unsigned int hashIndex = hash_phone(newNode->phone);
    
    // Insert at the beginning of the list at the hash index (chaining for collision)
    if (addressHashTable[hashIndex] == NULL) {
        addressHashTable[hashIndex] = newNode;
    } else {
        newNode->next = addressHashTable[hashIndex];
        addressHashTable[hashIndex] = newNode;
    }
}

// Function to find address by phone number
AddressNode* find_address_by_phone(const char* phone) {
    unsigned int hashIndex = hash_phone(phone);
    AddressNode* current = addressHashTable[hashIndex];
    
    while (current != NULL) {
        if (strcmp(current->phone, phone) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Function to find address by email
AddressNode* find_address_by_email(const char* email) {
    // We need to search through all hash table entries since we're searching by email
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        AddressNode* current = addressHashTable[i];
        while (current != NULL) {
            if (strcmp(current->email, email) == 0) {
                return current;
            }
            current = current->next;
        }
    }
    
    return NULL;
}

// Function to load users from file into linked list
void load_users_from_file() {
    FILE* fp = fopen(USER_FILE, "r");
    if (!fp) return; // File doesn't exist yet
    
    char email[MAX_EMAIL_LEN], password[MAX_PASSWORD_LEN];
    while (fscanf(fp, "%s %s", email, password) == 2) {
        UserNode* newNode = create_user_node(email, password);
        if (newNode) {
            add_user_to_list(newNode);
        }
    }
    
    fclose(fp);
}

// Function to load addresses from file into hash table
void load_addresses_from_file() {
    FILE* fp = fopen(ADDRESS_FILE, "r");
    if (!fp) return; // File doesn't exist yet
    
    char line[1024];
    char email[MAX_EMAIL_LEN], name[MAX_NAME_LEN], phone[MAX_PHONE_LEN];
    char addr1[MAX_ADDR_LEN], addr2[MAX_ADDR_LEN], pincode[MAX_PINCODE_LEN];
    
    while (fgets(line, sizeof(line), fp)) {
        // Parse line: "email: name | phone | addr1, addr2, pincode"
        char* email_part = strtok(line, ":");
        if (!email_part) continue;
        
        // Clean email
        while (isspace(*email_part)) email_part++;
        strncpy(email, email_part, MAX_EMAIL_LEN - 1);
        email[MAX_EMAIL_LEN - 1] = '\0';
        
        // Get name part
        char* name_part = strtok(NULL, "|");
        if (!name_part) continue;
        while (isspace(*name_part)) name_part++;
        strncpy(name, name_part, MAX_NAME_LEN - 1);
        name[MAX_NAME_LEN - 1] = '\0';
        
        // Get phone part
        char* phone_part = strtok(NULL, "|");
        if (!phone_part) continue;
        while (isspace(*phone_part)) phone_part++;
        strncpy(phone, phone_part, MAX_PHONE_LEN - 1);
        phone[MAX_PHONE_LEN - 1] = '\0';
        
        // Get address parts
        char* addr_part = strtok(NULL, "\n");
        if (!addr_part) continue;
        
        sscanf(addr_part, "%[^,], %[^,], %s", addr1, addr2, pincode);
        
        // Create and add the address node to hash table
        AddressNode* newNode = create_address_node(email, name, phone, addr1, addr2, pincode);
        if (newNode) {
            add_address_to_hash_table(newNode);
        }
    }
    
    fclose(fp);
}

// Function to save users from linked list to file
void save_users_to_file() {
    FILE* fp = fopen(USER_FILE, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open user file for writing\n");
        return;
    }
    
    UserNode* current = userListHead;
    while (current != NULL) {
        fprintf(fp, "%s %s\n", current->email, current->password);
        current = current->next;
    }
    
    fclose(fp);
}

// Function to save address information for a user
void save_address_for_user(const AddressNode* address) {
    FILE* fp = fopen(ADDRESS_FILE, "a");
    if (!fp) {
        fprintf(stderr, "Failed to open address file for writing\n");
        return;
    }
    
    // Store in order: email, name, phone, address details
    fprintf(fp, "%s: %s | %s | %s, %s, %s\n", 
            address->email, address->name, address->phone, 
            address->addr1, address->addr2, address->pincode);
    
    fclose(fp);
}

// Function to add a new user
void signup_user(const char* email, const char* password) {
    // Create new user node
    UserNode* newUser = create_user_node(email, password);
    if (!newUser) {
        return;
    }
    
    // Add to linked list
    add_user_to_list(newUser);
    
    // Save the updated list to file
    save_users_to_file();
}

// Function to check if a user already exists
int user_exists(const char* email) {
    return (find_user_by_email(email) != NULL);
}

// Function to authenticate a user
int authenticate_user(const char* email, const char* password) {
    UserNode* user = find_user_by_email(email);
    if (user && strcmp(user->password, password) == 0) {
        return 1;
    }
    return 0;
}

// Function to update user's address information
void update_user_address(const char* email, const char* name, const char* phone, 
                        const char* addr1, const char* addr2, const char* pincode) {
    // Check if user exists
    if (!user_exists(email)) {
        return;
    }
    
    // Check if address already exists for this email
    AddressNode* existingAddress = find_address_by_email(email);
    
    if (existingAddress) {
        // Update existing address information
        strncpy(existingAddress->name, name, MAX_NAME_LEN - 1);
        existingAddress->name[MAX_NAME_LEN - 1] = '\0';
        
        // If phone number changed, we need to rehash
        if (strcmp(existingAddress->phone, phone) != 0) {
            // Remove from current hash location
            unsigned int oldHashIndex = hash_phone(existingAddress->phone);
            if (addressHashTable[oldHashIndex] == existingAddress) {
                addressHashTable[oldHashIndex] = existingAddress->next;
            } else {
                AddressNode* prev = addressHashTable[oldHashIndex];
                while (prev->next != existingAddress) {
                    prev = prev->next;
                }
                prev->next = existingAddress->next;
            }
            
            // Update phone and rehash
            strncpy(existingAddress->phone, phone, MAX_PHONE_LEN - 1);
            existingAddress->phone[MAX_PHONE_LEN - 1] = '\0';
            
            existingAddress->next = NULL;
            add_address_to_hash_table(existingAddress);
        }
        
        // Update other fields
        strncpy(existingAddress->addr1, addr1, MAX_ADDR_LEN - 1);
        existingAddress->addr1[MAX_ADDR_LEN - 1] = '\0';
        
        strncpy(existingAddress->addr2, addr2, MAX_ADDR_LEN - 1);
        existingAddress->addr2[MAX_ADDR_LEN - 1] = '\0';
        
        strncpy(existingAddress->pincode, pincode, MAX_PINCODE_LEN - 1);
        existingAddress->pincode[MAX_PINCODE_LEN - 1] = '\0';
        
        // Rewrite entire address file with updated information
        save_addresses_to_file();
    } else {
        // Create new address node
        AddressNode* newAddress = create_address_node(email, name, phone, addr1, addr2, pincode);
        if (newAddress) {
            // Add to hash table
            add_address_to_hash_table(newAddress);
            
            // Save to file
            save_address_for_user(newAddress);
        }
    }
}


// Function to free all memory used by the user linked list
void free_user_list() {
    UserNode* current = userListHead;
    UserNode* next;
    
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    
    userListHead = NULL;
}

// Function to free all memory used by the address hash table
void free_address_hash_table() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        AddressNode* current = addressHashTable[i];
        AddressNode* next;
        
        while (current != NULL) {
            next = current->next;
            free(current);
            current = next;
        }
        
        addressHashTable[i] = NULL;
    }
}

// Function to parse post data for signup and signin
void parse_post_data(char* email, char* password, char* pincode) {
    char buffer[1024];
    int content_length = atoi(getenv("CONTENT_LENGTH") ? getenv("CONTENT_LENGTH") : "0");
    
    if (content_length > 0) {
        fgets(buffer, content_length + 1, stdin);
    } else {
        fgets(buffer, sizeof(buffer), stdin);
    }
    
    char raw_email[MAX_EMAIL_LEN] = "", raw_password[MAX_PASSWORD_LEN] = "", raw_pincode[MAX_PINCODE_LEN] = "";
    sscanf(buffer, "email=%99[^&]&password=%99[^&]&pincode=%19s", raw_email, raw_password, raw_pincode);
    
    url_decode(email, raw_email);
    url_decode(password, raw_password);
    url_decode(pincode, raw_pincode);
}

// Function to parse address form data with name and phone
void parse_address_data(char* email, char* name, char* phone, char* addr1, char* addr2, char* pincode) {
    char buffer[2048];
    int content_length = atoi(getenv("CONTENT_LENGTH") ? getenv("CONTENT_LENGTH") : "0");
    
    if (content_length > 0) {
        fgets(buffer, content_length + 1, stdin);
    } else {
        fgets(buffer, sizeof(buffer), stdin);
    }
    
    // More robust parsing approach
    char *token, *saveptr;
    char temp_buffer[2048];
    strcpy(temp_buffer, buffer);
    
    // Initialize all strings to empty
    email[0] = name[0] = phone[0] = addr1[0] = addr2[0] = pincode[0] = '\0';
    
    // Parse each parameter
    token = strtok_r(temp_buffer, "&", &saveptr);
    while (token != NULL) {
        char raw_value[500] = "";
        
        if (strncmp(token, "email=", 6) == 0) {
            sscanf(token, "email=%499[^\n]", raw_value);
            url_decode(email, raw_value);
        }
        else if (strncmp(token, "name=", 5) == 0) {
            sscanf(token, "name=%499[^\n]", raw_value);
            url_decode(name, raw_value);
        }
        else if (strncmp(token, "phone=", 6) == 0) {
            sscanf(token, "phone=%499[^\n]", raw_value);
            url_decode(phone, raw_value);
        }
        else if (strncmp(token, "addr1=", 6) == 0) {
            sscanf(token, "addr1=%499[^\n]", raw_value);
            url_decode(addr1, raw_value);
        }
        else if (strncmp(token, "addr2=", 6) == 0) {
            sscanf(token, "addr2=%499[^\n]", raw_value);
            url_decode(addr2, raw_value);
        }
        else if (strncmp(token, "pincode=", 8) == 0) {
            sscanf(token, "pincode=%499[^\n]", raw_value);
            url_decode(pincode, raw_value);
        }
        
        token = strtok_r(NULL, "&", &saveptr);
    }
}

int main(void) {
    // Load existing users into the linked list
    load_users_from_file();
    
    // Load existing addresses into the hash table
    load_addresses_from_file();
    
    char* query = getenv("QUERY_STRING");
    if (!query) {
        printf("Content-Type: text/html\n\n");
        printf("<h1>Error: No query string provided.</h1>");
        free_user_list();
        free_address_hash_table();
        return 1;
    }

    char action[20] = "";
    sscanf(query, "action=%19s", action);

    if (strcmp(action, "signup") == 0) {
        char email[MAX_EMAIL_LEN], password[MAX_PASSWORD_LEN], pincode[MAX_PINCODE_LEN];
        parse_post_data(email, password, pincode);

        const char* allowed[] = {"600097","600090","600094","600095","600098"};
        int allowed_flag = 0;
        for (int i = 0; i < 5; ++i) {
            if (strcmp(pincode, allowed[i]) == 0) {
                allowed_flag = 1;
                break;
            }
        }
        if (!allowed_flag) {
            printf("Content-Type: text/plain\n\n");
            printf("invalid_pincode");
        } else if (user_exists(email)) {
            printf("Content-Type: text/plain\n\n");
            printf("user_exists");
        } else {
            signup_user(email, password);
            printf("Status: 303 See Other\r\n");
            printf("Location: /address_form.html?email=%s\r\n\r\n", email);
        }

    } else if (strcmp(action, "address") == 0) {
        char email[MAX_EMAIL_LEN], name[MAX_NAME_LEN], phone[MAX_PHONE_LEN];
        char addr1[MAX_ADDR_LEN], addr2[MAX_ADDR_LEN], pincode[MAX_PINCODE_LEN];
        
        parse_address_data(email, name, phone, addr1, addr2, pincode);
        
        if (user_exists(email)) {
            update_user_address(email, name, phone, addr1, addr2, pincode);
            printf("Status: 303 See Other\r\n");
            printf("Location: /shop.html.html\r\n\r\n");
        } else {
            printf("Content-Type: text/html\n\n");
            printf("<h3>Error: User not found.</h3>");
        }

    } else if (strcmp(action, "signin") == 0) {
        char email[MAX_EMAIL_LEN], password[MAX_PASSWORD_LEN], dummy[MAX_PINCODE_LEN];
        parse_post_data(email, password, dummy);

        printf("Content-Type: text/plain\n\n");
        if (authenticate_user(email, password)) {
            printf("success");
        } else {
            printf("fail");
        }
    } else {
        printf("Content-Type: text/html\n\n");
        printf("<h1>Invalid action requested.</h1>");
    }
    
    // Free memory used by the linked list
    free_user_list();
    
    // Free memory used by the hash table
    free_address_hash_table();
    
    return 0;
}