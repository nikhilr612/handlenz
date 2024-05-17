#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

#include "scales.h"
#include "rtmidi_c.h"

typedef struct {
	// If dispersion is lower than this value - ignore landmark set.
	float dispersion_lower_bound;

	// Scale dispersion values by this one.
	float dispersion_upper_bound;

	// If hand hasn't displacement as much as the square root of this value, relative to previous position, do nothing.
	float d2_threshold;

	// Indices per unit distance.
	float index_rate;

	// Maximum volume for emitted notes.
	uint8_t max_volume;

	Scale scale;

	// Id for MIDI output port.
	unsigned int midiout_port;

	// If true, then NOTE_OFF messages are never sent. Otherwise, the last played note will be turned off before the next note is turned on.
	bool let_ring;

	// The MIDI program to use.
	uint8_t program;
} AppConfig;

void default_app_config(AppConfig* cfg, Scale sc);

typedef struct {
	float lastpos[3];
	tone_t last_played;
	RtMidiOutPtr outdevice;
} AppState;

AppState create_app_state();
void set_output_device(AppState* st, RtMidiOutPtr ptr);

#endif