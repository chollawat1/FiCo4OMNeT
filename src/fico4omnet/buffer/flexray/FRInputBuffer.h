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

#ifndef FICO4OMNET_FRINPUTBUFFER_H_
#define FICO4OMNET_FRINPUTBUFFER_H_

//FiCo4OMNeT
#include "fico4omnet/base/FiCo4OMNeT_Defs.h"
#include "fico4omnet/buffer/flexray/FRBuffer.h"

namespace FiCo4OMNeT {

/**
 * @brief This buffer holds messages which were received by this node.
 *
 * @ingroup Buffer
 *
 * @author Stefan Buschmann
 */
class FRInputBuffer :public FRBuffer{

public:
    /**
     * @brief Puts the frame into the collection and informs the connected gates about the receiption.
     *
     * @param msg The DataFrame to put in the buffer.
     *
     */
    virtual void putFrame(cMessage* msg);
};

}
#endif /* FRINPUTBUFFER_H_ */
