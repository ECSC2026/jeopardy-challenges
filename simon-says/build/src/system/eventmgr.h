
#pragma once


enum Key {
	Key_Invalid,
	
	Key_Space,
	Key_Enter,
	Key_Escape,
	
	Key_Up,
	Key_Down,
	Key_Left,
	Key_Right,

	Key_0,
	Key_1,
	Key_2,
	Key_3,
	Key_4,
	Key_5,
	Key_6,
	Key_7,
	Key_8,
	Key_9,
	
	Key_a,
	Key_b,
	Key_c,
	Key_d,
	Key_e,
	Key_f,
	Key_g,
	Key_h,
	Key_i,
	Key_j,
	Key_k,
	Key_l,
	Key_m,
	Key_n,
	Key_o,
	Key_p,
	Key_q,
	Key_r,
	Key_s,
	Key_t,
	Key_u,
	Key_v,
	Key_w,
	Key_x,
	Key_y,
	Key_z,

	Key_Last
};


class EventMgr {
public:
	EventMgr();
	
	bool update();

	bool pressed[Key_Last];
	bool hold[Key_Last];
	bool released[Key_Last];
	
	bool resized;
	int resize_width;
	int resize_height;
};
