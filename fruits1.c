#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- linked‑list struct ---------- */
typedef struct Product 
{
    char  name[64];
    char  image[256];
    int   price;
    int   stock;
    struct Product *next;
} Product;

void save_cart(const char *path, const char *product_name) {
    FILE *fp = fopen(path, "a");  // Open in append mode
    if (!fp) 
    { 
        perror("cart save"); 
        return; 
    }
    fprintf(fp, "%s,1\n", product_name);  // For now, quantity=1
    fclose(fp);
}

/* ---------- append helper ---------- */
void append(Product **head, Product item) {
    Product *node = malloc(sizeof(Product));
    *node = item;
    node->next = NULL;

    if (*head == NULL) 
    { *head = node; return; }
    Product *cur = *head;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

/* ---------- load / save ---------- */
Product *load_products(const char *path) {
    FILE *fp = fopen("C:\\Apache24\\data\\fruits.txt", "r");
    if (!fp) { perror("open"); return NULL; }
    Product *head = NULL;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        Product p = {0};
        if (sscanf(line, "%63[^,],%255[^,],%d,%d", &p.name, &p.image, &p.price, &p.stock) == 4) {
            append(&head, p);
        }
    }
    fclose(fp);
    return head;
}

void save_products(const char *path, Product *list) {
    FILE *fp = fopen("C:\\Apache24\\data\\fruits.txt", "w");
    if (!fp) { perror("save"); return; }
    for (Product *p = list; p; p = p->next)
        fprintf(fp, "%s,%s,%d,%d\n", p->name, p->image, p->price, p->stock);
    fclose(fp);
}

/* ---------- tiny CGI param helper ---------- */
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
    size_t nlen = strlen(name);
    for (char *p = qs; *p;) {
        if (strncmp(p, name, nlen) == 0 && p[nlen] == '=') {
            char *val = strdup(p + nlen + 1);
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

/* ---------- HTML output ---------- */
void print_html(Product *list) {
    puts("Content-type:text/html\r\n\r\n");
    puts("<html><head><title>Fruits</title>"
         "<meta charset='UTF-8'>"
         "<style>"
         "body{font-family:sans-serif;background:#f5f5f5;margin:0;padding:20px;}"
         ".container{display:flex;flex-wrap:wrap;justify-content:center;gap:20px;}"
         ".card{background:white;border-radius:12px;padding:15px;width:240px;"
         "box-shadow:0 6px 12px rgba(0,0,0,0.1);text-align:center;transition:transform 0.2s ease;}"
         ".card:hover{transform:scale(1.03);}"
         ".product-img{width:160px;height:160px;object-fit:cover;border-radius:10px;margin-bottom:10px;}"
         ".back-btn{margin:20px auto;display:block;width:max-content;padding:10px 24px;border:none;"
         "border-radius:8px;background:#2196F3;color:white;text-decoration:none;font-size:16px;}"
         "h2{text-align:center;color:#333;}"
         "h3{margin:10px 0;color:#444;font-size:20px;}"
         "p{margin:6px 0;font-size:15px;color:#666;}"
         "button{margin-top:12px;padding:10px 22px;border:none;border-radius:6px;background:#4caf50;"
         "color:white;font-size:15px;cursor:pointer;transition:background 0.3s;}"
         "button:hover:not(:disabled){background:#43a047;}"
         "button:disabled{background:#ccc;cursor:not-allowed;}"
         "form{margin-top:8px;}"
         "</style>"
         "</head><body><h2>🍉 Fruits Store</h2>");

    puts("<a href='http://localhost/shop.html.html' class='back-btn'>&larr; Back to Categories</a>");
    puts("<div class='container'>");

    for (Product *p = list; p; p = p->next) {
        printf("<div class='card'>");

        printf("<h3>%s</h3>", p->name);
        printf("<img src='/images/%s' alt='%s' class='product-img'><br>", p->image, p->name);
        printf("<p><strong>Price:</strong> &#8377;%d</p>", p->price);
        printf("<p><strong>Stock:</strong> %d</p>", p->stock);

        if (p->stock > 0)
            printf("<form method='GET' action='/cgi-bin/fruits1.cgi'>"
                   "<input type='hidden' name='buy' value='%s'>"
                   "<button type='submit'>🛒 Add to cart</button></form>", p->name);
        else
            printf("<button disabled>Sold Out</button>");

        puts("</div>");
    }

    puts("</div></body></html>");
}

/* ---------- main ---------- */
int main(void) {
    const char *file = "C:\\Apache24\\data\\fruits.txt";

    Product *list = load_products(file);
    if (!list) {
        puts("Content-type:text/plain\r\n\r\nCannot load.");
        return 1;
    }

    char *want = get_query_param("buy");
    if (want) {
        for (Product *p = list; p; p = p->next) {
            if (strcmp(p->name, want) == 0 && p->stock > 0) {
                p->stock--;
                save_cart("C:\\Apache24\\data\\cart.txt", p->name);
                break;
            }
        }
        save_products(file, list);
        free(want);
    }

    print_html(list);

    Product *cur = list, *tmp;
    while (cur) { tmp = cur->next; free(cur); cur = tmp; }
    return 0;
}
