/* backend.c
   Full backend implementation for Universal Reservation System
   Features:
   - Customer lists (confirmed + waitlist)
   - Hash table for fast lookup
   - Undo (stack) for last booking
   - Route graph with Dijkstra shortest path
   - Route validation and cost calculation (PRICE_PER_UNIT)
   - File persistence (confirmed.csv, waitlist.csv, meta.txt)
   - Exposes backend_get_shortest_path_text()
   - Made by Piyush Gairola,Ajeet Singh Panwar,Ashish Kunal
*/

#include "backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#define MAX_STACK 100
#define HASH_SIZE 101
#define CONFIRMED_FILE "confirmed.csv"
#define WAITLIST_FILE "waitlist.csv"
#define META_FILE "meta.txt"

#define PRICE_PER_UNIT 100 /* price multiplier per graph weight unit */

struct customer {
    int reservation_id;
    char name[50];
    int age;
    char contact[15];
    int slot_number;
    int route_from;
    int route_to;
    int cost; /* distance * PRICE_PER_UNIT */
    struct customer *next;
};

struct HashNode {
    int reservation_id;
    struct customer *custPtr;
    struct HashNode *next;
};

/* Globals */
static struct customer *confirmed_list = NULL;
static struct customer *waitlist = NULL;
static struct HashNode *hashTable[HASH_SIZE];

static int total_slots = 5;
static int booked_slots = 0;
static int next_reservation_id = 1000;

static struct customer undo_stack[MAX_STACK];
static int top = -1;

/* Graph */
struct Edge {
    int to;
    int weight;
    struct Edge *next;
};

struct Graph {
    int n;
    struct Edge **adj;
};

static struct Graph *route_graph = NULL;

/* City names array (demo) */
static const char *CityName[] = {
    "Delhi", "Mumbai", "Chennai", "Kolkata", "Goa", "Bangalore"
};
static const int CITY_COUNT = 6;

/* ----------------- UNDO ----------------- */
static void push_undo_local(struct customer c) {
    if (top >= MAX_STACK - 1) {
        /* ignore if full */
    } else {
        undo_stack[++top] = c;
    }
}

void backend_undo() {
    if (top < 0) return;
    struct customer last = undo_stack[top--];
    int id = last.reservation_id;
    backend_cancel(id);
}

/* ----------------- HASH ----------------- */
static int hashFunction(int reservation_id) {
    if (reservation_id < 0) reservation_id = -reservation_id;
    return reservation_id % HASH_SIZE;
}

static void init_hash_table() {
    for (int i = 0; i < HASH_SIZE; i++) hashTable[i] = NULL;
}

static void insertRecord(struct customer *p) {
    if (!p) return;
    int idx = hashFunction(p->reservation_id);
    struct HashNode *node = (struct HashNode*)malloc(sizeof(struct HashNode));
    if (!node) return;
    node->reservation_id = p->reservation_id;
    node->custPtr = p;
    node->next = hashTable[idx];
    hashTable[idx] = node;
}

static struct customer* searchRecord(int reservation_id) {
    int idx = hashFunction(reservation_id);
    struct HashNode *h = hashTable[idx];
    while (h) {
        if (h->reservation_id == reservation_id) return h->custPtr;
        h = h->next;
    }
    return NULL;
}

static void deleteRecord(int reservation_id) {
    int idx = hashFunction(reservation_id);
    struct HashNode *h = hashTable[idx];
    struct HashNode *prev = NULL;
    while (h && h->reservation_id != reservation_id) {
        prev = h;
        h = h->next;
    }
    if (!h) return;
    if (!prev) hashTable[idx] = h->next;
    else prev->next = h->next;
    free(h);
}

/* ----------------- PASSENGER LIST ----------------- */
static void insert_customer_local(int reservation_id, const char name[], int age, const char contact[], int slot_number, int cost) {
    struct customer *newc = (struct customer*)malloc(sizeof(struct customer));
    if (!newc) return;
    newc->reservation_id = reservation_id;
    strncpy(newc->name, name, sizeof(newc->name)-1); newc->name[sizeof(newc->name)-1]='\0';
    newc->age = age;
    strncpy(newc->contact, contact, sizeof(newc->contact)-1); newc->contact[sizeof(newc->contact)-1]='\0';
    newc->slot_number = slot_number;
    newc->route_from = -1;
    newc->route_to = -1;
    newc->cost = cost;
    newc->next = NULL;

    if (confirmed_list == NULL) confirmed_list = newc;
    else {
        struct customer *t = confirmed_list;
        while (t->next) t = t->next;
        t->next = newc;
    }
    insertRecord(newc);
}

static void delete_customer_local(int reservation_id) {
    struct customer *temp = confirmed_list, *prev = NULL;
    if (!temp) return;
    if (temp->reservation_id == reservation_id) {
        deleteRecord(temp->reservation_id);
        confirmed_list = temp->next;
        free(temp);
        booked_slots--;
        return;
    }
    while (temp && temp->reservation_id != reservation_id) {
        prev = temp;
        temp = temp->next;
    }
    if (!temp) return;
    deleteRecord(temp->reservation_id);
    prev->next = temp->next;
    free(temp);
    booked_slots--;
}

/* ----------------- WAITLIST ----------------- */
static void enqueue_waitlist_local(int reservation_id, const char name[], int age, const char contact[], int route_from, int route_to, int cost) {
    struct customer *newr = (struct customer*)malloc(sizeof(struct customer));
    if (!newr) return;
    newr->reservation_id = reservation_id;
    strncpy(newr->name, name, sizeof(newr->name)-1); newr->name[sizeof(newr->name)-1]='\0';
    newr->age = age;
    strncpy(newr->contact, contact, sizeof(newr->contact)-1); newr->contact[sizeof(newr->contact)-1]='\0';
    newr->slot_number = -1;
    newr->route_from = route_from;
    newr->route_to = route_to;
    newr->cost = cost;
    newr->next = NULL;

    if (!waitlist) waitlist = newr;
    else {
        struct customer *t = waitlist;
        while (t->next) t = t->next;
        t->next = newr;
    }
}

static void dequeue_waitlist_local() {
    if (!waitlist) return;
    struct customer *t = waitlist;
    waitlist = waitlist->next;
    free(t);
}

/* ----------------- GRAPH UTILITIES (Dijkstra) ----------------- */
void add_edge_local(struct Graph *g, int u, int v, int w) {
    struct Edge *e = malloc(sizeof(struct Edge));
    if (!e) return;
    e->to = v; e->weight = w; e->next = g->adj[u];
    g->adj[u] = e;
    struct Edge *e2 = malloc(sizeof(struct Edge));
    if (!e2) return;
    e2->to = u; e2->weight = w; e2->next = g->adj[v];
    g->adj[v] = e2;
}

/* Dijkstra: returns 0 on success; out_distance set to distance (>=0).
   If no path exists, out_distance is set to -1 and function returns -1.
   If out_path and out_len provided, path (sequence of node indices) is written into out_path (up to out_path_len),
   and *out_len is set to the path length.
*/
static int dijkstra_shortest_path(struct Graph *g, int src, int dest, int *out_distance, int out_path[], int *out_len, int out_path_len) {
    if (!g) return -1;
    if (src < 0 || src >= g->n || dest < 0 || dest >= g->n) return -1;
    int n = g->n;
    int *dist = malloc(sizeof(int) * n);
    int *prev = malloc(sizeof(int) * n);
    int *visited = malloc(sizeof(int) * n);
    if (!dist || !prev || !visited) {
        free(dist); free(prev); free(visited);
        return -1;
    }
    for (int i = 0; i < n; i++) {
        dist[i] = INT_MAX;
        prev[i] = -1;
        visited[i] = 0;
    }
    dist[src] = 0;

    for (int count = 0; count < n; count++) {
        int u = -1;
        int best = INT_MAX;
        for (int i = 0; i < n; i++) {
            if (!visited[i] && dist[i] < best) {
                best = dist[i];
                u = i;
            }
        }
        if (u == -1) break;
        visited[u] = 1;
        if (u == dest) break;
        struct Edge *e = g->adj[u];
        while (e) {
            int v = e->to;
            if (!visited[v] && dist[u] != INT_MAX && dist[u] + e->weight < dist[v]) {
                dist[v] = dist[u] + e->weight;
                prev[v] = u;
            }
            e = e->next;
        }
    }

    if (dist[dest] == INT_MAX) {
        free(dist); free(prev); free(visited);
        if (out_distance) *out_distance = -1;
        if (out_len) *out_len = 0;
        return -1;
    }

    if (out_distance) *out_distance = dist[dest];

    if (out_path && out_len) {
        int tmp[512];
        int idx = 0;
        int cur = dest;
        while (cur != -1 && idx < 511) {
            tmp[idx++] = cur;
            cur = prev[cur];
        }
        /* reverse into out_path */
        int plen = (idx < out_path_len) ? idx : out_path_len;
        for (int i = 0; i < plen; i++) {
            out_path[i] = tmp[idx - 1 - i];
        }
        *out_len = plen;
    }

    free(dist); free(prev); free(visited);
    return 0;
}

/* Helper: compute distance and cost if path exists. Returns distance or -1 if no path. */
static int compute_route_distance_and_cost(int from, int to, int *out_cost) {
    if (!route_graph) return -1;
    if (from < 0 || from >= route_graph->n || to < 0 || to >= route_graph->n) return -1;
    int dist = -1;
    int res = dijkstra_shortest_path(route_graph, from, to, &dist, NULL, NULL, 0);
    if (res != 0 || dist < 0) return -1;
    if (out_cost) *out_cost = dist * PRICE_PER_UNIT;
    return dist;
}

/*Piyush  book/cancel/modify/search wrappers for GUI */
int backend_book(const char *name, int age, const char *contact, int route_from, int route_to) {
    /* Validate route indices */
    if (route_from < 0 || route_from >= CITY_COUNT || route_to < 0 || route_to >= CITY_COUNT) {
        return -1; /* invalid indices */
    }
    /* Check shortest path exists and compute cost */
    int cost = 0;
    int dist = compute_route_distance_and_cost(route_from, route_to, &cost);
    if (dist < 0) {
        return -1; /* no route exists */
    }

    int reservation_id = next_reservation_id++;
    if (booked_slots < total_slots) {
        insert_customer_local(reservation_id, name, age, contact, ++booked_slots, cost);
        struct customer *node = searchRecord(reservation_id);
        if (node) {
            node->route_from = route_from;
            node->route_to = route_to;
            node->cost = cost;
        }
        struct customer temp;
        temp.reservation_id = reservation_id;
        strncpy(temp.name, name, sizeof(temp.name)-1); temp.name[sizeof(temp.name)-1]='\0';
        temp.age = age;
        strncpy(temp.contact, contact, sizeof(temp.contact)-1); temp.contact[sizeof(temp.contact)-1]='\0';
        temp.slot_number = booked_slots;
        temp.route_from = route_from;
        temp.route_to = route_to;
        temp.cost = cost;
        push_undo_local(temp);
    } else {
        enqueue_waitlist_local(reservation_id, name, age, contact, route_from, route_to, cost);
    }
    return reservation_id;
}

void backend_cancel(int reservation_id) {
    delete_customer_local(reservation_id);
    if (waitlist) {
        struct customer *temp = waitlist;
        insert_customer_local(temp->reservation_id, temp->name, temp->age, temp->contact, ++booked_slots, temp->cost);
        struct customer *node = searchRecord(temp->reservation_id);
        if (node) {
            node->route_from = temp->route_from;
            node->route_to = temp->route_to;
            node->cost = temp->cost;
        }
        dequeue_waitlist_local();
    }
}

void backend_modify(int reservation_id, const char *newname, int newage, const char *newcontact) {
    struct customer *temp = searchRecord(reservation_id);
    if (!temp) {
        temp = waitlist;
        while (temp && temp->reservation_id != reservation_id) temp = temp->next;
    }
    if (!temp) return;
    if (newname && strlen(newname) > 0) strncpy(temp->name, newname, sizeof(temp->name)-1);
    if (newage > 0) temp->age = newage;
    if (newcontact && strlen(newcontact) > 0) strncpy(temp->contact, newcontact, sizeof(temp->contact)-1);
}

int backend_search(int reservation_id) {
    struct customer *found = searchRecord(reservation_id);
    if (found) return 1; /* confirmed */
    struct customer *t = waitlist;
    while (t) {
        if (t->reservation_id == reservation_id) return 2; /* waitlist */
        t = t->next;
    }
    return 0;
}

void backend_assign_route(int id, int from, int to) {
    /* Attempt to compute cost and assign only if path exists */
    if (!route_graph) return;
    if (from < 0 || from >= route_graph->n || to < 0 || to >= route_graph->n) return;
    int cost = 0;
    int dist = compute_route_distance_and_cost(from, to, &cost);
    if (dist < 0) {
        /* invalid route -> do nothing */
        return;
    }
    struct customer *c = searchRecord(id);
    if (c) {
        c->route_from = from; c->route_to = to; c->cost = cost; return;
    }
    struct customer *t = waitlist;
    while (t) {
        if (t->reservation_id == id) { t->route_from = from; t->route_to = to; t->cost = cost; return; }
        t = t->next;
    }
}

/* Helper safe append */
static void append_safe(char *buf, int *pos, int size, const char *fmt, ...) {
    if (*pos >= size-1) return;
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf + *pos, size - *pos, fmt, ap);
    va_end(ap);
    if (r > 0) *pos += r;
    if (*pos > size-1) *pos = size-1;
}

void backend_get_confirmed_text(char *buf, int bufsize) {
    int pos = 0;
    if (!confirmed_list) {
        append_safe(buf, &pos, bufsize, "No confirmed reservations.\n");
        buf[pos]='\0';
        return;
    }
    struct customer *t = confirmed_list;
    while (t) {
        append_safe(buf, &pos, bufsize, "ID:%d | %s | Age:%d | Contact:%s | Slot:%d", t->reservation_id, t->name, t->age, t->contact, t->slot_number);
        if (t->route_from != -1 || t->route_to != -1) {
            const char *from = (t->route_from >=0 && t->route_from < CITY_COUNT) ? CityName[t->route_from] : "N/A";
            const char *to   = (t->route_to   >=0 && t->route_to   < CITY_COUNT) ? CityName[t->route_to]   : "N/A";
            append_safe(buf, &pos, bufsize, " | Route:%s->%s | Cost:₹%d", from, to, t->cost);
        }
        append_safe(buf, &pos, bufsize, "\n");
        t = t->next;
    }
    buf[pos]='\0';
}

void backend_get_waitlist_text(char *buf, int bufsize) {
    int pos = 0;
    if (!waitlist) { append_safe(buf, &pos, bufsize, "Waitlist empty.\n"); buf[pos]='\0'; return; }
    struct customer *t = waitlist;
    while (t) {
        append_safe(buf, &pos, bufsize, "ID:%d | %s | Age:%d | Contact:%s", t->reservation_id, t->name, t->age, t->contact);
        if (t->route_from != -1 || t->route_to != -1) {
            const char *from = (t->route_from >=0 && t->route_from < CITY_COUNT) ? CityName[t->route_from] : "N/A";
            const char *to   = (t->route_to   >=0 && t->route_to   < CITY_COUNT) ? CityName[t->route_to]   : "N/A";
            append_safe(buf, &pos, bufsize, " | Route:%s->%s | Cost:₹%d", from, to, t->cost);
        }
        append_safe(buf, &pos, bufsize, "\n");
        t = t->next;
    }
    buf[pos]='\0';
}

void backend_get_slotmap_text(char *buf, int bufsize) {
    int pos = 0;
    for (int i = 1; i <= total_slots; i++) {
        int found = 0;
        struct customer *t = confirmed_list;
        while (t) {
            if (t->slot_number == i) {
                append_safe(buf, &pos, bufsize, "Slot %d - %s (ID:%d)\n", i, t->name, t->reservation_id);
                found = 1; break;
            }
            t = t->next;
        }
        if (!found) append_safe(buf, &pos, bufsize, "Slot %d - Available\n", i);
    }
    buf[pos]='\0';
}

void backend_get_availability_text(char *buf, int bufsize) {
    int pos = 0;
    append_safe(buf, &pos, bufsize, "Total: %d\nBooked: %d\nAvailable: %d\n", total_slots, booked_slots, total_slots - booked_slots);
    buf[pos]='\0';
}

/* ------------- file persistence ------------- */
void backend_save_all() {
    /* confirmed */
    FILE *f = fopen(CONFIRMED_FILE, "w");
    if (f) {
        struct customer *t = confirmed_list;
        while (t) {
            /* include cost as last field */
            fprintf(f, "%d,%s,%d,%s,%d,%d,%d,%d\n", t->reservation_id, t->name, t->age, t->contact, t->slot_number, t->route_from, t->route_to, t->cost);
            t = t->next;
        }
        fclose(f);
    }
    /* waitlist */
    f = fopen(WAITLIST_FILE, "w");
    if (f) {
        struct customer *t = waitlist;
        while (t) {
            fprintf(f, "%d,%s,%d,%s,%d,%d,%d,%d\n", t->reservation_id, t->name, t->age, t->contact, t->slot_number, t->route_from, t->route_to, t->cost);
            t = t->next;
        }
        fclose(f);
    }
    /* meta */
    f = fopen(META_FILE, "w");
    if (f) {
        fprintf(f, "%d\n%d\n%d\n", next_reservation_id, total_slots, booked_slots);
        fclose(f);
    }
}

/* ------------- load & init ------------- */
void backend_init() {
    init_hash_table();
    /* routes demo graph (CITY_COUNT nodes) */
    route_graph = malloc(sizeof(struct Graph));
    if (!route_graph) return;
    route_graph->n = CITY_COUNT;
    route_graph->adj = malloc(sizeof(struct Edge *) * route_graph->n);
    for (int i=0;i<route_graph->n;i++) route_graph->adj[i]=NULL;

    /* sample weighted edges (undirected) */
    add_edge_local(route_graph,0,1,5);  /* Delhi - Mumbai (5) */
    add_edge_local(route_graph,0,2,8);  /* Delhi - Chennai (8) */
    add_edge_local(route_graph,1,2,3);  /* Mumbai - Chennai (3) */
    add_edge_local(route_graph,1,3,7);  /* Mumbai - Kolkata (7) */
    add_edge_local(route_graph,2,4,6);  /* Chennai - Goa (6) */
    add_edge_local(route_graph,4,5,2);  /* Goa - Bangalore (2) */
    add_edge_local(route_graph,3,5,10); /* Kolkata - Bangalore (10) */

    /* load meta */
    FILE *f = fopen(META_FILE, "r");
    if (f) {
        if (fscanf(f, "%d\n%d\n%d\n", &next_reservation_id, &total_slots, &booked_slots) != 3) {
            next_reservation_id = 1000; total_slots = 5; booked_slots = 0;
        }
        fclose(f);
    }

    /* confirmed */
    f = fopen(CONFIRMED_FILE, "r");
    if (f) {
        int id, age, slot, rf, rt, cost;
        char name[50], contact[15];
        while (fscanf(f, "%d,%49[^,],%d,%14[^,],%d,%d,%d,%d\n", &id, name, &age, contact, &slot, &rf, &rt, &cost) == 8) {
            insert_customer_local(id, name, age, contact, slot, cost);
            struct customer *c = searchRecord(id);
            if (c) { c->route_from = rf; c->route_to = rt; c->cost = cost; }
        }
        fclose(f);
    }

    /* waitlist */
    f = fopen(WAITLIST_FILE, "r");
    if (f) {
        int id, age, slot, rf, rt, cost;
        char name[50], contact[15];
        while (fscanf(f, "%d,%49[^,],%d,%14[^,],%d,%d,%d,%d\n", &id, name, &age, contact, &slot, &rf, &rt, &cost) == 8) {
            enqueue_waitlist_local(id, name, age, contact, rf, rt, cost);
        }
        fclose(f);
    }
}

void backend_change_slots(int n) {
    // Do not reduce below current bookings
    if (n < 1) return;
    if (n < booked_slots) return;

    total_slots = n;
}

/* ----------------- Shortest path text API ----------------- */
/* Writes human-readable path, distance and cost into buf.
   Returns 0 on success, -1 on failure.
*/
int backend_get_shortest_path_text(int from, int to, char *buf, int bufsize) {
    if (!buf || bufsize <= 0) return -1;
    buf[0] = '\0';
    if (!route_graph) {
        snprintf(buf, bufsize, "Route graph not initialized.\n");
        return -1;
    }
    if (from < 0 || from >= route_graph->n || to < 0 || to >= route_graph->n) {
        snprintf(buf, bufsize, "Invalid city indices. Valid: 0..%d\n", route_graph->n - 1);
        return -1;
    }
    int dist = -1;
    int path_nodes[512];
    int path_len = 0;
    int res = dijkstra_shortest_path(route_graph, from, to, &dist, path_nodes, &path_len, 512);
    if (res != 0 || dist < 0 || path_len == 0) {
        snprintf(buf, bufsize, "No route exists between %s and %s.\n",
                 (from>=0 && from < CITY_COUNT) ? CityName[from] : "N/A",
                 (to>=0 && to < CITY_COUNT) ? CityName[to] : "N/A");
        return -1;
    }
    int cost = dist * PRICE_PER_UNIT;
    int pos = 0;
    for (int i = 0; i < path_len; ++i) {
        const char *name = (path_nodes[i] >=0 && path_nodes[i] < CITY_COUNT) ? CityName[path_nodes[i]] : "N/A";
        if (i == 0) pos += snprintf(buf+pos, bufsize - pos, "%s", name);
        else pos += snprintf(buf+pos, bufsize - pos, " -> %s", name);
        if (pos >= bufsize-1) break;
    }
    pos += snprintf(buf+pos, bufsize - pos, "\nDistance: %d\nCost: ₹%d\n", dist, cost);
    if (pos >= bufsize) buf[bufsize-1] = '\0';
    return 0;
}
