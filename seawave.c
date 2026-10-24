#define SW_IMP_
#include "seawave.h"

void generate(int num_channels, int sample_rate, int bit_depth, int duration, char sound) {
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

  double sound_freq = note_freq(&sound);

  wav_header_t wav_header = create_wav_header(1, wav_meta);
  generate_sine_wave(wav_meta, buffer, sound_freq);
  create_wav_file(output, wav_meta, wav_header, buffer);
  free(buffer);

  exit(0);
}

void resample(const char *filename, int new_sample_rate) {
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

void read_sheet(const char *filename) {
  FILE *fptr = fopen(filename, "r");

  if (fptr == NULL) {
    fprintf(stderr, "error: could not open a file\n");
    exit(1);
  }

  char line[256];
  char melody[64][3];

  int bpm = 0;
  int notes_per_bar = 0;
  int note_type = 0;

  int ln = 1;
  int nn = 0;

  while (fgets(line, sizeof(line), fptr)) {
    if (ln == 1) {
      sscanf(line, "%d %d/%d", &bpm, &notes_per_bar, &note_type);
    }

    if (ln > 1) {
      char *tok = strtok(line, " \r\n");

      while (tok) {
        snprintf(melody[nn++], sizeof(melody[0]), "%s", tok);
        tok = strtok(NULL, " \r\n");
      }
    }

    ln++;
  }

  printf("BPM: %d\n", bpm);
  printf("Scheme: %d/%d\n", notes_per_bar, note_type);

  for (int i = 0; i < nn; i++) {
    printf("%s -> %.02f\n", melody[i], note_freq(melody[i]));
  }

  fclose(fptr);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("usage: ./seawave -m <mode> -f <input.wav> <mode_options>\n");
    exit(0);
  }

  int mode = 0;

  int num_channels = 2;
  int sample_rate = 48000;
  int new_sample_rate = 0;
  int bit_depth = 16;
  int duration = 0;
  int bpm = 0;
  char sound = 'C';

  const char *in_file = "";

  for (int i = 1; i < argc; i++) {
    const char *flag = argv[i];

    if (i + 1 >= argc) {
      fprintf(stderr, "%s needs a value\n", flag);
      return 1;
    }

    const char *val = argv[++i];

    if (strcmp(flag, "-m") == 0) {
        if (strcmp(val, "generate") == 0) mode = 0;
        else if (strcmp(val, "resample") == 0) mode = 1;
        else if (strcmp(val, "melody") == 0) mode = 2;
        else { fprintf(stderr, "unknown mode: %s\n", val); return 1; }
    }
    else if (strcmp(flag, "-f") == 0) in_file = val;
    else if (strcmp(flag, "--rate") == 0) sample_rate = parse_int(flag, val);
    else if (strcmp(flag, "-n") == 0) new_sample_rate = parse_int(flag, val);
    else if (strcmp(flag, "-c") == 0) num_channels = parse_int(flag, val);
    else if (strcmp(flag, "-b") == 0) bit_depth = parse_int(flag, val);
    else if (strcmp(flag, "-d") == 0) duration = parse_int(flag, val);
    else if (strcmp(flag, "-b") == 0) bpm = parse_int(flag, val);
    else if (strcmp(flag, "--sound") == 0) sound = val[0];
    else { fprintf(stderr, "unknown option: %s\n", flag); return 1; }
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

    generate(num_channels, sample_rate, bit_depth, duration, sound);
  }

  if (mode == 1) {
    if (strcmp(in_file, "") == 0) {
      fprintf(stderr, "error: wav file is required for resampling\n");
      exit(1);
    }

    if (new_sample_rate == 0) {
      printf("error: new sample rate is invalid\n");
      exit(1);
    }

    resample(in_file, new_sample_rate);
  }

  if (mode == 2) {
    if (strcmp(in_file, "") == 0) {
      fprintf(stderr, "error: sheet is required for melody mode\n");
      exit(1);
    }

    read_sheet(in_file);
  }
}
