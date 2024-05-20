BUILD_TARGET=./build

CMAKE_TARGET=cmake -S . -B ${BUILD_TARGET} 
CMAKE_EXE=${CMAKE_TARGET} -DBUILD_EXE=ON
CMAKE_WIIUSE=${CMAKE_TARGET} -DBUILD_WII_USE=ON -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLE_SDL=NO


MAKE_TARGET=make -C ${BUILD_TARGET}

EXE_TARGET=wii-controller-c

TEENSY_TARGET=./teensy

CMAKE_CLEAN_ALL=rm -rf ${BUILD_TARGET} ${EXE_TARGET}

INSTALL_PREFIX=/usr/bin

SYSTEMD_SERVICE_DIR=./scripts

INSTALL_SYSTEMD_UNIT=wii-controller.service

all :: 
	@echo "Making project"
	@${CMAKE_TARGET}
	@${MAKE_TARGET}
	@mv ${BUILD_TARGET}/${EXE_TARGET} .

exe ::
	@echo "Making project and executable"
	@${CMAKE_EXE}
	@${MAKE_TARGET}
	@mv ${BUILD_TARGET}/${EXE_TARGET} . 

wiiuse ::
	@${CMAKE_CLEAN_ALL}
	@echo "Making wiiuse"
	@${CMAKE_WIIUSE}
	@${MAKE_TARGET}
	@sudo ${MAKE_TARGET} install
	

install-teensy-firmware :: 
	@platformio platforms install teensy

ubuntu-prep ::
	@sudo apt install platformio libbluetooth-dev

arch-prep ::
	@yay bluez-utils platformio

clean ::
	@echo "Removing build and run"
	@${CMAKE_CLEAN_ALL}

format ::
	@find ./src -iname "*.cpp" -o -iname "*.hpp" -o -iname "*.h" -o -iname "*.cc" -o -iname "*.c" | xargs clang-format -i -sort-includes=false -style=file
	@find ./include -iname "*.cpp" -o -iname "*.hpp" -o -iname "*.h" -o -iname "*.cc" -o -iname "*.c" | xargs clang-format -i -sort-includes=false -style=file

verify-teensy ::
	platformio run -d ${TEENSY_TARGET}

upload-teensy ::
	platformio run -t upload -d ${TEENSY_TARGET}

install :: all
	@sudo cp ${EXE_TARGET} ${INSTALL_PREFIX}
	@sudo cp ${SYSTEMD_SERVICE_DIR}/${INSTALL_SYSTEMD_UNIT} /etc/systemd/system

uninstall ::
	@sudo rm -rf ${INSTALL_PREFIX}/${EXE_TARGET}
	@sudo rm ${SYSTEMD_SERVICE_DIR}/${INSTALL_SYSTEMD_UNIT} /etc/systemd/system/${INSTALL_SYSTEMD_UNIT}
