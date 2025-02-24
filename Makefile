src=$(wildcard net/*.cc util/vvlog/*.cc)

INCLUDE_DIRS = ./net \
				./ \
				./codec \
				./util
CXXFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
	
client:
	g++ $(src) client.cc -std=c++17 -pthread -g -I./net -o client

.PHONY: clean
clean:
	rm server && rm client

server:
	g++ -std=c++17 -pthread -g -I./application -I./route -I./net -I./util \
	$(src) \
	main.cc \
	$(CXXFLAGS) \
	-o server