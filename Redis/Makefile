#set environment variable RM_INCLUDE_DIR to the location of redismodule.h
ifndef RM_INCLUDE_DIR
	RM_INCLUDE_DIR=./
endif

ifndef SRC_DIR
	SRC_DIR=src
endif

ifndef MODULE_NAME
	MODULE_NAME=RedisSketches
endif


all:
	$(MAKE) -C ./$(SRC_DIR)
	cp ./$(SRC_DIR)/$(MODULE_NAME).so .

clean: FORCE
	rm -rf *.xo *.so *.o
	rm -rf ./$(SRC_DIR)/*.xo ./$(SRC_DIR)/*.so ./$(SRC_DIR)/*.o
	rm -rf ./$(SRC_DIR)/*/*.xo ./$(SRC_DIR)/*/*.so ./$(SRC_DIR)/*/*.o

run:
	redis-server --loadmodule ./$(MODULE_NAME).so --logfile ./redis-server.log

FORCE:
