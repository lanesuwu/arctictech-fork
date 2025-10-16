#pragma once

#include "../../SDK/Misc/Vector.h"

class CUserCmd;

class CAutoPeek {
	int block_buttons = 0;
	bool returning = false;
	bool prev_state = false;
	Vector position;
	float anim = 0.f;
public:

	void CreateMove();
	void Draw();
	void Return();
	void Disable() { returning = false; };
	bool IsReturning() { return returning; };
};

extern CAutoPeek* AutoPeek;