# comp535_pa4

## to compile sender:

gcc sender.c multicast.c -o ../out/sender

## to compile receiver:

gcc receiver.c multicast.c -o ../out/receiver


## Exploring:

### compile sender:
 gcc sender2.c multicast.c -o sender

### compile receiver:
gcc receiver22.c multicast.c -o receiver

## what it does:
- the sender cycliclly sends the files forever
- when a receiver joins the session, it gets the number of files from the first packet 
- it then creates a file buffer when it sees a new file id. 
- it fills the file buffer with data from the packets.
- it keeps track of the num of packets receved and the total needed.
- when it has rescieved all the packets it saves the file
- when it has saved all the files it exits the session

### example run:

1) run: `./sender ../share/descr.txt ../share/illustration.jpeg`
2) in another terminal run: `./receiver`