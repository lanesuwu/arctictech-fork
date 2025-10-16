#pragma once

class CPingReducer {
	struct globals_t {
		float curtime{ };
		float frametime{ };
		int tickcount{ };
		int cs_tickcount{};

		void read();
		void write();
	};

	globals_t shared_data;
	globals_t backup_data;

public:
	bool AllowReadPackets();
	void ReadPackets(bool final_tick);
};

extern CPingReducer* PingReducer;