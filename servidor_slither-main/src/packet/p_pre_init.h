#ifndef SRC_PACKET_P_PRE_INIT_H_
#define SRC_PACKET_P_PRE_INIT_H_

#include "packet/p_base.h"
#include <string>

struct packet_pre_init : public PacketBase {
    // '6' is the Pre-init response type
    packet_pre_init() : PacketBase(static_cast<out_packet_t>('6')) {
        // Construct the payload.
        // The client expects the raw secret string immediately after the header.
        // Header is handled by PacketBase (ClientTime + '6').
        // We simply append the data.
        
        payload = "dakrtywcilopu"
                  "hgrmzwsdolitualk"
                  "srrarjsrzyjhrnzv"
                  "fdfkrsyahjvuobhj"
                  "kmzwvgoppxaagiwv"
                  "scjlqoualghnuvde"
                  "dozuwcdjosrcnhjp"
                  "rwlkfqbyegkorwte"
                  "pmlstcfhksxakilr"
                  "uwdhhouwdchnsqec"
                  "ngvqpcz";
    }

    std::string payload;

    size_t get_size() const noexcept { return 3 + payload.size(); }
};

inline std::ostream& operator<<(std::ostream& out, const packet_pre_init& p) {
    out << static_cast<PacketBase>(p);
    // Write raw payload string bytes. 
    // Do NOT use write_string() as it adds a length byte which breaks this specific handshake.
    out << p.payload;
    return out;
}

#endif // SRC_PACKET_P_PRE_INIT_H_