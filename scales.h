#ifndef SCALES_H
#define SCALES_H

/**
 * Utilities for working with musical scales in MIDI.
 * */

#include <stdint.h>

// An midi key associated with a tone.
// Defined to be an unsigned 8-bit integer in accordance with midi standard. 
typedef uint8_t tone_t;

// Get the midi key corresponding to 12-tone scientific pitch notation.
// Importantly, double flats or double sharps are not recognized.
// Example midi_key_by_name("F#4") returns 66.
// Example midi_key_by_name("C4") return 60.
// Atmost 3 characters are read, and atleast 2 characters must be specified.
// If key letter is not valid, it is assumed to be C.
tone_t midi_key_by_name(const char* name);

// Immutable type to work with scales.
typedef struct {
	// Total number of tones in the scale.
	uint8_t n_tones;
	// The root of the scale
	tone_t root;
	// The list of tones relative to root, present in the scale.
	tone_t tone_list[12];
} Scale;

// Return a tone form the scale as per the integer `i`.
// This the same as `root + tone_list[i % n_tones] + (i / n_tones)*12`.
tone_t index_scale(const Scale* sc, size_t i);

// Create a scale
Scale scale_from_id(uint16_t id, tone_t root);

#endif