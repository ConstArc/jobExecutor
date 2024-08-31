#!/bin/bash

#=========================================================#

# Change if needed to

SERVER_NAME=linux13.di.uoa.gr
PORT_NUMBER=8647
BUFFER_SIZE=8
THREAD_POOL_SIZE=4

# Change this to "val" if you want to check for 
# leaks on the server, else keep it "simple"
MODE=simple

#=========================================================#

# Run jobExecutorServer with valgrind if mode is "val"
if [ "$MODE" == "val" ]; then
    valgrind --leak-check=full --show-leak-kinds=all --log-file=./valgrind/ServerValgrind.log \
        ./bin/jobExecutorServer "$PORT_NUMBER" "$BUFFER_SIZE" "$THREAD_POOL_SIZE" 2> ./errors/ServerErrors.log &
else
    ./bin/jobExecutorServer "$PORT_NUMBER" "$BUFFER_SIZE" "$THREAD_POOL_SIZE" 2> ./errors/ServerErrors.log &
fi

sleep 2

./bin/jobCommander "$SERVER_NAME" "$PORT_NUMBER" poll 2>> ./errors/CommandersErrors.log &

sleep 2

# Terminate server
./bin/jobCommander "$SERVER_NAME" "$PORT_NUMBER" exit 2>> ./errors/CommandersErrors.log

sleep 1