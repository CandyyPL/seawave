#define SW_IMP_
#include "seawave.h"

void generate(int num_channels, int sample_rate, int bit_depth, int duration) {
  if (num_channels == 0) num_channels = 1;
  if (sample_rate == 0) sample_rate = 44100;
  if (bit_depth == 0) bit_depth = 16;
  if (duration == 0) duration = 3;
  char *output = "output.wav";

  wav_metadata_t wav_meta = {
    .num_channels = num_channels,
    .sample_rate = sample_rate,
    .bit_depth = bit_depth,
    .duration = duration
  };

  int total_samples = get_total_samples(wav_meta, wav_meta.sample_rate);
  int16_t *buffer = malloc(sizeof(int16_t) * total_samples);

  if (buffer == NULL) {
    printf("error: could not allocate memory\n");
    exit(1);
  }

  wav_header_t wav_header = create_wav_header(1, wav_meta);
  generate_sine_wave(wav_meta, buffer, BASE_FREQ);
  create_wav_file(output, wav_meta, wav_header, buffer);
  free(buffer);

  exit(0);
}

void resample(char *filename, int new_sample_rate) {
  wav_metadata_t wav_meta;

  read_wav_meta(filename, &wav_meta);

  int total_samples = get_total_samples(wav_meta, wav_meta.sample_rate);
  int16_t *wav_data = malloc(sizeof(int16_t) * total_samples);

  if (wav_data == NULL) {
    printf("error: could not allocate memory\n");
    exit(1);
  }

  read_wav_data(filename, total_samples, wav_data);

  int new_total_samples = get_total_samples(wav_meta, new_sample_rate);
  int16_t *new_wav_data = malloc(sizeof(int16_t) * new_total_samples);

  if (new_wav_data == NULL) {
    printf("error: could not allocate memory\n");
    exit(1);
  }

  nn_resample(wav_meta, new_sample_rate, wav_data, new_wav_data);

  wav_meta.sample_rate = new_sample_rate;
  wav_header_t wav_header = create_wav_header(1, wav_meta);

  char new_filename[64];
  get_resampled_filename(filename, new_sample_rate, new_filename);

  create_wav_file(new_filename, wav_meta, wav_header, wav_data);
  free(new_wav_data);

  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("usage: ./seawave -m <mode> -f <input.wav> <mode_options>\n");
    exit(0);
  }

  int mode = 0;

  int num_channels = 0;
  int sample_rate = 0;
  int new_sample_rate = 0;
  int bit_depth = 0;
  int duration = 0;

  char *in_file = "test.wav";

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-m") == 0 && (i + 1) < argc) {
      if (strcmp(argv[i + 1], "generate") == 0) {
        mode = 0;
      }
      else if (strcmp(argv[i + 1], "resample") == 0) {
        mode = 1;
      }
    }

    if (strcmp(argv[i], "-f") == 0 && (i + 1) < argc) {
      in_file = argv[i + 1];
    }

    if (strcmp(argv[i], "-r") == 0 && (i + 1) < argc) {
      new_sample_rate = atoi(argv[i + 1]);
    }

    if (strcmp(argv[i], "-c") == 0 && (i + 1) < argc) {
      num_channels = atoi(argv[i + 1]);
    }

    if (strcmp(argv[i], "-r") == 0 && (i + 1) < argc) {
      sample_rate = atoi(argv[i + 1]);
    }

    if (strcmp(argv[i], "-b") == 0 && (i + 1) < argc) {
      bit_depth = atoi(argv[i + 1]);
    }

    if (strcmp(argv[i], "-t") == 0 && (i + 1) < argc) {
      duration = atoi(argv[i + 1]);
    }
  }

  if (mode == 0) {
    if (num_channels == 0) {
      printf("error: number of channels is invalid\n");
      exit(1);
    }

    if (sample_rate == 0) {
      printf("error: sample rate is invalid\n");
      exit(1);
    }

    if (duration == 0) {
      printf("error: duration is invalid\n");
      exit(1);
    }

    generate(num_channels, sample_rate, bit_depth, duration);
  }

  if (mode == 1) {
    if (new_sample_rate == 0) {
      printf("error: new sample rate is invalid\n");
      exit(1);
    }

    resample(in_file, new_sample_rate);
  }
}
