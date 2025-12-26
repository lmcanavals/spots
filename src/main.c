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

typedef unsigned char byte;

void spots(const char *input_path, const char *output_path, int size, int gap) {
  int width, height, comp;

  byte *data = stbi_load(input_path, &width, &height, &comp, CHANNELS);
  if (!data) {
    fprintf(stderr, "ERROR: Could not load image %s\n", input_path);
    exit(EXIT_FAILURE);
  }

  byte *out_data = (byte *)calloc((size_t)width * height * CHANNELS, 1);
  size_t *offsets = (size_t *)calloc((size_t)(size * size), sizeof(size_t));
  if (!out_data || !offsets) {
    stbi_image_free(data);
    fprintf(stderr, "ERROR: Memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }

  int step = size + gap;
  int radius = size / 2;
  int sqrt_radius = radius * radius;
  float fix = (float)((size + 1) % 2) / 2.0f;

  size_t npos = 0;
  for (int dy = -radius; dy < size - radius; ++dy) {
    for (int dx = -radius; dx < size - radius; ++dx) {
      float mx = dx + fix;
      float my = dy + fix;
      if (mx * mx + my * my > sqrt_radius) {
        continue;
      }
      int x = (size_t)(dx + radius);
      int y = (size_t)(dy + radius);
      offsets[npos++] = (y * (size_t)width + x) * CHANNELS;
    }
  }

  byte *in_p = data;
  byte *out_p = out_data;

#pragma omp parallel for collapse(2) schedule(static)
  for (int y = gap; y <= height - size; y += step) {
    for (int x = gap; x <= width - size; x += step) {

      // base_idx is the top-left corner of the current "spot"
      size_t base_idx = (y * (size_t)width + x) * CHANNELS;

      int sum_r = 0, sum_g = 0, sum_b = 0;

#pragma clang loop vectorize(enable) interleave(enable)
      for (size_t i = 0; i < npos; ++i) {
        size_t p_idx = base_idx + offsets[i];
        sum_r += in_p[p_idx];
        sum_g += in_p[p_idx + 1];
        sum_b += in_p[p_idx + 2];
      }

      byte avg_r = (byte)(sum_r / npos);
      byte avg_g = (byte)(sum_g / npos);
      byte avg_b = (byte)(sum_b / npos);

#pragma clang loop vectorize(enable)
      for (size_t i = 0; i < npos; ++i) {
        size_t p_idx = base_idx + offsets[i];
        out_p[p_idx] = avg_r;
        out_p[p_idx + 1] = avg_g;
        out_p[p_idx + 2] = avg_b;
        out_p[p_idx + 3] = 255;
      }
    }
  }

  int success = stbi_write_png(output_path, width, height, CHANNELS, out_data,
                               width * CHANNELS);

  free(offsets);
  free(out_data);
  stbi_image_free(data);

  if (!success) {
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
