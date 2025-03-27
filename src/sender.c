// Sender initiailization
#include "multicast.h"

mcast_t *m = multicast_init("239.0.0.1", 5000, 5000);
multicast_send(m, chunk_data, chunk_size);

