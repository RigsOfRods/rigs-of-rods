#pragma once

#include "../../source/main/gameplay/Replay.h"

#ifdef ROR_FAKES_IMPL
    Replay::Replay(Actor* b, int nframes) {}
    Replay::~Replay(void) {}
    void Replay::setHidden(bool) {}
    void * Replay::getReadBuffer(int,int,unsigned long &) {return nullptr;}
    void Replay::setVisible(bool) {}
    void Replay::writeDone(void) {}
    void * Replay::getWriteBuffer(int) {return nullptr;}
#endif // ROR_FAKES_IMPL
