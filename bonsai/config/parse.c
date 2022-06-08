#include "bonsai/config/parse.h"
#include "bonsai/log.h"

#define len_config_loc 2

static const char* fnames[] = {
    "bonsai",
    "config",
};

static const char* fallback[] = {
    ".config/bonsai",
    ".config/bonsai/config",
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
bsi_config_init(struct bsi_config* config, struct bsi_server* server)
{
    config->server = server;
    wl_list_init(&config->atoms);
    config->found = false;
    memset(config->path, 0, 255);
    return config;
}

void
bsi_config_find(struct bsi_config* config)
{
    char* pconf;
    if ((pconf = getenv("XDG_CONFIG_HOME"))) {
        char pfull[255] = { 0 };

        for (size_t i = 0; i < len_config_loc; ++i) {
            snprintf(pfull, 255, "%s/%s", pconf, fnames[i]);

            if (access(pfull, F_OK) != 0) {
                bsi_error("No config exists in location '%s'", pfull);
                memset(pfull, 0, 255);
                continue;
            }

            config->found = true;
            memcpy(config->path, pfull, 255);
        }
    } else {
        bsi_error("$XDG_CONFIG_HOME not set");

        char* phome;
        char pfull[255] = { 0 };

        if (!(phome = getenv("HOME"))) {
            bsi_error("$HOME not set");
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < len_config_loc; ++i) {
            snprintf(pfull, 255, "%s/%s", phome, fallback[i]);

            if (access(pfull, F_OK) != 0) {
                bsi_error("No config exists in location '%s'", pfull);
                memset(pfull, 0, 255);
                continue;
            }

            config->found = true;
            memcpy(config->path, pfull, 255);
        }
    }
}

void
bsi_config_parse(struct bsi_config* config)
{
    bsi_config_find(config);
    if (!config->found) {
        bsi_error("No config found, using defaults");
        return;
    }

    FILE* f;
    if ((f = fopen(config->path, "r"))) {
        bsi_errno("Failed to open config file '%s'", config->path);
        exit(EXIT_FAILURE);
    }

    size_t len = 0;
    char* line = NULL;
    while (getline(&line, &len, f) != -1) {
        for (size_t i = 0; i < len_keywords; ++i) {
            if (strncmp(keywords[i], line, strlen(keywords[i])) == 0) {
                struct bsi_config_atom* atom =
                    calloc(1, sizeof(struct bsi_config_atom));
                bsi_config_atom_init(atom, i, impls[i], line);
                wl_list_insert(&config->atoms, &atom->link);
                break;
            }
        }
    }

    fclose(f);
}

void
bsi_config_apply(struct bsi_config* config)
{
    if (wl_list_empty(&config->atoms)) {
        bsi_info("Server config is empty, using defaults");
    }

    size_t len_applied = 0;
    struct bsi_config_atom* atom;
    wl_list_for_each(atom, &config->atoms, link)
    {
        atom->impl->apply(atom, config->server);
        ++len_applied;
    }

    bsi_info("Applied %ld config cmds", len_applied);
}

#undef len_config_loc
#undef len_keywords
