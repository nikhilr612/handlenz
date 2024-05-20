#include "app.h"
#include "scales.h"
#include <stdint.h>

void default_app_config(AppConfig *cfg, Scale sc) {
	cfg -> d2_threshold = 0.001;
	cfg -> dispersion_lower_bound = 0.09;
	cfg -> dispersion_upper_bound = 0.20;
	cfg -> index_rate = 4.0;
	cfg -> max_volume = 127;
	cfg -> midiout_port = 0;
	cfg -> let_ring = false;
	cfg -> program = 0;
	cfg -> scale = sc;
	cfg -> cooldown_millis = 100;
}

// Create a safe app state.
AppState create_app_state() {
	AppState ret;

	ret.lastpos[0] = 0;
	ret.lastpos[1] = 0;
	ret.lastpos[2] = 0;

	ret.last_played = 0;
	ret.outdevice = NULL;
	ret.last_time = 0;

	return ret;
}

void set_output_device(AppState *st, RtMidiOutPtr ptr) {
	st->outdevice = ptr;
}

void program_change(RtMidiOutPtr outdevice, uint8_t program) {
    if (outdevice != NULL) {
        uint8_t message[2] = {192, program};
        rtmidi_out_send_message(outdevice, message, 2);
    }
}