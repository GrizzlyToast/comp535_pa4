# comp535_pa4

## to compile sender:

gcc sender.c multicast.c -o ./sender

## to compile receiver:

gcc receiver.c multicast.c -o ./receiver

## Usage

1) start the distribution service:
`./sender <file 1> <file 2> ...`
example: `./sender ../share/descr.txt ../share/illustration.jpeg`

2) join receivers to the service:
`./receiver <ID>`
example: `./receiver 1`

## How it works

- The distribution service will detects if clients join the service
- If there are no active clients for 30s the service ends
- When a client joins, it stays until it has downloaded all the files, then it quits.
