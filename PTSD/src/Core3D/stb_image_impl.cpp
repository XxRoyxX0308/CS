// Single compilation unit for stb_image implementation.
// This ensures STB_IMAGE_IMPLEMENTATION is only defined once across the project.
// Use explicit path to avoid picking up SDL2_image's bundled stb_image.h.

#define STB_IMAGE_IMPLEMENTATION
#include "../../lib/stb/stb_image.h"
