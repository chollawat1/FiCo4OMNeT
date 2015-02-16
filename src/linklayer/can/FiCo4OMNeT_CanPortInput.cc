//Copyright (c) 2014, CoRE Research Group, Hamburg University of Applied Sciences
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification,
//are permitted provided that the following conditions are met:
//
//1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
//2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
//3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
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

#include "FiCo4OMNeT_CanPortInput.h"

namespace FiCo4OMNeT {

Define_Module(CanPortInput);

void CanPortInput::initialize() {
    bandwidth =
            getParentModule()->getParentModule()->gate("gate$o")->getPathEndGate()->getOwnerModule()->getParentModule()->par(
                    "bandwidth");
    errorperc = getParentModule()->getParentModule()->par("errorperc");

    scheduledDataFrame = NULL;
    scheduledErrorFrame = NULL;

    rcvdDFSignal = registerSignal("receivedCompleteDF");
    rcvdRFSignal = registerSignal("receivedCompleteRF");
    rcvdDFSignalFromNode = registerSignal("receivedCompleteDFFromNode");
    rcvdRFSignalFromNode = registerSignal("receivedCompleteRFFromNode");
    rcvdDFSignalFromGW = registerSignal("receivedCompleteDFFromGW");
    rcvdRFSignalFromGW = registerSignal("receivedCompleteRFFromGW");
    WATCH_MAP(incomingDataFrameIDs);

}

void CanPortInput::handleMessage(cMessage *msg) {
    std::string msgClass = msg->getClassName();
    if (msg->isSelfMessage()) {
        if (ErrorFrame *ef = dynamic_cast<ErrorFrame *>(msg)) {
            forwardOwnErrorFrame(ef);
            scheduledErrorFrame = NULL;

        } else if (CanDataFrame *df = dynamic_cast<CanDataFrame *>(msg)) {
            forwardDataFrame(df);
            scheduledDataFrame = NULL;
        }
    } else if (CanDataFrame *df = dynamic_cast<CanDataFrame *>(msg)) {
//        if (checkExistence(df)) {
            if (checkExistence(df) && !amITheSendingNode()) {
            int rcverr = intuniform(0, 99);
            if (rcverr < errorperc) {
                generateReceiveError(df);
            }
            receiveMessage(df);
        } else {
            if (scheduledDataFrame != NULL) {
                cancelEvent(scheduledDataFrame);
            }
            dropAndDelete(scheduledDataFrame);
            scheduledDataFrame = NULL;
        }
        delete msg;
    } else if (ErrorFrame *ef = dynamic_cast<ErrorFrame *>(msg)) {
        handleExternErrorFrame(ef);
        delete msg;
    }
}

void CanPortInput::receiveMessage(CanDataFrame *df) {
    int frameLength = static_cast<int> (df->getBitLength());
    if (scheduledDataFrame != NULL) {
        cancelEvent(scheduledDataFrame);
    }
    delete (scheduledDataFrame);
    scheduledDataFrame = df->dup();
    scheduleAt((simTime() + calculateScheduleTiming(frameLength)),
            scheduledDataFrame);
}

void CanPortInput::generateReceiveError(CanDataFrame *df) {
    ErrorFrame *errorMsg = new ErrorFrame("receiveError");
    int errorPos = intuniform(0, static_cast<int>(df->getBitLength()) - MAXERRORFRAMESIZE);
    errorMsg->setKind(intuniform(2, 3)); //2: CRC-error, 3: Bit-Stuffing-error
    errorMsg->setCanID(df->getCanID());
    if (errorPos > 0)
        errorPos--;
    errorMsg->setPos(errorPos);
    if (scheduledErrorFrame != NULL) {
        cancelEvent(scheduledErrorFrame);
    }
    delete (scheduledErrorFrame);
    scheduledErrorFrame = errorMsg;
    scheduleAt((simTime() + calculateScheduleTiming(errorPos)), scheduledErrorFrame);
}

bool CanPortInput::checkExistence(CanDataFrame *df) {
    if (df->getRtr()) {
        if (checkIncomingDataFrames(df->getCanID())
                && !checkOutgoingRemoteFrames(df->getCanID())) { // Remote frames will be forwarded to the sink app
            return true;
        }
        return checkOutgoingDataFrames(df->getCanID());
    } else {
        return checkIncomingDataFrames(df->getCanID());
    }
}

bool CanPortInput::checkOutgoingDataFrames(unsigned int id) {
    std::map<unsigned int, cGate*>::iterator it;
    it = outgoingDataFrameIDs.find(id);
    if (it != outgoingDataFrameIDs.end()) {
        return true;
    }
    return false;
}

bool CanPortInput::checkOutgoingRemoteFrames(unsigned int id) {
    for (std::vector<unsigned int>::iterator it = outgoingRemoteFrameIDs.begin();
            it != outgoingRemoteFrameIDs.end(); ++it) {
        if (*it == id) {
            return true;
        }
    }
    return false;
}

bool CanPortInput::checkIncomingDataFrames(unsigned int id) {
//    for (std::vector<int>::iterator it = incomingDataFrameIDs.begin();
//            it != incomingDataFrameIDs.end(); ++it) {
//        if (*it == id) {
//            return true;
//        }
//    }
//    return false;

    std::map<unsigned int, cGate*>::iterator it;
    it = incomingDataFrameIDs.find(id);
    if (it != incomingDataFrameIDs.end()) {
        return true;
    }
    return false;
}

double CanPortInput::calculateScheduleTiming(int length) {
    return static_cast<double> (length) / bandwidth;
}

void CanPortInput::forwardDataFrame(CanDataFrame *df) {
    std::map<unsigned int, cGate*>::iterator it;
    it = incomingDataFrameIDs.find(df->getCanID());
    if (it != incomingDataFrameIDs.end()) {
        emit(rcvdDFSignal, df);
        if (df->getMessageSource() == SOURCE_NODE) {
            emit(rcvdDFSignalFromNode, df);
        } else if (df->getMessageSource() == SOURCE_GW) {
            emit(rcvdDFSignalFromGW, df);
        }
        sendDirect(df, it->second);
    }

    if (df->getRtr()) {
        std::map<unsigned int, cGate*>::iterator it2;
        it2 = outgoingDataFrameIDs.find(df->getCanID());
        if (it2 != outgoingDataFrameIDs.end()) {
            emit(rcvdRFSignal, df);
            if (df->getMessageSource() == SOURCE_NODE) {
                emit(rcvdRFSignalFromNode, df);
            } else if (df->getMessageSource() == SOURCE_GW) {
                emit(rcvdRFSignalFromGW, df);
            }
            sendDirect(df, it2->second);
        }
    }
}

void CanPortInput::forwardOwnErrorFrame(ErrorFrame *ef) {
    cModule* portOutput = getParentModule()->getSubmodule("canPortOutput");
    sendDirect(ef, portOutput, "directIn");
}

void CanPortInput::handleExternErrorFrame(ErrorFrame *ef) {
    CanPortOutput* portOutput =
            dynamic_cast<CanPortOutput*> (getParentModule()->getSubmodule("canPortOutput"));
    portOutput->sendingCompleted();

    if ((checkOutgoingDataFrames(ef->getCanID())
            || checkOutgoingRemoteFrames(ef->getCanID()))) {
        if (ef->getKind() > 2) {
            portOutput->handleReceivedErrorFrame();
        }
    }

    if (scheduledDataFrame != NULL && scheduledDataFrame->isScheduled()

    && (ef->getCanID() == scheduledDataFrame->getCanID())) {
        cancelAndDelete(scheduledDataFrame);
        scheduledDataFrame = NULL;
    }

    if (scheduledErrorFrame != NULL && scheduledErrorFrame->isScheduled()
            && ef->getCanID() == scheduledErrorFrame->getCanID()) {
        cancelAndDelete(scheduledErrorFrame);
        scheduledErrorFrame = NULL;
    }
}

void CanPortInput::registerOutgoingDataFrame(unsigned int canID, cGate* outGate) {
    outgoingDataFrameIDs.insert(std::pair<unsigned int, cGate*>(canID, outGate));
}

void CanPortInput::registerOutgoingRemoteFrame(unsigned int canID) {
    std::vector<unsigned int>::iterator it;
    it = outgoingRemoteFrameIDs.begin();
    it = outgoingRemoteFrameIDs.insert(it, canID);
}

void CanPortInput::registerIncomingDataFrame(unsigned int canID, cGate* inGate) {
//    std::vector<int>::iterator it;
//    it = incomingDataFrameIDs.begin();
//    it = incomingDataFrameIDs.insert(it, canID);
    incomingDataFrameIDs.insert(std::pair<int, cGate*>(canID, inGate));
}

bool CanPortInput::amITheSendingNode(){
    CanOutputBuffer* outputBuffer =
                dynamic_cast<CanOutputBuffer*> (getParentModule()->getParentModule()->getSubmodule("bufferOut"));
    return (outputBuffer->getCurrentFrame() != NULL);
}

}
