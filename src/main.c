#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CHANNELS 4
#define DEFAULT_SIZE 5
#define DEFAULT_GAP 1

typedef struct {
  size_t x;
  size_t y;
} Pos;

void spots(const char *input_path, const char *output_path, int size, int gap) {
  int width, height, comp;

  unsigned char *data = stbi_load(input_path, &width, &height, &comp, CHANNELS);
  if (!data) {
    fprintf(stderr, "ERROR: Could not load image %s\n", input_path);
    exit(EXIT_FAILURE);
  }

  unsigned char *out_data =
      (unsigned char *)calloc((size_t)width * height * CHANNELS, 1);
  Pos *positions = (Pos *)calloc((size_t)(size * size), sizeof(Pos));
  if (!positions || !positions) {
    stbi_image_free(data);
    fprintf(stderr, "ERROR: Memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }

  int step = size + gap;
  int radius = size / 2;
  int sqrt_radius = radius * radius;
  float fix = (float)((size + 1) % 2) / 2.0f;

  // --- PHASE 1: PRE-CALCULATE MASK ---
  size_t npos = 0;
  for (int dy = -radius; dy < size - radius; ++dy) {
    for (int dx = -radius; dx < size - radius; ++dx) {
      float mx = dx + fix;
      float my = dy + fix;
      if (mx * mx + my * my > sqrt_radius) {
        continue;
      }
      positions[npos].x = (size_t)(dx + radius);
      positions[npos++].y = (size_t)(dy + radius);
    }
  }

  // --- PHASE 2: PARALLEL PROCESSING ---
  // Change to schedule(static) and restrict pointers
  unsigned char *restrict in_p = data;
  unsigned char *restrict out_p = out_data;

#pragma clang loop vectorize(enable)
  for (int y = gap; y < height; y += step) {
    for (int x = gap; x < width; x += step) {

      int sum_r = 0, sum_g = 0, sum_b = 0;
      size_t base_width = (size_t)width;

      // COMBINED LOOP: Reading data
      for (size_t i = 0; i < npos; ++i) {
        size_t offset =
            ((positions[i].y + y) * base_width + (positions[i].x + x)) *
            CHANNELS;
        sum_r += in_p[offset];
        sum_g += in_p[offset + 1];
        sum_b += in_p[offset + 2];
      }

      unsigned char avg_r = (unsigned char)(sum_r / npos);
      unsigned char avg_g = (unsigned char)(sum_g / npos);
      unsigned char avg_b = (unsigned char)(sum_b / npos);

      // COMBINED LOOP: Writing data
      for (size_t i = 0; i < npos; ++i) {
        size_t offset =
            ((positions[i].y + y) * base_width + (positions[i].x + x)) *
            CHANNELS;
        out_p[offset] = avg_r;
        out_p[offset + 1] = avg_g;
        out_p[offset + 2] = avg_b;
        out_p[offset + 3] = 255;
      }
    }
  }

  int success = stbi_write_png(output_path, width, height, CHANNELS, out_data,
                               width * CHANNELS);

  free(out_data);
  free(positions);
  stbi_image_free(data);

  if (success) {
    // ... rest of your cleanup/save code ...
  } else {
    fprintf(stderr, "ERROR: Could not write PNG file.\n");
  }
}

void print_usage(const char *app_name) {
  fprintf(stderr, "Usage: %s [options] <input_file> <output_file>\n", app_name);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "  -h\t\t- This help information.\n");
  fprintf(stderr, "  -s size\t- Cell size in pixels (default: %d)\n",
          DEFAULT_SIZE);
  fprintf(stderr, "  -g gap\t- Gap in pixels (default: %d)\n", DEFAULT_GAP);
  fprintf(stderr, "\nExample: %s -s 8 -g 2 foto.jpg foto_spots.png\n",
          app_name);
  exit(EXIT_SUCCESS);
}

void process_args(int argc, const char *argv[], int *size, int *gap,
                  const char **input_path, const char **output_path) {
  while (--argc) {
    ++argv;
    if (argv[0][0] == '-' && argv[0][1] == 'h') {
      print_usage(argv[0]);
    } else if (argv[0][0] == '-' && argv[0][1] == 's') {
      *size = atoi((++argv)[0]);
      --argc;
    } else if (argv[0][0] == '-' && argv[0][1] == 'g') {
      *gap = atoi((++argv)[0]);
      --argc;
    } else if (!*input_path) {
      *input_path = argv[0];
    } else {
      *output_path = argv[0];
    }
  }
  if (!*input_path || !*output_path || *size < 1 || *gap < 0) {
    fprintf(stderr, "Wrong options...\n");
    print_usage(argv[0]);
  }
}

int main(int argc, const char *argv[]) {
  int size = DEFAULT_SIZE;
  int gap = DEFAULT_GAP;
  const char *input_path = nullptr;
  const char *output_path = nullptr;
  process_args(argc, argv, &size, &gap, &input_path, &output_path);
  spots(input_path, output_path, size, gap);

  return EXIT_SUCCESS;
}
