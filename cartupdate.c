#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void url_decode(char *s) {
    char *o = s;
    for (char *p = s; *p; ++p) {
        if (*p == '+') *o = ' ';
        else if (*p == '%' && p[1] && p[2]) {
            int h;
            sscanf(p + 1, "%2x", &h);
            *o = h;
            p += 2;
        } else *o = *p;
        o++;
    }
    *o = 0;
}

char *get_query_param(const char *name) {
    char *qs = getenv("QUERY_STRING");
    if (!qs) return NULL;
    size_t len = strlen(name);
    for (char *p = qs; *p;) {
        if (strncmp(p, name, len) == 0 && p[len] == '=') {
            char *val = strdup(p + len + 1);
            char *amp = strchr(val, '&');
            if (amp) *amp = 0;
            url_decode(val);
            return val;
        }
        p = strchr(p, '&');
        if (!p) break;
        ++p;
    }
    return NULL;
}

typedef struct Node //linked list to store cart item name and quantity,reads data from cart.txt
{
    char name[64];
    int qty;
    struct Node *next;
} Node;

// Find item in linked list
Node* find_item(Node *head, const char *item_name) //function to check if an item is already present in the cart,if yes it returns the pointer to where it is called in the main function
{
    Node *current = head;
    while (current) {
        if (strcmp(current->name, item_name) == 0) 
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int main(void) {
    printf("Content-type: text/plain\r\n\r\n");

    // Get query parameters
    char *item = get_query_param("item");
    char *action = get_query_param("action");

    if (!item || !action) {
        printf("ERROR: missing parameters\n");
        return 1;
    }

    const char *cart_file = "C:\\Apache24\\data\\cart.txt";

    // Read the cart file into a linked list
    FILE *in = fopen(cart_file, "r");
    Node *head = NULL;
    if (in) {
        char line[128];
        while (fgets(line, sizeof(line), in)) {
            line[strcspn(line, "\r\n")] = 0;
            char name[64];
            int qty;
            if (sscanf(line, "%63[^,],%d", name, &qty) == 2) {
                Node *existing = find_item(head, name);//this is where the prvs function is called
                
                if (existing) {
                    // Update quantity of existing item
                    existing->qty += qty;
                } else {
                    // Add new item
                    Node *node = malloc(sizeof(Node));
                    strcpy(node->name, name);
                    node->qty = qty;
                    node->next = head;
                    head = node;
                }
            }
        }
        fclose(in);
    }

    // Handle the action on the specified item
    Node *target = find_item(head, item);
    
    if (target) {
        // Item exists in cart
        if (strcmp(action, "decrement") == 0) {
            target->qty--;
        } else if (strcmp(action, "increment") == 0) {
            target->qty++;
        } else if (strcmp(action, "remove") == 0) {
            target->qty = 0; // Mark for removal
        }
    } else if (strcmp(action, "increment") == 0) {
        // Adding new item to cart
        Node *node = malloc(sizeof(Node));
        strcpy(node->name, item);
        node->qty = 1;
        node->next = head;
        head = node;
    }

    // Write the updated cart back to cart.txt, removing items with qty <= 0
    FILE *out = fopen(cart_file, "w");
    if (out) {
        Node *prev = NULL;
        Node *current = head;
        
        while (current) {
            if (current->qty <= 0) {
                // Remove this node
                Node *to_free = current;
                
                if (prev) {
                    prev->next = current->next;
                    current = current->next;
                } else {
                    head = current->next;
                    current = head;
                }
                
                free(to_free);
            } else {
                // Write this node to file
                fprintf(out, "%s,%d\n", current->name, current->qty);
                prev = current;
                current = current->next;
            }
        }
        
        fclose(out);
    }

    // Clean up the linked list
    while (head) {
        Node *tmp = head->next;
        free(head);
        head = tmp;
    }

    free(item);
    free(action);

    printf("OK");
    return 0;
}