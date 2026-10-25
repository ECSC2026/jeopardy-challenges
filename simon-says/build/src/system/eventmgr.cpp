
#include "system/eventmgr.h"

#include <SDL2/SDL.h>

#include <cstring>


Key map_key(SDL_Keysym key) {
	switch (key.sym) {
		case SDLK_SPACE: return Key_Space;
		case SDLK_RETURN: return Key_Enter;
		case SDLK_ESCAPE: return Key_Escape;
		
		case SDLK_UP: return Key_Up;
		case SDLK_DOWN: return Key_Down;
		case SDLK_LEFT: return Key_Left;
		case SDLK_RIGHT: return Key_Right;

		case SDLK_0: return Key_0;
		case SDLK_1: return Key_1;
		case SDLK_2: return Key_2;
		case SDLK_3: return Key_3;
		case SDLK_4: return Key_4;
		case SDLK_5: return Key_5;
		case SDLK_6: return Key_6;
		case SDLK_7: return Key_7;
		case SDLK_8: return Key_8;
		case SDLK_9: return Key_9;
		
		case SDLK_a: return Key_a;
		case SDLK_b: return Key_b;
		case SDLK_c: return Key_c;
		case SDLK_d: return Key_d;
		case SDLK_e: return Key_e;
		case SDLK_f: return Key_f;
		case SDLK_g: return Key_g;
		case SDLK_h: return Key_h;
		case SDLK_i: return Key_i;
		case SDLK_j: return Key_j;
		case SDLK_k: return Key_k;
		case SDLK_l: return Key_l;
		case SDLK_m: return Key_m;
		case SDLK_n: return Key_n;
		case SDLK_o: return Key_o;
		case SDLK_p: return Key_p;
		case SDLK_q: return Key_q;
		case SDLK_r: return Key_r;
		case SDLK_s: return Key_s;
		case SDLK_t: return Key_t;
		case SDLK_u: return Key_u;
		case SDLK_v: return Key_v;
		case SDLK_w: return Key_w;
		case SDLK_x: return Key_x;
		case SDLK_y: return Key_y;
		case SDLK_z: return Key_z;
	}
	return Key_Invalid;
}


EventMgr::EventMgr() {
	memset(pressed, 0, sizeof(pressed));
	memset(hold, 0, sizeof(hold));
	memset(released, 0, sizeof(released));
}

bool EventMgr::update() {
	resized = false;

	memset(pressed, 0, sizeof(pressed));
	memset(released, 0, sizeof(released));

	SDL_Event e;
	while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return false;
        }
		else if (e.type == SDL_KEYDOWN) {
			if (!e.key.repeat) {
				Key key = map_key(e.key.keysym);
				pressed[key] = true;
				hold[key] = true;
			}
		}
		else if (e.type == SDL_KEYUP) {
			if (!e.key.repeat) {
				Key key = map_key(e.key.keysym);
				released[key] = true;
				hold[key] = false;
			}
		}
		else if (e.type == SDL_WINDOWEVENT) {
			if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				resized = true;
				resize_width = e.window.data1;
				resize_height = e.window.data2;
			}
		}
	}
    return true;
}
