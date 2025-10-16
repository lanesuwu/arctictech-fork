#pragma once

#include <deque>


class CPingSpike {
    struct IncSequence {
        int in_reliable;
        int out_reliable;
        int sequence;
        float time;
    };

    std::deque<IncSequence> sequences;
    int lastincomingsequencenumber;
public:
    void OnSendDatagram();
    void OnPacketStart();
    void LevelInit();
};

extern CPingSpike* PingSpike;