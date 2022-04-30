#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PORT=/dev/ttyUSB0
BUADRATE=921600
BUILD=docker
PROJECT_NAME := motohs
LOCAL_USER=$(id -u ${USER}):$(id -g ${USER})

SRC_DIR=${PWD}
IMG_NAME="motohs_firmware"
CONTAINER_NAME="motohs_firmware_work"

DOCKER_RUN=docker run \
		--rm \
		--name ${CONTAINER_NAME} \
		-v ${SRC_DIR}:${SRC_DIR}:rw \
		-u $$(id -u):$$(id -g) \
		-e IDF_PATH=${SRC_DIR}/esp-idf/ \
		-e CCACHE_DIR=${SRC_DIR}/.ccache \
		--group-add dialout	\
		-w "${SRC_DIR}"

# To build with local toolchain, pass BUILD=local to make
ifeq ($(BUILD), local)
include $(IDF_PATH)/make/project.mk
else

all:            ## Build firmware
	${DOCKER_RUN} ${IMG_NAME} idf.py build

ota:
	python3 tools/imgtool.py  sign \
                              --header-size 0x20 \
                              --align 4 \
                              --slot-size 0x200000 \
                              --version 1.0.0 \
                              --pad-header \
                              build/motohs.bin build/motohs_signed.bin

clean:          ## Clean build directory
	${DOCKER_RUN} ${IMG_NAME} idf.py fullclean
	rm -rf ./.ccache

menuconfig:     ## Start SDK configuration GUI
	${DOCKER_RUN} -it ${IMG_NAME} idf.py menuconfig
	
flash:          ## Specify PORT=/dev/ttyUSB* device. Default is /dev/ttyUSB0
	${DOCKER_RUN} --device=${PORT} ${IMG_NAME} idf.py -p $(PORT) -b $(BUADRATE) flash
	
monitor:        ## Start serial terminal from ESP-IDF with debug capabilities
	${DOCKER_RUN} --device=${PORT} -it ${IMG_NAME} idf.py monitor

size-components:## Analyze components size
	${DOCKER_RUN} -it ${IMG_NAME} idf.py size-components

erase:		## Erase specified partition
	${DOCKER_RUN} --device=${PORT} ${IMG_NAME} parttool.py --port $(PORT) --baud $(BUADRATE) erase_partition --partition-name=$(PARTITION)

erase-all:	## Erase whole chip
	${DOCKER_RUN} --device=${PORT} ${IMG_NAME} esptool.py --port $(PORT) --baud $(BUADRATE) erase_flash

partition-write:## Write specified partition
	${DOCKER_RUN} --device=${PORT} ${IMG_NAME} parttool.py --port $(PORT) --baud $(BUADRATE) write_partition --partition-name=$(PARTITION) --input "$(FILE)"

partition-gen:	## Generate settings partition
	${DOCKER_RUN} ${IMG_NAME} python esp-idf/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py  generate $(IN) $(OUT) $(SIZE)

write-files:	## Write Ublox and gas gause firmware files to flash storage
	${DOCKER_RUN} --device=${PORT} ${IMG_NAME} parttool.py --port $(PORT) --baud $(BUADRATE) write_partition --partition-name=files --input ./build/files.bin

docker-init:    ## Initialize docker image
	docker build -t $(IMG_NAME) .
	pip3 install --user -r tools/requirements.txt
	
docker-rm:      ## Remove docker image
	docker image rm ${IMG_NAME}
	
help:           ## Show this help.
	@fgrep -h "##" $(MAKEFILE_LIST) | fgrep -v fgrep | sed -e 's/\\$$//' | sed -e 's/##//'
	
endif
