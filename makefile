CXX ?=g++
DEBUG ?= 1
ifeq ($(DEBUG),1)
	CXXFLAGS += -g
else
	CXXFLAGS += -O2
endif

server: main.cpp ./timer/lst_timer.cc ./http/http_connect.cc ./log/log.cc ./CGImysql/sql_connection_pool.cc ./webserver.cc config.cc
	$(CXX) -o server $^$(CXXFLAGS) -lpthread -lmysqlclient

clean:
	rm -r server