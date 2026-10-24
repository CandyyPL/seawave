#ifndef SW_H_
#define SW_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>

#define PI 3.141592
#define BASE_FREQ 440
#define MIN_AMP 0
#define MID_AMP 8000
#define MAX_AMP 16000

#pragma pack(push, 1)
typedef struct WavHeader {
  // riff chunk
  char riff_id[4];
  uint32_t riff_size;
  char wave_id[4];

  // fmt subchunk
  char fmt[4];
  uint32_t fmt_size;
  uint16_t audio_format;
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;

  // data chunk
  char data_id[4];
  uint32_t data_size;
} wav_header_t;
#pragma pack(pop)

typedef struct WavMetadata {
  uint16_t num_channels;
  uint32_t sample_rate;
  uint16_t bit_depth;
  uint32_t duration;
} wav_metadata_t;

typedef struct WavFile {
  wav_header_t *h;
  wav_metadata_t *m;
  int16_t *data;
} wav_file_t;

typedef struct Note {
  const char *name;
  double freq;
} note_t;

note_t notes[] = {
  {"C4", 261.63},
  {"d4", 277.18},
  {"D4", 293.66},
  {"e4", 311.13},
  {"E4", 329.63},
  {"F4", 349.23},
  {"g4", 369.99},
  {"G4", 392.00},
  {"a4", 415.30},
  {"A4", 440.00},
  {"b4", 466.16},
  {"B4", 493.88}
};

wav_metadata_t get_wav_meta(wav_header_t header);
wav_header_t create_wav_header(uint8_t audio_format, wav_metadata_t wav_meta);

void generate_sine_wave(wav_metadata_t wav_meta, int16_t *buffer, double frequency);
void nn_resample(wav_metadata_t wav_meta, int new_sample_rate, int16_t *buffer_src, int16_t *buffer_dst);

void create_wav_file(const char *filename, wav_metadata_t wav_meta, wav_header_t wav_header, int16_t *buffer);
void write_wav_data(FILE *fptr, wav_metadata_t wav_meta, wav_header_t header, int16_t *data);
void read_wav_meta(const char *filename, wav_metadata_t *wav_meta);
void read_wav_data(const char *filename, int total_samples, int16_t *wav_data);

void get_resampled_filename(const char *filename, int new_sample_rate, char *new_filename);

int get_total_samples(wav_metadata_t wav_meta, int target_sample_rate);

int parse_int(const char *flag, const char *s);

double note_freq(const char *key);

#ifdef SW_IMP_

wav_metadata_t get_wav_meta(wav_header_t header) {
  int num_samples = header.data_size / header.block_align;
  int duration = num_samples / header.sample_rate;

  wav_metadata_t wav_meta = {
    .num_channels = header.num_channels,
    .sample_rate = header.sample_rate,
    .bit_depth = header.bits_per_sample,
    .duration = duration
  };

  return wav_meta;
}

wav_header_t create_wav_header(uint8_t audio_format, wav_metadata_t wav_meta) {
  wav_header_t h;

  memcpy(h.riff_id, "RIFF", 4);
  memcpy(h.wave_id, "WAVE", 4);
  memcpy(h.fmt, "fmt ", 4);
  memcpy(h.data_id, "data", 4);

  h.fmt_size = 16;
  h.audio_format = audio_format;
  h.num_channels = wav_meta.num_channels;
  h.sample_rate = wav_meta.sample_rate;
  h.block_align = (wav_meta.bit_depth / 8) * wav_meta.num_channels;
  h.byte_rate = wav_meta.sample_rate * h.block_align;
  h.bits_per_sample = wav_meta.bit_depth;

  int num_frames = wav_meta.sample_rate * wav_meta.duration;

  h.data_size = num_frames * h.block_align;
  h.riff_size = h.data_size + 36;

  return h;
}

void generate_sine_wave(wav_metadata_t wav_meta, int16_t *buffer, double frequency) {
  int num_frames = wav_meta.sample_rate * wav_meta.duration;

  for (int i = 0; i < num_frames; i++) {
    double t = (double)i / wav_meta.sample_rate;
    double value = sin(2.0 * PI * frequency * t);

    for (int c = 0; c < wav_meta.num_channels; c++) {
      buffer[i * wav_meta.num_channels + c] = value * MID_AMP;
    }
  }
}

void nn_resample(wav_metadata_t wav_meta, int new_sample_rate, int16_t *buffer_src, int16_t *buffer_dst) {
  double ratio = (double)wav_meta.sample_rate / new_sample_rate;
  int num_frames = wav_meta.sample_rate * wav_meta.duration;
  double new_num_frames = round(num_frames / ratio);

  for (int i = 0; i < new_num_frames; i++) {
    int source = round(i * ratio);

    if (source < num_frames) {
      for (int c = 0; c < wav_meta.num_channels; c++) {
        buffer_dst[i * wav_meta.num_channels + c] = buffer_src[source * wav_meta.num_channels + c];
      }
    }
    else break;
  }
}

void create_wav_file(const char *filename, wav_metadata_t wav_meta, wav_header_t wav_header, int16_t *buffer) {
  FILE *fptr = fopen(filename, "wb");

  if (fptr == NULL) {
    fprintf(stderr, "error: could not open a file\n");
  }

  write_wav_data(fptr, wav_meta, wav_header, buffer);
  fclose(fptr);
}

void write_wav_data(FILE *fptr, wav_metadata_t wav_meta, wav_header_t header, int16_t *data) {
  fwrite(&header, sizeof(header), 1, fptr);

  int total_samples = wav_meta.duration * wav_meta.sample_rate * wav_meta.num_channels;
  fwrite(data, sizeof(int16_t), total_samples, fptr);
}

void read_wav_meta(const char *filename, wav_metadata_t *wav_meta) {
  FILE *fptr = fopen(filename, "rb");

  if (fptr == NULL) {
    fprintf(stderr, "error: could not open file for read\n");
  }

  wav_header_t wav_header;

  fread(&wav_header, sizeof(wav_header_t), 1, fptr);
  *wav_meta = get_wav_meta(wav_header);

  fclose(fptr);
}

void read_wav_data(const char *filename, int total_samples, int16_t *wav_data) {
  FILE *fptr = fopen(filename, "rb");

  if (fptr == NULL) {
    printf("error: could not open file for read\n");
  }

  fread(wav_data, sizeof(int16_t), total_samples, fptr);
  fclose(fptr);
}

void get_resampled_filename(const char *filename, int new_sample_rate, char *new_filename) {
  char base_name[64];
  snprintf(base_name, 64, "%s", filename);

  char *ext = strstr(base_name, ".wav");
  if (ext != NULL) *ext = '\0';

  snprintf(new_filename, 64, "%s_%d.wav", base_name, new_sample_rate);
}

int get_total_samples(wav_metadata_t wav_meta, int target_sample_rate) {
  int num_samples = wav_meta.sample_rate * wav_meta.duration;

  double ratio = (double)wav_meta.sample_rate / target_sample_rate;
  int new_num_frames = round(num_samples / ratio);
  return new_num_frames * wav_meta.num_channels;
}

int parse_int(const char *flag, const char *s) {
  char *end;
  errno = 0;

  long n = strtol(s, &end, 10);

  if (errno || end == s || *end != '\0' || n < 0 || n > INT_MAX) {
    fprintf(stderr, "%s: invalid number '%s'\n", flag, s);
    exit(1);
  }

  return (int)n;
}

double note_freq(const char *key) {
  for (size_t i = 0; i < sizeof(notes) / sizeof(*notes); i++) {
    if (strcmp(notes[i].name, key) == 0) return notes[i].freq;
  }

  return 0.0;
}

#endif // SW_IMP_

#endif // SW_H_
