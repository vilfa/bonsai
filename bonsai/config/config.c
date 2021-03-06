#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <wayland-util.h>

#include "bonsai/config/atom.h"
#include "bonsai/config/config.h"
#include "bonsai/log.h"
#include "bonsai/server.h"

#define len_fnames 2
#define len_config_loc 4

static const char* fnames[] = {
    "bonsai",
    "config",
};

static const char* fallback[] = {
    BSI_USERCONFDIR "/bonsai",
    BSI_USERCONFDIR "/bonsai/config",
    BSI_PREFIX "/" BSI_SYSCONFDIR "/bonsai",
    BSI_PREFIX "/" BSI_SYSCONFDIR "/bonsai/config",
};

#define len_keywords 4

static const char* keywords[] = {
    [BSI_CONFIG_ATOM_OUTPUT] = "output",
    [BSI_CONFIG_ATOM_INPUT] = "input",
    [BSI_CONFIG_ATOM_WORKSPACE] = "workspace",
    [BSI_CONFIG_ATOM_WALLPAPER] = "wallpaper",
};

static const struct bsi_config_atom_impl* impls[] = {
    [BSI_CONFIG_ATOM_OUTPUT] = &output_impl,
    [BSI_CONFIG_ATOM_INPUT] = &input_impl,
    [BSI_CONFIG_ATOM_WORKSPACE] = &workspace_impl,
    [BSI_CONFIG_ATOM_WALLPAPER] = &wallpaper_impl,
};

struct bsi_config*
config_init(struct bsi_config* config, struct bsi_server* server)
{
    config->server = server;
    wl_list_init(&config->atoms);
    config->found = false;
    memset(config->path, 0, 255);
    return config;
}

void
config_destroy(struct bsi_config* config)
{
    struct bsi_config_atom *atom, *atom_tmp;
    wl_list_for_each_safe(atom, atom_tmp, &config->atoms, link)
    {
        config_atom_destroy(atom);
    }
    struct bsi_input_config *conf, *conf_tmp;
    wl_list_for_each_safe(conf, conf_tmp, &config->server->config.input, link)
    {
        config_input_destroy(conf);
    }
}

void
config_find(struct bsi_config* config)
{
    char* pconf;
    if ((pconf = getenv("XDG_CONFIG_HOME"))) {
        char pfull[255] = { 0 };

        for (size_t i = 0; i < len_fnames; ++i) {
            snprintf(pfull, 255, "%s/%s", pconf, fnames[i]);

            struct stat st;
            if (stat(pfull, &st) != 0) {
                info("No config exists in location '%s'", pfull);
                memset(pfull, 0, 255);
                continue;
            }
            if (access(pfull, F_OK | R_OK) != 0 || !S_ISREG(st.st_mode)) {
                info("No config exists in location '%s'", pfull);
                memset(pfull, 0, 255);
                continue;
            }

            config->found = true;
            memcpy(config->path, pfull, 255);
        }
    } else {
        info("XDG_CONFIG_HOME not set");

        char* phome;
        char pfull[255] = { 0 };

        if (!(phome = getenv("HOME"))) {
            error("HOME not set");
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < len_config_loc; ++i) {
            if (fallback[i][0] != '/')
                snprintf(pfull, 255, "%s/%s", phome, fallback[i]);
            else
                snprintf(pfull, 255, "%s", fallback[i]);

            struct stat st;
            if (stat(pfull, &st) != 0) {
                info("No config exists in location '%s'", pfull);
                memset(pfull, 0, 255);
                continue;
            }
            if (access(pfull, F_OK | R_OK) != 0 || !S_ISREG(st.st_mode)) {
                info("No config exists in location '%s'", pfull);
                memset(pfull, 0, 255);
                continue;
            }

            config->found = true;
            memcpy(config->path, pfull, 255);
            break;
        }
    }
}

void
config_parse(struct bsi_config* config)
{
    config_find(config);
    if (!config->found) {
        info("No config found, using defaults");
        return;
    }

    info("Found config '%s'", config->path);

    FILE* f;
    if (!(f = fopen(config->path, "r"))) {
        errn("Failed to open config '%s'", config->path);
        exit(EXIT_FAILURE);
    }

    size_t len = 0;
    char* line = NULL;
    while (getline(&line, &len, f) != -1) {
        if (strncmp(line, "#", 1) == 0) {
            /* Skip comments. */
            continue;
        }
        for (size_t i = 0; i < len_keywords; ++i) {
            if (strncmp(keywords[i], line, strlen(keywords[i])) == 0) {
                line[strcspn(line, "\r\n")] = '\0';
                line[len] = '\0';
                struct bsi_config_atom* atom =
                    calloc(1, sizeof(struct bsi_config_atom));
                config_atom_init(atom, i, impls[i], line);
                wl_list_insert(&config->atoms, &atom->link);
                free(line);
                line = NULL;
                break;
            }
        }
    }

    fclose(f);
}

void
config_apply(struct bsi_config* config)
{
    if (wl_list_empty(&config->atoms)) {
        info("Server config is empty, using defaults");
    }

    size_t len_applied = 0;
    struct bsi_config_atom* atom;
    wl_list_for_each(atom, &config->atoms, link)
    {
        switch (atom->type) {
            case BSI_CONFIG_ATOM_WALLPAPER:
            case BSI_CONFIG_ATOM_WORKSPACE:
            case BSI_CONFIG_ATOM_INPUT:
                atom->impl->apply(atom, config->server);
                ++len_applied;
                break;
            case BSI_CONFIG_ATOM_OUTPUT:
                ++len_applied;
                break;
        }
    }

    debug("Applied %ld config commands", len_applied);
}

#undef len_config_loc
#undef len_keywords
