CC = cc
# נתיבים יחסיים
ENGINE_DIR = .
API_DIR = ..
CONFIG_DIR = $(API_DIR)/config
CORE_DIR = $(ENGINE_DIR)/core
WRAPPERS_DIR = $(ENGINE_DIR)/wrappers
PCAP_ENGINE_DIR = ../../c_pcap_processing
SRC_DIR = $(PCAP_ENGINE_DIR)/src

# Flags
CFLAGS = -Wall -Wextra -Wpedantic -Wshadow -g \
         -I$(CONFIG_DIR) \
         -I$(CORE_DIR) \
         -I$(WRAPPERS_DIR) \
         -I$(PCAP_ENGINE_DIR) \
         -I$(SRC_DIR)/parser \
         -I$(SRC_DIR)/layers/l2 \
         -I$(SRC_DIR)/layers/l3 \
         -I$(SRC_DIR)/layers/l3/ipv4 \
         -I$(SRC_DIR)/layers/l3/ipv6 \
         -I$(SRC_DIR)/layers/l4 \
         -I$(SRC_DIR)/layers/l4/tcp \
         -I$(SRC_DIR)/layers/l4/udp \
         -I$(SRC_DIR)/layers/l4/icmp \
         -I$(PCAP_ENGINE_DIR)/helper \
         -I$(SRC_DIR)/structs/layer4_hash \
         -I$(SRC_DIR)/structs/layer2_hash \
         -I$(SRC_DIR)/structs/patricia_tree \
         -I$(WRAPPERS_DIR)/parsing \
         -D_DEFAULT_SOURCE

LDFLAGS = -lpcap -lcjson

# קבצי מקור
SRCS = $(CORE_DIR)/c_worker.c \
       $(PCAP_ENGINE_DIR)/src/parser/parser.c \
       $(PCAP_ENGINE_DIR)/src/layers/l2/eth_handler.c \
       $(PCAP_ENGINE_DIR)/src/layers/l3/l3_handler.c \
       $(PCAP_ENGINE_DIR)/src/layers/l3/ipv4/ipv4.c \
       $(PCAP_ENGINE_DIR)/src/layers/l3/ipv6/ipv6.c \
       $(PCAP_ENGINE_DIR)/src/layers/l4/l4_handler.c \
       $(PCAP_ENGINE_DIR)/src/layers/l4/tcp/tcp_handler.c \
       $(PCAP_ENGINE_DIR)/src/layers/l4/udp/udp_handler.c \
       $(PCAP_ENGINE_DIR)/src/layers/l4/icmp/icmp_handler.c \
       $(PCAP_ENGINE_DIR)/helper/print_packet.c \
       $(PCAP_ENGINE_DIR)/src/structs/layer4_hash/flow_table.c \
       $(PCAP_ENGINE_DIR)/src/structs/layer2_hash/mac_table.c \
       $(PCAP_ENGINE_DIR)/src/structs/patricia_tree/ip_tree.c \
       $(WRAPPERS_DIR)/parsing/parser_wrapper.c

OBJS = $(SRCS:.c=.o)

TARGET = $(CORE_DIR)/c_worker

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)