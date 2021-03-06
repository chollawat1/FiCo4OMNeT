//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "fico4omnet/applications/flexray/sink/FRTrafficSinkAppBase.h"

#include "fico4omnet/buffer/flexray/FRInputBuffer.h"

//Auto-generated messages
#include "fico4omnet/linklayer/flexray/messages/FRFrame_m.h"

namespace FiCo4OMNeT {

Define_Module(FRTrafficSinkAppBase);

FRTrafficSinkAppBase::FRTrafficSinkAppBase(){
    this->bufferMessageCounter = 0;
    this->idle = true;
    this->currentFrameID = 0;
}

FRTrafficSinkAppBase::~FRTrafficSinkAppBase(){

}

void FRTrafficSinkAppBase::handleMessage(cMessage *msg) {
    if (msg->arrivedOn("controllerIn")) {
        bufferMessageCounter++;
        if (idle) {
            requestFrame();
        }
    } else if (FRFrame * frame = dynamic_cast<FRFrame *>(msg)) {
        int i = frame->getFrameID();
        currentFrameID = i;
        bufferMessageCounter--;
        startWorkOnFrame(0); //TODO working time
    } else if (msg->isSelfMessage()) {
        FRInputBuffer *buffer =
                dynamic_cast<FRInputBuffer*> (getParentModule()->getSubmodule("inputBuffer"));
        buffer->deleteFrame(currentFrameID);
        if (bufferMessageCounter > 0) {
            requestFrame();
        } else {
            idle = true;
        }
    }
    delete msg;
}

void FRTrafficSinkAppBase::requestFrame() {
    FRInputBuffer *buffer = dynamic_cast<FRInputBuffer*> (getParentModule()->getSubmodule(
            "inputBuffer"));
    buffer->deliverNextFrame();
    idle = false;
}

void FRTrafficSinkAppBase::startWorkOnFrame(double workTime) {
    cMessage *msg = new cMessage("workFinished");
    scheduleAt(simTime() + workTime, msg);
}

}
