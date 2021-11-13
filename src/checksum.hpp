#ifndef CHECKSUM_H_
#define CHECKSUM_H_

#include "packet.hpp"

// checksumを検証し、もし違っていたら置き換える
void recaluculate_checksum(Packet *pkt);

#endif
