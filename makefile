CXX ?=g++
MAIN = main.cc
TIMER = ./include/timer/lst_timer.cc
HTTP = ./include/http/http_connect.cc
SQL = ./include/mysql/sql_connection_pool.cc
LOG = ./include/log/log.cc
WEBSERVER = ./include/webserver/webserver.cc
CONFIG = ./include/config/config.cc
TAR = server
DEBUG ?= 1
ifeq ($(DEBUG),1)
	CXXFLAGS += -g
else
	CXXFLAGS += -O2
endif

server:$(MAIN) $(TIMER) $(HTTP) $(SQL) $(LOG) $(WEBSERVER) $(CONFIG)
	$(CXX) -o $(TAR) $^ $(CXXFLAGS) -lpthread -lmysqlclient

clean:
	rm -rf $(TAR)