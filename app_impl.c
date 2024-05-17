#include "app.h"
#include "scales.h"
#include <stdint.h>

void default_app_config(AppConfig *cfg, Scale sc) {
	cfg -> d2_threshold = 0.001;
	cfg -> dispersion_lower_bound = 0.08;
	cfg -> dispersion_upper_bound = 0.20;
	cfg -> index_rate = 8.0;
	cfg -> max_volume = 127;
	cfg -> midiout_port = 0;
	cfg -> let_ring = false;
	cfg -> program = 46;
	cfg -> scale = sc;
}

// Create a safe app state.
AppState create_app_state() {
	AppState ret;

	ret.lastpos[0] = 0;
	ret.lastpos[1] = 0;
	ret.lastpos[2] = 0;

	ret.last_played = 0;
	ret.outdevice = NULL;

	return ret;
}

void set_output_device(AppState *st, RtMidiOutPtr ptr) {
	st->outdevice = ptr;
}