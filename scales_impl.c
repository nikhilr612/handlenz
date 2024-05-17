#include "scales.h"

int8_t parse_reg(char c) {
	if ('0' <= c && c <= '9') {
		signed char d = ((c - '0') - 4);
		return (int8_t)d*12;
	} else {
		return 0;
	}
}

tone_t midi_key_by_name(const char *name) {
	tone_t ret = 0;

	switch (name[0]) {
	case 'C':
		ret = 60;
		break;
	case 'D':
		ret = 62;
		break;
	case 'E':
		ret = 64;
		break;
	case 'F':
		ret = 65;
		break;
	case 'G':
		ret = 67;
		break;
	case 'A':
		ret = 69;
		break;
	case 'B':
		ret = 71;
		break;
	default:
		ret = 60;
		break;
	};

	switch (name[1]) {
		case '#':
			ret++;
			break;
		case 'b':
			ret--;
			break;
		default:
			ret += parse_reg(name[1]);
			return ret;
	}

	ret += parse_reg(name[2]);
	return ret;
}

Scale scale_from_id(uint16_t id, tone_t root) {
	Scale ret;
	ret.root = root;

	uint8_t n = 0;
	tone_t i = 0;

	while ((id != 0) && (n < 12)) {
		if (id & 1) {
			ret.tone_list[n++] = i;
		}
		id >>= 1;
		i++;
	}

	ret.n_tones = n;
	return ret;
}

tone_t index_scale(const Scale* sc, size_t i) {
	tone_t ret = sc -> root;
	size_t n_regs = i / (sc -> n_tones) % 10;
	if (n_regs > 9) {
		n_regs = 9;
	}
	ret += (sc -> tone_list)[i % (sc -> n_tones)];
	ret += 12*n_regs;
	return ret;
}