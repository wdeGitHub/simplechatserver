CXX = g++
CXXFLAGS = -std=c++11 -I. -Ithread -Isql -Irequest -Ijson -I/usr/include/mysql 
LDFLAGS = -L/usr/lib64/mysql
LDLIBS = -lmysqlclient -lpthread

SRC = main.cpp \
      simplechat.cpp \
      request/tcp_conn.cpp \
      sql/sqlconnectionpool.cpp

OBJ = $(SRC:.cpp=.o)

simplechat: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f simplechat $(OBJ)