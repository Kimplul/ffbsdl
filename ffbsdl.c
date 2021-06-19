#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_haptic.h>

typedef uint32_t effect_mask;

typedef enum {
	// top-level choices
	CREATE_EFFECT,
	MODIFY_EFFECT,
	PLAY_EFFECT,
	STOP_EFFECT,
	DESTROY_EFFECT,
	SET_AUTOCENTER,
	SET_GAIN,
	QUIT,

	// effect creation choices
	CREATE_CONSTANT,
	CREATE_SINE,
	CREATE_TRIANGLE,
	CREATE_SAWTOOTHUP,
	CREATE_SAWTOOTHDOWN,
	CREATE_RAMP,
	CREATE_SPRING,
	CREATE_DAMPER,
	CREATE_INERTIA,
	CREATE_FRICTION,

	// effect modification choices
	MODIFY_CONSTANT,
	MODIFY_SINE,
	MODIFY_TRIANGLE,
	MODIFY_SAWTOOTHUP,
	MODIFY_SAWTOOTHDOWN,
	MODIFY_RAMP,
	MODIFY_SPRING,
	MODIFY_DAMPER,
	MODIFY_INERTIA,
	MODIFY_FRICTION,

	TRY_AGAIN,
} choice;

typedef struct {
	effect_mask effect;
	char *option_str;
} effect_choice;

typedef struct {
	SDL_HapticEffect effect;
	int id;
	bool active;
} haptic_elem;

int init(){
	int ret = SDL_Init(SDL_INIT_HAPTIC);
	if(ret){
		puts(SDL_GetError());
		return ret;
	}

	return 0;
}

void cleanup(){
	SDL_Quit();
}

SDL_Haptic *get_haptic(){
	// open first haptic device, could be a good idea to try and let the
	// user choose which device to open but for now this is alright
	SDL_Haptic *haptic = SDL_HapticOpen(0);
	if(!haptic){
		fputs("Couldn't open haptic device.", stderr);
	} else {
		puts("Found haptic device:");
		puts(SDL_HapticName(0));
	}

	return haptic;
}

effect_mask get_supported_effects(SDL_Haptic *haptic){
	return SDL_HapticQuery(haptic);
}

void destroy_haptic(SDL_Haptic *haptic){
	SDL_HapticClose(haptic);
}

void destroy_joystick(SDL_Joystick *joy){
	SDL_JoystickClose(joy);
}

const char *get_haptic_type_name(uint16_t type){
#define CASE(x) case SDL_HAPTIC_##x: return #x

	switch(type){
		CASE(CONSTANT);
		CASE(SINE);
		CASE(TRIANGLE);
		CASE(SAWTOOTHUP);
		CASE(SAWTOOTHDOWN);
		CASE(RAMP);
		CASE(SPRING);
		CASE(DAMPER);
		CASE(FRICTION);
		CASE(INERTIA);
		CASE(CUSTOM);
	}

	return "ERR";

#undef CASE
}

void show_status(SDL_Haptic *haptic, size_t num_elems, haptic_elem elems[]){
	puts("EFFECTS:");
	puts("ID\tNAME\tSTATUS");

	for(size_t i = 0; i < num_elems; ++i){
		if(elems[i].active)
			printf("%i\t%s\t%s\n",
					elems[i].id,
					get_haptic_type_name(elems[i].effect.type),
					SDL_HapticGetEffectStatus(haptic, elems[i].id) ? "PLAYING" : "STOPPED"
			      );
	}

	puts("");
}

void show_choices(){
	puts("c: Create effect");
	puts("m: Modify effect");
	puts("p: Play effect");
	puts("s: Stop effect");
	puts("d: Destroy effect");
	puts("g: Set gain");
	puts("a: Set autocenter");
	puts("q: Quit");
}

void discard_line(){
	while(getchar() != '\n');
}

choice get_choice(){
	char c = 0;
	scanf("%c", &c);
	discard_line();

	switch(c){
		case 'c': return CREATE_EFFECT;
		case 'm': return MODIFY_EFFECT;
		case 'p': return PLAY_EFFECT;
		case 's': return STOP_EFFECT;
		case 'd': return DESTROY_EFFECT;
		case 'a': return SET_AUTOCENTER;
		case 'g': return SET_GAIN;
		case 'q': return QUIT;
	}

	return TRY_AGAIN;
}

int get_int(const char *s, long long int min, long long int max, long long int d){
	// slightly dangerous, since someone could perform a buffer overflow
	// attack but I'll let it slide this once
	printf(s, min, max, d);

	int res;
	char in[80];
	fgets(in, sizeof(in), stdin);

	if(in[0] == '\n')
		res = d;
	else
		sscanf(in, "%i", &res);

	if(strlen(in) >= 79)
		discard_line();

	if(res < min)
		res = min;

	if(res > max)
		res = max;

	return res;
}

#define FFB_ATTR(n, x, a, b) \
	effect->n.x = get_int(#x " [%lli - %lli, current %lli]: ", a, b, effect->n.x)


#define CONSTANT_ATTR(x, a, b) \
	FFB_ATTR(constant, x, a, b)

#define SHORT_CONSTANT_ATTR(x) \
	CONSTANT_ATTR(x, 0, USHRT_MAX)


#define PERIODIC_ATTR(x, a, b) \
	FFB_ATTR(periodic, x, a, b)

#define SHORT_PERIODIC_ATTR(x) \
	PERIODIC_ATTR(x, 0, USHRT_MAX)


#define RAMP_ATTR(x, a, b) \
	FFB_ATTR(ramp, x, a, b)

#define SHORT_RAMP_ATTR(x) \
	RAMP_ATTR(x, 0, USHRT_MAX)


#define COND_ATTR(x, a, b) \
	FFB_ATTR(condition, x, a, b)

#define SHORT_COND_ATTR(x) \
	COND_ATTR(x, 0, USHRT_MAX)

void get_condition_effect_input(SDL_HapticEffect *effect){
	COND_ATTR(direction.dir[0], 0, 36000);
	COND_ATTR(length, 0, UINT_MAX);
	SHORT_COND_ATTR(delay);

	SHORT_COND_ATTR(right_sat[0]);
	SHORT_COND_ATTR(left_sat[0]);
	SHORT_COND_ATTR(right_coeff[0]);
	SHORT_COND_ATTR(left_coeff[0]);
	SHORT_COND_ATTR(deadband[0]);
	COND_ATTR(center[0], SHRT_MIN, SHRT_MAX);
}

void modify_inertia(SDL_Haptic *haptic, int id, SDL_HapticEffect *effect){
	get_condition_effect_input(effect);
	SDL_HapticUpdateEffect(haptic, id, effect);
}

int create_inertia(SDL_Haptic *haptic, SDL_HapticEffect *effect){
	effect->type = SDL_HAPTIC_INERTIA;

	effect->condition.type = SDL_HAPTIC_INERTIA;

	effect->condition.direction.type = SDL_HAPTIC_CARTESIAN;
	effect->condition.direction.dir[0] = 9000;
	effect->condition.length = 2000;
	effect->condition.delay = 0;

	effect->condition.right_sat[0] = 0;
	effect->condition.left_sat[0] = 0;
	effect->condition.right_coeff[0] = 0;
	effect->condition.left_coeff[0] = 0;
	effect->condition.deadband[0] = 0;
	effect->condition.center[0] = 0;

	get_condition_effect_input(effect);

#define SET(x) effect->condition.x

	SET(right_sat[2]) 	= SET(right_sat[1]) 	= SET(right_sat[0]);
	SET(left_sat[2]) 	= SET(left_sat[1]) 	= SET(left_sat[0]);
	SET(right_coeff[2]) 	= SET(right_coeff[1]) 	= SET(right_coeff[0]);
	SET(left_coeff[2]) 	= SET(left_coeff[1]) 	= SET(left_coeff[0]);
	SET(deadband[2]) 	= SET(deadband[1]) 	= SET(deadband[0]);
	SET(center[2]) 		= SET(center[1]) 	= SET(center[0]);

#undef SET

	return SDL_HapticNewEffect(haptic, effect);
}

void modify_friction(SDL_Haptic *haptic, int id, SDL_HapticEffect *effect){
	get_condition_effect_input(effect);
	SDL_HapticUpdateEffect(haptic, id, effect);
}

int create_friction(SDL_Haptic *haptic, SDL_HapticEffect *effect){
	effect->type = SDL_HAPTIC_FRICTION;

	effect->condition.type = SDL_HAPTIC_FRICTION;

	effect->condition.direction.type = SDL_HAPTIC_CARTESIAN;
	effect->condition.direction.dir[0] = 9000;
	effect->condition.length = 2000;
	effect->condition.delay = 0;

	effect->condition.right_sat[0] = 0;
	effect->condition.left_sat[0] = 0;
	effect->condition.right_coeff[0] = 0;
	effect->condition.left_coeff[0] = 0;
	effect->condition.deadband[0] = 0;
	effect->condition.center[0] = 0;

	get_condition_effect_input(effect);

#define SET(x) effect->condition.x

	SET(right_sat[2]) 	= SET(right_sat[1]) 	= SET(right_sat[0]);
	SET(left_sat[2]) 	= SET(left_sat[1]) 	= SET(left_sat[0]);
	SET(right_coeff[2]) 	= SET(right_coeff[1]) 	= SET(right_coeff[0]);
	SET(left_coeff[2]) 	= SET(left_coeff[1]) 	= SET(left_coeff[0]);
	SET(deadband[2]) 	= SET(deadband[1]) 	= SET(deadband[0]);
	SET(center[2]) 		= SET(center[1]) 	= SET(center[0]);

#undef SET

	return SDL_HapticNewEffect(haptic, effect);
}

void modify_damper(SDL_Haptic *haptic, int id, SDL_HapticEffect *effect){
	get_condition_effect_input(effect);
	SDL_HapticUpdateEffect(haptic, id, effect);
}

int create_damper(SDL_Haptic *haptic, SDL_HapticEffect *effect){
	effect->type = SDL_HAPTIC_DAMPER;

	effect->condition.type = SDL_HAPTIC_DAMPER;

	effect->condition.direction.type = SDL_HAPTIC_CARTESIAN;
	effect->condition.direction.dir[0] = 9000;
	effect->condition.length = 2000;
	effect->condition.delay = 0;

	effect->condition.right_sat[0] = 0;
	effect->condition.left_sat[0] = 0;
	effect->condition.right_coeff[0] = 0;
	effect->condition.left_coeff[0] = 0;
	effect->condition.deadband[0] = 0;
	effect->condition.center[0] = 0;

	get_condition_effect_input(effect);

#define SET(x) effect->condition.x

	SET(right_sat[2]) 	= SET(right_sat[1]) 	= SET(right_sat[0]);
	SET(left_sat[2]) 	= SET(left_sat[1]) 	= SET(left_sat[0]);
	SET(right_coeff[2]) 	= SET(right_coeff[1]) 	= SET(right_coeff[0]);
	SET(left_coeff[2]) 	= SET(left_coeff[1]) 	= SET(left_coeff[0]);
	SET(deadband[2]) 	= SET(deadband[1]) 	= SET(deadband[0]);
	SET(center[2]) 		= SET(center[1]) 	= SET(center[0]);

#undef SET

	return SDL_HapticNewEffect(haptic, effect);
}

void modify_spring(SDL_Haptic *haptic, int id, SDL_HapticEffect *effect){
	get_condition_effect_input(effect);
	SDL_HapticUpdateEffect(haptic, id, effect);
}

int create_spring(SDL_Haptic *haptic, SDL_HapticEffect *effect){
	effect->type = SDL_HAPTIC_SPRING;

	effect->condition.type = SDL_HAPTIC_SPRING;

	effect->condition.direction.type = SDL_HAPTIC_CARTESIAN;
	effect->condition.direction.dir[0] = 9000;
	effect->condition.length = 2000;
	effect->condition.delay = 0;

	effect->condition.right_sat[0] = 0;
	effect->condition.left_sat[0] = 0;
	effect->condition.right_coeff[0] = 0;
	effect->condition.left_coeff[0] = 0;
	effect->condition.deadband[0] = 0;
	effect->condition.center[0] = 0;

	get_condition_effect_input(effect);

#define SET(x) effect->condition.x

	SET(right_sat[2]) 	= SET(right_sat[1]) 	= SET(right_sat[0]);
	SET(left_sat[2]) 	= SET(left_sat[1]) 	= SET(left_sat[0]);
	SET(right_coeff[2]) 	= SET(right_coeff[1]) 	= SET(right_coeff[0]);
	SET(left_coeff[2]) 	= SET(left_coeff[1]) 	= SET(left_coeff[0]);
	SET(deadband[2]) 	= SET(deadband[1]) 	= SET(deadband[0]);
	SET(center[2]) 		= SET(center[1]) 	= SET(center[0]);

#undef SET

	return SDL_HapticNewEffect(haptic, effect);
}

void get_ramp_effect_input(SDL_HapticEffect *effect){
	RAMP_ATTR(direction.dir[0], 0, 36000);
	RAMP_ATTR(length, 0, UINT_MAX);
	SHORT_RAMP_ATTR(delay);

	SHORT_RAMP_ATTR(start);
	SHORT_RAMP_ATTR(end);

	SHORT_RAMP_ATTR(attack_length);
	SHORT_RAMP_ATTR(attack_level);
	SHORT_RAMP_ATTR(fade_length);
	SHORT_RAMP_ATTR(fade_level);
}

void modify_ramp(SDL_Haptic *haptic, int id, SDL_HapticEffect *effect){
	get_ramp_effect_input(effect);
	SDL_HapticUpdateEffect(haptic, id, effect);
}

int create_ramp(SDL_Haptic *haptic, SDL_HapticEffect *effect){
	effect->type = SDL_HAPTIC_RAMP;
	effect->ramp.type = SDL_HAPTIC_TRIANGLE;

	effect->ramp.direction.type = SDL_HAPTIC_CARTESIAN;
	effect->ramp.direction.dir[0] = 9000;
	effect->ramp.length = 2000;
	effect->ramp.delay = 0;

	effect->ramp.start = 0;
	effect->ramp.end = 65535;

	effect->ramp.attack_length = 0;
	effect->ramp.attack_level = 0;
	effect->ramp.fade_length = 0;
	effect->ramp.fade_level = 0;

	get_ramp_effect_input(effect);

	return SDL_HapticNewEffect(haptic, effect);
}

void get_periodic_effect_input(SDL_HapticEffect *effect){
	PERIODIC_ATTR(direction.dir[0], 0, 36000);
	PERIODIC_ATTR(length, 0, UINT_MAX);
	SHORT_PERIODIC_ATTR(delay);

	SHORT_PERIODIC_ATTR(period);
	SHORT_PERIODIC_ATTR(magnitude);
	SHORT_PERIODIC_ATTR(offset);
	SHORT_PERIODIC_ATTR(phase);

	SHORT_PERIODIC_ATTR(attack_length);
	SHORT_PERIODIC_ATTR(attack_level);
	SHORT_PERIODIC_ATTR(fade_length);
	SHORT_PERIODIC_ATTR(fade_level);
}

void modify_triangle(SDL_Haptic *haptic, int id, SDL_HapticEffect *effect){
	get_periodic_effect_input(effect);
	SDL_HapticUpdateEffect(haptic, id, effect);
}

int create_triangle(SDL_Haptic *haptic, SDL_HapticEffect *effect){
	effect->type = SDL_HAPTIC_TRIANGLE;
	effect->periodic.type = SDL_HAPTIC_TRIANGLE;

	effect->periodic.direction.type = SDL_HAPTIC_CARTESIAN;
	effect->periodic.direction.dir[0] = 9000;
	effect->periodic.length = 2000;
	effect->periodic.delay = 0;

	effect->periodic.period = 2000;
	effect->periodic.magnitude = 65535;
	effect->periodic.offset = 0;
	effect->periodic.phase = 0;

	effect->periodic.attack_length = 0;
	effect->periodic.attack_level = 0;
	effect->periodic.fade_length = 0;
	effect->periodic.fade_level = 0;

	get_periodic_effect_input(effect);

	return SDL_HapticNewEffect(haptic, effect);
}

void modify_sawtoothdown(SDL_Haptic *haptic, int id, SDL_HapticEffect *effect){
	get_periodic_effect_input(effect);
	SDL_HapticUpdateEffect(haptic, id, effect);
}

int create_sawtoothdown(SDL_Haptic *haptic, SDL_HapticEffect *effect){
	effect->type = SDL_HAPTIC_SAWTOOTHDOWN;
	effect->periodic.type = SDL_HAPTIC_SAWTOOTHDOWN;

	effect->periodic.direction.type = SDL_HAPTIC_CARTESIAN;
	effect->periodic.direction.dir[0] = 9000;
	effect->periodic.length = 2000;
	effect->periodic.delay = 0;

	effect->periodic.period = 2000;
	effect->periodic.magnitude = 65535;
	effect->periodic.offset = 0;
	effect->periodic.phase = 0;

	effect->periodic.attack_length = 0;
	effect->periodic.attack_level = 0;
	effect->periodic.fade_length = 0;
	effect->periodic.fade_level = 0;

	get_periodic_effect_input(effect);

	return SDL_HapticNewEffect(haptic, effect);
}

void modify_sawtoothup(SDL_Haptic *haptic, int id, SDL_HapticEffect *effect){
	get_periodic_effect_input(effect);
	SDL_HapticUpdateEffect(haptic, id, effect);
}

int create_sawtoothup(SDL_Haptic *haptic, SDL_HapticEffect *effect){
	effect->type = SDL_HAPTIC_SAWTOOTHUP;
	effect->periodic.type = SDL_HAPTIC_SAWTOOTHUP;

	effect->periodic.direction.type = SDL_HAPTIC_CARTESIAN;
	effect->periodic.direction.dir[0] = 9000;
	effect->periodic.length = 2000;
	effect->periodic.delay = 0;

	effect->periodic.period = 2000;
	effect->periodic.magnitude = 65535;
	effect->periodic.offset = 0;
	effect->periodic.phase = 0;

	effect->periodic.attack_length = 0;
	effect->periodic.attack_level = 0;
	effect->periodic.fade_length = 0;
	effect->periodic.fade_level = 0;

	get_periodic_effect_input(effect);

	return SDL_HapticNewEffect(haptic, effect);
}

void modify_sine(SDL_Haptic *haptic, int id, SDL_HapticEffect *effect){
	get_periodic_effect_input(effect);
	SDL_HapticUpdateEffect(haptic, id, effect);
}

int create_sine(SDL_Haptic *haptic, SDL_HapticEffect *effect){
	effect->type = SDL_HAPTIC_SINE;
	effect->periodic.type = SDL_HAPTIC_SINE;

	effect->periodic.direction.type = SDL_HAPTIC_CARTESIAN;
	effect->periodic.direction.dir[0] = 9000;
	effect->periodic.length = 2000;
	effect->periodic.delay = 0;

	effect->periodic.period = 2000;
	effect->periodic.magnitude = 65535;
	effect->periodic.offset = 0;
	effect->periodic.phase = 0;

	effect->periodic.attack_length = 0;
	effect->periodic.attack_level = 0;
	effect->periodic.fade_length = 0;
	effect->periodic.fade_level = 0;

	get_periodic_effect_input(effect);

	return SDL_HapticNewEffect(haptic, effect);
}

void get_constant_effect_input(SDL_HapticEffect *effect){
	CONSTANT_ATTR(direction.dir[0], 0, 36000);
	CONSTANT_ATTR(length, 0, UINT_MAX);
	SHORT_CONSTANT_ATTR(delay);

	SHORT_CONSTANT_ATTR(level);

	SHORT_CONSTANT_ATTR(attack_length);
	SHORT_CONSTANT_ATTR(attack_level);
	SHORT_CONSTANT_ATTR(fade_length);
	SHORT_CONSTANT_ATTR(fade_level);
}

void modify_constant(SDL_Haptic *haptic, int id, SDL_HapticEffect *effect){
	get_constant_effect_input(effect);
	SDL_HapticUpdateEffect(haptic, id, effect);
}

int create_constant(SDL_Haptic *haptic, SDL_HapticEffect *effect){
	effect->type = SDL_HAPTIC_CONSTANT;
	effect->constant.type = SDL_HAPTIC_CONSTANT;

	effect->constant.direction.type = SDL_HAPTIC_CARTESIAN;
	effect->constant.direction.dir[0] = 9000;
	effect->constant.length = 2000;
	effect->constant.delay = 0;
	effect->constant.level = 32767;

	effect->constant.attack_length = 0;
	effect->constant.attack_level = 0;
	effect->constant.fade_length = 0;
	effect->constant.fade_level = 0;

	get_constant_effect_input(effect);

	return SDL_HapticNewEffect(haptic, effect);
}

void show_create_effect_choices(effect_mask supported_effects){
#define OPTION(x, c) {SDL_HAPTIC_##x, #c ": " #x}

	struct {
		uint16_t type;
		const char *option_str;
	} options[] = {
		OPTION(CONSTANT, c),
		OPTION(SINE, s),
		OPTION(TRIANGLE, t),
		OPTION(SAWTOOTHUP, u),
		OPTION(SAWTOOTHDOWN, d),
		OPTION(RAMP, r),
		OPTION(SPRING, S),
		OPTION(DAMPER, D),
		OPTION(INERTIA, i),
		OPTION(FRICTION, f),
	};

	for(size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i){
		if(options[i].type & supported_effects)
			puts(options[i].option_str);
	}

#undef OPTION
}

choice get_create_effect_choice(effect_mask supported_effects){
#define OPTION(x, c) {CREATE_##x, c}	

	struct {
		choice x;
		char c;
	} options[] = {
		OPTION(CONSTANT, 'c'),
		OPTION(SINE, 's'),
		OPTION(TRIANGLE, 't'),
		OPTION(SAWTOOTHUP, 'u'),
		OPTION(SAWTOOTHDOWN, 'd'),
		OPTION(RAMP, 'r'),
		OPTION(SPRING, 'S'),
		OPTION(DAMPER, 'D'),
		OPTION(INERTIA, 'i'),
		OPTION(FRICTION, 'f'),
	};

	char option = 0;
	scanf("%c", &option);
	discard_line();

	for(size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i){
		if(options[i].c == option)
			return options[i].x;
	}

	return TRY_AGAIN;

#undef OPTION
}

void run_create_effect_choice(SDL_Haptic *haptic, size_t num_elems, haptic_elem elems[], choice c){
	haptic_elem *elem = 0;
	for(size_t i = 0; i < num_elems; ++i){
		if(!elems[i].active){
			elem = &elems[i];
			break;
		}
	}

	int id = 0;
	SDL_HapticEffect *effect = &elem->effect;

	switch(c){
		case CREATE_CONSTANT:
			id = create_constant(haptic, effect);
			break;

		case CREATE_SINE:
			id = create_sine(haptic, effect);
			break;

		case CREATE_TRIANGLE:
			id = create_triangle(haptic, effect);
			break;

		case CREATE_SAWTOOTHUP:
			id = create_sawtoothup(haptic, effect);
			break;

		case CREATE_SAWTOOTHDOWN:
			id = create_sawtoothdown(haptic, effect);
			break;

		case CREATE_RAMP:
			id = create_ramp(haptic, effect);
			break;

		case CREATE_SPRING:
			id = create_spring(haptic, effect);
			break;

		case CREATE_DAMPER:
			id = create_damper(haptic, effect);
			break;

		case CREATE_INERTIA:
			id = create_inertia(haptic, effect);
			break;

		case CREATE_FRICTION:
			id = create_friction(haptic, effect);
			break;
	}

	if(id < 0){
		fputs(SDL_GetError(), stderr);
		elem->active = false;
	} else {
		elem->id = id;
		elem->active = true;
	}
}

void create_effect(SDL_Haptic *haptic, size_t num_elems, haptic_elem elems[], effect_mask supported_effects){
	show_create_effect_choices(supported_effects);

	choice c;
	for(;;){
		c = get_create_effect_choice(supported_effects);

		if(c != TRY_AGAIN)
			break;
		else
			puts("Try again.");
	}

	run_create_effect_choice(haptic, num_elems, elems, c);
}

int get_id(size_t num_elems, haptic_elem elems[]){
	static int id = 0;
	id = get_int("Element ID [%i - %i, current %i]: ",
			0, num_elems, id);

	for(size_t i = 0; i < num_elems; ++i){
		if(elems[i].active && elems[i].id == id)
			return id;
	}

	fprintf(stderr, "Effect with ID %i not found.\n", id);

	return -1;
}

choice get_modify_choice(uint16_t t){
#define CHOICE(x) case SDL_HAPTIC_##x: return MODIFY_##x

	switch(t){
		CHOICE(CONSTANT);
		CHOICE(SINE);
		CHOICE(TRIANGLE);
		CHOICE(SAWTOOTHUP);
		CHOICE(SAWTOOTHDOWN);
		CHOICE(RAMP);
		CHOICE(SPRING);
		CHOICE(DAMPER);
		CHOICE(INERTIA);
		CHOICE(FRICTION);
	}

	return TRY_AGAIN;

#undef CHOICE
}

void modify_effect(SDL_Haptic *haptic, size_t num_elems, haptic_elem elems[]){
	haptic_elem *elem = 0;
	int id = get_id(num_elems, elems);

	if(id < 0)
		return;

	elem = &elems[id];

	SDL_HapticEffect *effect = &elem->effect;

	choice c = get_modify_choice(effect->type);
	switch(c){
		case MODIFY_CONSTANT:
			modify_constant(haptic, id, effect);
			break;

		case MODIFY_SINE:
			modify_sine(haptic, id, effect);
			break;

		case MODIFY_TRIANGLE:
			modify_triangle(haptic, id, effect);
			break;

		case MODIFY_SAWTOOTHUP:
			modify_sawtoothup(haptic, id, effect);
			break;

		case MODIFY_SAWTOOTHDOWN:
			modify_sawtoothdown(haptic, id, effect);
			break;

		case MODIFY_RAMP:
			modify_ramp(haptic, id, effect);
			break;

		case MODIFY_SPRING:
			modify_spring(haptic, id, effect);
			break;

		case MODIFY_DAMPER:
			modify_damper(haptic, id, effect);
			break;

		case MODIFY_INERTIA:
			modify_inertia(haptic, id, effect);
			break;

		case MODIFY_FRICTION:
			modify_friction(haptic, id, effect);
			break;
	}
}

void play_effect(SDL_Haptic *haptic, size_t num_elems, haptic_elem elems[]){
	int id = get_id(num_elems, elems);

	if(id < 0)
		return;

	static uint32_t iterations = 0;
	iterations = get_int("Iterations [%lli - %lli, current %lli]: ",
			0, UINT_MAX, iterations);

	SDL_HapticRunEffect(haptic, id, iterations);
}

void stop_effect(SDL_Haptic *haptic, size_t num_elems, haptic_elem elems[]){
	int id = get_id(num_elems, elems);

	if(id < 0)
		return;

	SDL_HapticStopEffect(haptic, id);
}

void destroy_effect(SDL_Haptic *haptic, size_t num_elems, haptic_elem elems[]){
	int id = get_id(num_elems, elems);

	if(id < 0)
		return;

	SDL_HapticDestroyEffect(haptic, id);
	elems[id].active = false;
}

void set_autocenter(SDL_Haptic *haptic){
	static int autocenter = 0;
	autocenter = get_int("Autocenter [%i - %i, current %i]: ",
			0, 100, autocenter);

	SDL_HapticSetAutocenter(haptic, autocenter);
}

void set_gain(SDL_Haptic *haptic){
	static int gain = 100;
	gain = get_int("Gain [%i - %i, current %i]: ",
			0, 100, gain);

	SDL_HapticSetGain(haptic, gain);
}

void run_choice(SDL_Haptic *haptic, size_t num_elems, haptic_elem elems[], effect_mask supported_effects, choice c){
	switch(c){
		case CREATE_EFFECT:
			create_effect(haptic, num_elems, elems, supported_effects);
			break;

		case MODIFY_EFFECT:
			modify_effect(haptic, num_elems, elems);
			break;

		case PLAY_EFFECT:
			play_effect(haptic, num_elems, elems);
			break;

		case STOP_EFFECT:
			stop_effect(haptic, num_elems, elems);
			break;

		case DESTROY_EFFECT:
			destroy_effect(haptic, num_elems, elems);
			break;

		case SET_AUTOCENTER:
			set_autocenter(haptic);
			break;

		case SET_GAIN:
			set_gain(haptic);
			break;
	}
}

void run(SDL_Haptic *haptic, effect_mask supported_effects){
	bool should_run = true;
	int num_elems = SDL_HapticNumEffects(haptic);

	haptic_elem *elems = (haptic_elem*)calloc(sizeof(haptic_elem), num_elems);
	do {
		show_status(haptic, num_elems, elems);

		show_choices();

		choice c;
		for(;;){
			c = get_choice();

			if(c != TRY_AGAIN)
				break;
			else
				puts("Try again.");
		}

		if(c == QUIT)
			should_run = false;
		else
			run_choice(haptic, num_elems, elems, supported_effects, c);

	} while(should_run);

	free(elems);
}

int main(){
	if(init())
		goto init_err;

	SDL_Haptic *haptic = get_haptic();
	if(!haptic)
		goto haptic_err;

	effect_mask supported_effects = get_supported_effects(haptic);
	run(haptic, supported_effects);

	destroy_haptic(haptic);
haptic_err:
	cleanup();
init_err:
	return 0;
}
