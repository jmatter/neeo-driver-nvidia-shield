# Settings
PORT_DIR = ../assets/btstack/port/libusb
MONGOOSE_DIR = ../assets/mongoose

.DEFAULT_GOAL := neeo-shield-driver

# Include correct Makefile for the required settings
include ${PORT_DIR}/Makefile

# Add the 'port' directory for searching missing stuff
VPATH+= ${PORT_DIR}
CFLAGS+= -I${PORT_DIR}
CFLAGS+= -I${MONGOOSE_DIR}
CFLAGS+= -DMG_ENABLE_FILESYSTEM=1

# Setup sources for dependencies
SOURCES = ${CORE_OBJ} ${COMMON_OBJ} ${ATT_OBJ} ${GATT_SERVER_OBJ} ${SM_OBJ} battery_service_server.o device_information_service_server.o hids_device.o btstack_ring_buffer.o
SOURCES+= ${MONGOOSE_DIR}/mongoose.c
SOURCES+= neeo-shield-driver.c

# Remove flags
CFLAGS:=$(filter-out -Wredundant-decls,${CFLAGS})
CFLAGS:=$(filter-out -Wshadow,${CFLAGS})
CFLAGS:=$(filter-out -Wmissing-prototypes,${CFLAGS})

# define target
neeo-shield-driver = neeo-shield-driver

# Create our target
neeo-shield-driver: $(SOURCES)
	${CC} $^ ${CFLAGS} ${LDFLAGS} -o ../neeo-shield-driver
