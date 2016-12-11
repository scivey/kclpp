deps:
	./scripts/deps.sh build

clean-deps:
	./scripts/deps.sh clean

clean:
	rm -rf build
	rm -f *.log

base: deps
	mkdir -p build && cd build && cmake ../

create-runner: base
	cd build && make runner -j8

run: create-runner
	./build/runner

create-tests: base
	cd build && make unit_test_runner func_test_runner -j8

test: create-tests
	./build/unit_test_runner
	./build/func_test_runner

.PHONY: run create-runner test create-tests
