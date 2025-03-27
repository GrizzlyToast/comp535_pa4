//Receiver setup
multicast_setup_recv(m);
if (multicast_check_receive(m) > 0) {
    multicast_receive(m, buffer, buffer_size);
}