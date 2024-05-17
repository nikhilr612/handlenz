#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "libmpc.h"
#include "app.h"

AppConfig global_config;

char* readFile(const char* filename) {
    FILE* file = fopen(filename, "r"); // Open file for reading
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        return NULL;
    }

    // Allocate initial buffer size
    size_t buffer_size = 1024; // Initial buffer size
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        fclose(file);
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    size_t length = 0; // Current length of string in buffer

    // Read file line by line
    char line[1024]; // Buffer for reading lines
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t line_length = strlen(line);
        // Resize buffer if necessary
        while (length + line_length + 1 > buffer_size) {
            buffer_size += buffer_size * 3 / 4;
            char* new_buffer = (char*)realloc(buffer, buffer_size);
            if (new_buffer == NULL) {
                fclose(file);
                free(buffer);
                fprintf(stderr, "Memory reallocation failed\n");
                return NULL;
            }
            buffer = new_buffer;
        }
        // Concatenate line to buffer
        strcpy(buffer + length, line);
        length += line_length;
    }

    fclose(file);

    // Null-terminate the buffer
    buffer[length] = '\0';

    return buffer;
}

void program_change(RtMidiOutPtr outdevice, uint8_t program) {
    if (outdevice != NULL) {
        uint8_t message[2] = {192, program};
        rtmidi_out_send_message(outdevice, message, 2);
    }
}

void on_capture(size_t idx, size_t n, LandmarkBuf* buf, void* userdata) {
	printf("Call %zu, %zu\n", idx, n);

    // TODO: Multi-hand support.
    if (idx > 0) return;

    float dispersion = 0;
    float ux = 0, uy = 0, uz = 0;
    size_t n_landmarks = (buf -> length);

	for (unsigned i = 0; i < n_landmarks; i++){
		float co_ords[3] = {0, 0, 0};
		MP_GetAtBuf(buf, i, co_ords);

        // Debug
		// printf("Landmark [h: %zu, i: %u] (%g, %g, %g)\n", idx, i, co_ords[0], co_ords[1], co_ords[2]);

        dispersion += co_ords[0]*co_ords[0];
        dispersion += co_ords[1]*co_ords[1];
        dispersion += co_ords[2]*co_ords[2];

        ux += co_ords[0];
        uy += co_ords[1];
        uz += co_ords[2];
	}

    dispersion = dispersion / n_landmarks;

    ux /= n_landmarks;
    uy /= n_landmarks;
    uz /= n_landmarks;

    AppState* app_state = (AppState*)userdata;

    float d2 = 0;
    d2 += pow((ux - (app_state -> lastpos[0])), 2.0);
    d2 += pow((uy - (app_state -> lastpos[1])), 2.0);
    d2 += pow((uz - (app_state -> lastpos[2])), 2.0);

    dispersion -= d2;
    dispersion = sqrt(dispersion);

    printf("dispersion: %g, d2: %g, s: %g\n", dispersion, d2, sqrt(d2));

    // Skip if threshold condition is not met. We don't want to emit notes when 
    if (d2 < global_config.d2_threshold) return;

    // If hand is in fist configuration, and dispersion is below the lower bound, volume = 0.
    // Skip, instead of sending a muted message.
    if (dispersion < global_config.dispersion_lower_bound) return;

    app_state -> lastpos[0] = ux;
    app_state -> lastpos[1] = uy;
    app_state -> lastpos[2] = uz;

    // Recenter.
    ux -= 0.5;
    uy -= 0.5;

    float r = sqrt(ux*ux + uy*uy); // Not taking uz into account. Z-position should acconut for velocity, indirectly through dispersion.
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
    printf("Successfully loaded graph file into memory.");

    AppState state = create_app_state();

    // C4 pentatonic scale.
    Scale sc = scale_from_id(661, midi_key_by_name("C4"));

    // TODO: Implement loading configuration from file.
    default_app_config(&global_config, sc);

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