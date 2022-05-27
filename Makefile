CXXFLAGS = -Ilib/include -Wall -Wextra -std=c++23
LDFLAGS = -Llib

build_all: clean build_aes build_test build_debug build_profile build_release build_speed_test

build_aes:
	@docker-compose exec aes mkdir -p ./lib/include
	docker-compose exec aes g++ $(CXXFLAGS) -O3 -static -c src/aes.cpp -o lib/aes.o
	docker-compose exec aes g++ $(CXXFLAGS) -g -static -c src/aes.cpp -o lib/aes_debug.o
	docker-compose exec aes ar rcs lib/libaes.a lib/aes.o
	docker-compose exec aes ar rcs lib/libaes_debug.a lib/aes_debug.o
	@docker-compose exec aes rm lib/aes.o lib/aes_debug.o
	docker-compose exec aes cp src/aes.h lib/include/aes.h

build_test: build_aes
	@docker-compose exec aes mkdir -p ./bin
	docker-compose exec aes g++ $(CXXFLAGS) -g -pthread tests/tests.cpp $(LDFLAGS) -lgtest -laes_debug -o bin/test

build_profile: build_aes
	@docker-compose exec aes mkdir -p ./bin
	docker-compose exec aes g++ $(CXXFLAGS) -pg -pthread cmd/main.cpp $(LDFLAGS) -laes_debug -lboost_system -lboost_coroutine -o bin/profile

build_debug: build_aes
	@docker-compose exec aes mkdir -p ./bin
	docker-compose exec aes g++ $(CXXFLAGS) -g -pthread cmd/main.cpp $(LDFLAGS) -laes_debug -lboost_system -lboost_coroutine -o bin/debug

build_speed_test: build_aes
	@docker-compose exec aes mkdir -p ./bin
	docker-compose exec aes g++ $(CXXFLAGS) -O3 -pthread src/aes.cpp speedtest/main.cpp $(LDFLAGS) -lboost_system -lboost_coroutine -lbenchmark -o bin/speedtest

build_release: build_aes
	@docker-compose exec aes mkdir -p ./bin
	docker-compose exec aes g++ $(CXXFLAGS) -O3 -pthread cmd/main.cpp $(LDFLAGS) -laes -lboost_system -lboost_coroutine -o bin/release

test:
	docker-compose exec aes bin/test

debug:
	docker-compose exec aes bin/debug

profile:
	docker-compose exec aes bin/profile

release:
	docker-compose exec aes bin/release

speed_test:
	docker-compose exec aes bin/speedtest

style_fix:
	docker-compose exec aes bash -c "clang-format -i src/*.cpp src/*.h tests/*.cpp speedtest/*.cpp cmd/*.cpp"

clean:
	docker-compose exec aes rm -rf bin/* lib/*

workflow_build_test:
	g++ $(FLAGS) -g -pthread ./src/AES.cpp ./tests/tests.cpp /usr/lib/libgtest.a -o bin/test

workflow_build_speed_test:
	g++ $(FLAGS) -O2 ./src/AES.cpp ./speedtest/main.cpp -o bin/speedtest	
