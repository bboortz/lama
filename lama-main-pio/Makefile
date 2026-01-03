timestamp = $(shell date +'%Y-%m-%d-%H-%M-%S')
branch = $(shell git rev-parse --abbrev-ref HEAD)
commitid = $(shell git rev-parse HEAD)
tag = $(branch)-$(commitid)
image = registry.gitlab.com/devara.world/health-app:$(tag)
image_latest = registry.gitlab.com/devara.world/health-app:latest
curdir = $(shell pwd)
export PROFILE ?= dev

help:               ## printing out the help
	@echo
	@echo *surfer*
	@echo
	@echo "--- TARGETS ---"
	@grep -F -h "##" $(MAKEFILE_LIST) | grep -v -E grep | sed -e 's/\\$$//' | sed -e 's/##//'
	@echo "------"
	@echo

clean:              ## clean up this project
	./scripts/clean.sh
c: clean

fmt:                ## format the sources
	./scripts/fmt.sh
f: fmt

build:              ## build the software
	./scripts/build.sh
b: build

test: fmt build     ## test the software
	./scripts/test.sh
t: test


upload: fmt build   ## upload the firmware
	./scripts/upload.sh
u: upload

monitor:            ## monitor the device
	./scripts/monitor.sh
m: monitor


doc:                ## create the documentation
	echo "not implemented"



