# comp535_pa4

## to compile sender:

gcc sender.c multicast.c -o ../out/sender

## to compile receiver:

gcc receiver.c multicast.c -o ../out/receiver

## TODOS

- make a thread to listen for NACKS and re-transmit the NACKS
-
