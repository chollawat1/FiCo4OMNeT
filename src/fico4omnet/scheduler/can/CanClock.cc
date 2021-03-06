//Copyright (c) 2014, CoRE Research Group, Hamburg University of Applied Sciences
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification,
//are permitted provided that the following conditions are met:
//
//1. Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
//2. Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
//3. Neither the name of the copyright holder nor the names of its contributors
//   may be used to endorse or promote products derived from this software without
//   specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
//ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "fico4omnet/scheduler/can/CanClock.h"

namespace FiCo4OMNeT {

Define_Module(CanClock);

CanClock::CanClock(){
    this->clockDriftSignal = 0;
    this->currentDrift = 0;
    this->maxDrift = 0;
    this->maxDriftChange = 0;
    this->randomStartDrift = true;
    this->lastDriftUpdate = 0;
}

void CanClock::initialize() {
    clockDriftSignal = registerSignal("clockDrift");
    maxDrift = par("maxDrift");
    maxDriftChange = par("maxDriftChange");
    lastDriftUpdate = simTime();
    calculateInitialDrift();
}

void CanClock::calculateInitialDrift(){
    randomStartDrift = par("randomStartDrift").boolValue();
    if (randomStartDrift) {
        currentDrift = uniform((-maxDrift), maxDrift);
    }
}

void CanClock::calculateNewDrift(){
    simtime_t timeSinceLastUpdate = simTime() - lastDriftUpdate;
    lastDriftUpdate = simTime();
    double maxDriftChangeSinceLastUpdate = maxDriftChange * timeSinceLastUpdate.dbl();
    double newDriftChange = uniform(-maxDriftChangeSinceLastUpdate, maxDriftChangeSinceLastUpdate);
    double newDrift = currentDrift + newDriftChange;
    if (newDrift > maxDrift)
        currentDrift = maxDrift;
    else if (newDrift < -maxDrift)
        currentDrift = -maxDrift;
    else
        currentDrift = newDrift;
    emit(clockDriftSignal,currentDrift);
}

double CanClock::getCurrentDrift(){
    calculateNewDrift();
    return currentDrift;
}

}
