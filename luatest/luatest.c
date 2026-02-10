#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sqlite3.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <dirent.h>  // For reading config dir

// Service struct (linked list for simplicity)
typedef struct Service {
    char *name;
    char *exec;
    char **deps;  // Array of dep names
    int dep_count;
    char *restart;  // "always", "on-failure"
    int failures;   // Track faults
    pid_t pid;
    struct Service *next;
} Service;

Service *services = NULL;
sqlite3 *db;

// C function exposed to Lua: register_service("name", "exec", {deps}, "restart")
static int l_register_service(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    const char *exec = luaL_checkstring(L, 2);
    const char *restart = luaL_checkstring(L, 4);

    Service *s = malloc(sizeof(Service));
    s->name = strdup(name);
    s->exec = strdup(exec);
    s->restart = strdup(restart);
    s->pid = -1;
    s->failures = 0;

    // Handle deps table
    luaL_checktype(L, 3, LUA_TTABLE);
    s->dep_count = lua_rawlen(L, 3);
    s->deps = malloc(s->dep_count * sizeof(char*));
    for (int i = 1; i <= s->dep_count; i++) {
        lua_rawgeti(L, 3, i);
        s->deps[i-1] = strdup(lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    // Add to list
    s->next = services;
    services = s;

    return 0;
}

// Load all Lua configs from dir
void load_configs(lua_State *L) {
    DIR *dir = opendir("/etc/myinit/services");
    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (strstr(ent->d_name, ".lua")) {
            char path[256];
            snprintf(path, sizeof(path), "/etc/myinit/services/%s", ent->d_name);
            if (luaL_dofile(L, path) != LUA_OK) {
                fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
            }
        }
    }
    closedir(dir);
}

// Sync services to SQLite
void sync_to_db() {
    sqlite3_open("/var/db/myinit.db", &db);
    // Create tables (services, deps)
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS services (name TEXT PRIMARY KEY, exec TEXT, restart TEXT, failures INT);", NULL, NULL, NULL);
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS deps (service TEXT, dep TEXT);", NULL, NULL, NULL);

    for (Service *s = services; s; s = s->next) {
        char sql[512];
        snprintf(sql, sizeof(sql), "INSERT OR REPLACE INTO services VALUES ('%s', '%s', '%s', %d);", s->name, s->exec, s->restart, s->failures);
        sqlite3_exec(db, sql, NULL, NULL, NULL);
        for (int i = 0; i < s->dep_count; i++) {
            snprintf(sql, sizeof(sql), "INSERT INTO deps VALUES ('%s', '%s');", s->name, s->deps[i]);
            sqlite3_exec(db, sql, NULL, NULL, NULL);
        }
    }
}

// ... (more: topological sort for deps, fork/exec, waitpid loop)

int main(int argc, char **argv)
{
   lua_State *L = luaL_newstate();
   luaL_openlibs(L);
   lua_register(L, "register_service", l_register_service);  // Expose to Lua
   load_configs(L);
   lua_close(L);

   sync_to_db();

   // Resolve and start services (implement topo_sort_start)
   //topo_sort_start(services);

   // Reap loop
   while (1) {
   	int status;
        pid_t pid = waitpid(-1, &status, 0);
        if (pid > 0) {
            // Find service, check exit, restart if needed, update DB failures
        }
   }

   sqlite3_close(db);
   return 0;
}
