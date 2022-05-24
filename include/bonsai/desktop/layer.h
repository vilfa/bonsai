#pragma once

#include <stdlib.h>
#include <wayland-util.h>

struct bsi_layer_background
{};

struct bsi_layer_bottom
{};

struct bsi_layer_top
{};

struct bsi_layer_overlay
{};

struct bsi_layers
{
    // TODO: Wat do?
    struct bsi_layer_background background;
    struct bsi_layer_bottom bottom;
    struct bsi_layer_top top;
    struct bsi_layer_overlay overlay;
};
