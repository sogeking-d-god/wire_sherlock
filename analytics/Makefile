CC = gcc

PCAP_ROOT = ../c_pcap_processing
SRC_PCAP  = $(PCAP_ROOT)/src

CFLAGS = -Wall -Wextra -O3 \
         -I./include \
         -I$(PCAP_ROOT) \
         -I$(SRC_PCAP)/parser \
         -I$(SRC_PCAP)/layers/l2 \
         -I$(SRC_PCAP)/layers/l3 \
         -I$(SRC_PCAP)/layers/l3/ipv4 \
         -I$(SRC_PCAP)/layers/l3/ipv6 \
         -I$(SRC_PCAP)/layers/l4 \
         -I$(SRC_PCAP)/layers/l4/tcp \
         -I$(SRC_PCAP)/layers/l4/udp \
         -I$(SRC_PCAP)/layers/l4/icmp \
         -I$(SRC_PCAP)/layers/l7 \
         -I$(SRC_PCAP)/layers/l7/http \
         -I$(PCAP_ROOT)/helper \
         -I$(SRC_PCAP)/structs/layer4_hash \
         -I$(SRC_PCAP)/structs/layer2_hash \
         -I$(SRC_PCAP)/structs/patricia_tree \
         -I$(SRC_PCAP)/structs/packet_store \
         -I$(PCAP_ROOT)/vendor/xxhash \
         -D_DEFAULT_SOURCE

LDFLAGS = -lm

SRC = src/pelt.c \
      src/stats.c \
      src/dbscan.c \
      src/kd_tree.c \
      src/time_series.c \
      src/metrics.c \
      src/intro_select.c \
      src/normalize.c \
      src/micro_detector.c \
      src/anomaly_detector.c

OBJ = $(SRC:.c=.o)
TARGET = libanalytics.a

all: $(TARGET)

$(TARGET): $(OBJ)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)
