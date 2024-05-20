#include <math.h>
#include <stdint.h>

#include "utils.h"
#include "libmpc.h"
#include "app.h"

AppConfig global_config;

void on_capture(size_t idx, size_t n, LandmarkBuf* buf, void* userdata) {
	printf("call %zu, %zu: ", idx, n);

    // TODO: Multi-hand support.
    if (idx > 0) return;

    float ux = 0, uy = 0, uz = 0;
    float co_ords[3] = {0, 0, 0};
    size_t n_landmarks = (buf -> length);

    float dispersion = 0.0;

    for (unsigned i = 4; i < n_landmarks; i += 4) {
        MP_GetAtBuf(buf, i, co_ords);

        float tx = co_ords[0], ty = co_ords[1], tz = co_ords[2];

        MP_GetAtBuf(buf, i-3, co_ords);

        tx -= co_ords[0];
        ty -= co_ords[1];
        tz -= co_ords[2];

        dispersion += tx*tx + ty*ty + tz*tz;

        ux += co_ords[0];
        uy += co_ords[1];
        uz += co_ords[2];
    }

    MP_GetAtBuf(buf, 0, co_ords);

    ux += co_ords[0];
    uy += co_ords[1];
    uz += co_ords[2];

    // 5 fingers + base.
    ux /= 6;
    uy /= 6;
    uz /= 6;

    AppState* app_state = (AppState*)userdata;

    float d2 = 0;
    d2 += pow((ux - (app_state -> lastpos[0])), 2.0);
    d2 += pow((uy - (app_state -> lastpos[1])), 2.0);
    d2 += pow((uz - (app_state -> lastpos[2])), 2.0);

    dispersion /= 5; // Five fingers per hand.
    dispersion = sqrt(dispersion);

    printf("dispersion: %g, d2: %g, s: %g\n", dispersion, d2, sqrt(d2));

    // Skip if threshold condition is not met. We don't want to emit notes when 
    if (d2 < global_config.d2_threshold) return;

    // If hand is in fist configuration, and dispersion is below the lower bound, volume = 0.
    // Skip, instead of sending a muted message.
    if (dispersion < global_config.dispersion_lower_bound) return;

    clock_t current_time = clock();
    clock_t elapsed = 1000*(current_time - (app_state -> last_time)) / CLOCKS_PER_SEC;

    // If sufficient time hasn't elapsed, skip.
    if (elapsed < global_config.cooldown_millis) return;
    else app_state -> last_time = current_time; // Otherwise update time.

    app_state -> lastpos[0] = ux;
    app_state -> lastpos[1] = uy;
    app_state -> lastpos[2] = uz;

    float r = fabs(ux - 0.5) + fabs(uy - 0.5) / sqrt(2);
    tone_t midi_key = index_scale(&global_config.scale, (size_t)(global_config.index_rate * r));

    uint8_t velocity = (uint8_t)(global_config.max_volume * (dispersion / global_config.dispersion_upper_bound));
    if (velocity > global_config.max_volume){
        velocity = global_config.max_volume;
    }

    if (app_state -> outdevice != NULL) {
        uint8_t message[3] = {144, midi_key, 0};
        if (!global_config.let_ring) {
            // Note-off
            rtmidi_out_send_message(app_state -> outdevice, message, 3);
        }
        message[2] = velocity;
        printf("key: %u\n, velocity: %u\n", midi_key, velocity);
        rtmidi_out_send_message(app_state -> outdevice, message, 3);

    } // Do nothing if the app state was poorly made.
}

int main() {
	const char* graph_spec = readFile("mediapipe/graphs/hand_tracking/hand_tracking_desktop_live.pbtxt");
    if (graph_spec == NULL) {
        printf("Failed to load graph.\n");
        return -1;
    }
    printf("Successfully loaded graph file into memory.\n");

    AppState state = create_app_state();

    // C4 pentatonic scale.
    Scale sc = scale_from_id(661, midi_key_by_name("C4"));

    // TODO: Implement loading configuration from file.
    default_app_config(&global_config, sc);

    if(app_config_from_ini(&global_config, "HandLenz.ini")) printf("\nLoaded configuration. ");
    else fprintf(stderr, "Warning: Could not load local configuration file. Falling back to defaults.\n");

    RtMidiOutPtr outdevice = rtmidi_out_create(RTMIDI_API_WINDOWS_MM, "handlenz");

    if (rtmidi_get_port_count(outdevice) == 0){
        printf("No MIDI output ports available.\n");
        return -1;
    }

    rtmidi_open_port(outdevice, global_config.midiout_port, "handlenz_port");
    program_change(outdevice, global_config.program);
    set_output_device(&state, outdevice);

    printf("Opened MIDI Output Port. Device is ready.\n");

	// Initialize LibMP graph.
	MPHandle landmarker = MP_Create(graph_spec, "input_video");
	MP_AddOutputStream(landmarker, "landmarks");

	MP_Start(landmarker);

	LandmarkBuf buf;
	if(!MP_InitBuf(&buf, 24)) {
		printf("Failed to initialize Landmark buffer.");
		return -1;
	}

    // No userdata here.
	MPCV_VideoCapture(0, landmarker, "landmarks", &buf, on_capture, (void*)(&state));

	MP_FreeBuf(&buf);

    // AppState is not responsible for free-ing these resources.
    rtmidi_close_port(outdevice);
    rtmidi_out_free(outdevice);

	MP_Delete(landmarker);
	free((void*)graph_spec);
	return 0;
}