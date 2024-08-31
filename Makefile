#=========================================================#

# If going for a quick run, these parameters must be set

# portNum
PORT = 6544

# serverName
SRVR_NAME = linux11.di.uoa.gr

# bufferSize
BUFF_SIZE = 8

# threadPoolSize
THREAD_POOL = 4

#=========================================================#


# Compiler and compilation flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -pthread -I$(INC_DIR)
LDFLAGS = -lpthread

ifdef DEBUG
CFLAGS += -DDEBUG
endif

# Directories
SRC_DIR   = ./src
INC_DIR   = ./include
BIN_DIR   = ./bin
BUILD_DIR = ./build
TEST_DIR  = ./tests
OUTPUT_DIR = ./output
ERRORS_DIR = ./errors
VAL_DIR = ./valgrind

EXECS = jobCommander jobExecutorServer
OBJS_SERVER = processUtils parsingUtils jobQueue controller worker
OBJS_JOB_CMDR = processUtils parsingUtils jobQueue

.PHONY: all clean $(BIN_DIR) $(BUILD_DIR) $(OUTPUT_DIR) $(ERRORS_DIR) $(VAL_DIR)

all: $(BIN_DIR) $(BUILD_DIR) $(OUTPUT_DIR) $(ERRORS_DIR) $(VAL_DIR) $(EXECS:%=$(BIN_DIR)/%)

$(BIN_DIR)/jobExecutorServer: $(BUILD_DIR)/jobExecutorServer.o $(OBJS_SERVER:%=$(BUILD_DIR)/%.o)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

$(BIN_DIR)/jobCommander: $(BUILD_DIR)/jobCommander.o $(OBJS_JOB_CMDR:%=$(BUILD_DIR)/%.o)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILD_DIR)/controller.o: $(SRC_DIR)/controller.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

$(BUILD_DIR)/worker.o: $(SRC_DIR)/worker.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(ERRORS_DIR):
	mkdir -p $(ERRORS_DIR)

$(VAL_DIR):
	mkdir -p $(VAL_DIR)

#=========================================================#

# For quick but restricted run

# Initiate server
run: all
	./bin/jobExecutorServer $(PORT) $(BUFF_SIZE) $(THREAD_POOL) 2> $(ERRORS_DIR)/ServerErrors.log &

# Initiate server with valgrind
val: all
	valgrind --leak-check=full --show-leak-kinds=all --log-file=$(VAL_DIR)/ServerValgrind.log \
		./bin/jobExecutorServer $(PORT) $(BUFF_SIZE) $(THREAD_POOL) 2> $(ERRORS_DIR)/ServerErrors.log &

# 'progDelay 5' commander
cmdr_prog: all
	./bin/jobCommander $(SRVR_NAME) $(PORT) issueJob ./tests/progDelay 5 2>> $(ERRORS_DIR)/CommandersErrors.log

# 'ls -l' commander
cmdr_ls: all
	./bin/jobCommander $(SRVR_NAME) $(PORT) issueJob ls -l 2>> $(ERRORS_DIR)/CommandersErrors.log

# 'exit' commander
cmdr_exit: all
	./bin/jobCommander $(SRVR_NAME) $(PORT) exit 2>> $(ERRORS_DIR)/CommandersErrors.log

#=========================================================#

# For versatile testing/run, but make sure to set up the
# appropriate parameters on the test_jobExecutor_*.sh files

# Run test
rtest: all
	$(TEST_DIR)/$(TEST) || true

# Allows for setting DEBUG=1 upon running tests, for debugging
rtest_debug: all
	TEST_DIR=$(TEST_DIR) TEST=$(TEST) $(MAKE) -e rtest

#=========================================================#

clean:
	rm -f $(BIN_DIR)/*
	rm -f $(BUILD_DIR)/*

deep_clean:
	rm -f $(BIN_DIR)/*
	rm -f $(BUILD_DIR)/*
	rm -rf $(ERRORS_DIR)
	rm -rf $(VAL_DIR)
	rm -rf $(OUTPUT_DIR)