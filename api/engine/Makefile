CC = cc

# נתיבי שורש יחסיים למיקום ה-Makefile (api/engine)
API_ROOT       = ..
PROJECT_ROOT   = ../..
ANALYTICS_ROOT = $(PROJECT_ROOT)/analytics
PCAP_ROOT      = $(PROJECT_ROOT)/c_pcap_processing

# ספריות פנימיות
CORE_DIR       = ./core
WRAPPERS_DIR   = ./wrappers
SRC_PCAP       = $(PCAP_ROOT)/src
SRC_ANALYTICS  = $(ANALYTICS_ROOT)/src

# Flags של קומפילציה - כולל את כל ה-Header paths מה-tree שלך
CFLAGS = -Wall -Wextra -Wpedantic -Wshadow -g \
         -I$(API_ROOT)/config \
         -I$(CORE_DIR) \
         -I$(WRAPPERS_DIR)/parsing \
         -I$(ANALYTICS_ROOT)/include \
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
         -I$(PCAP_ROOT)/helper \
         -I$(SRC_PCAP)/structs/layer4_hash \
         -I$(SRC_PCAP)/structs/layer2_hash \
         -I$(SRC_PCAP)/structs/patricia_tree \
         -I$(SRC_PCAP)/structs/packet_store \
         -D_DEFAULT_SOURCE
# קישור ספריות: הוספתי את -lm כי אלגוריתמים כמו DBSCAN/PELT דורשים math.h
LDFLAGS = -lpcap -lcjson -lm

# רשימת קבצי מקור לפי ה-tree שסיפקת
SRCS = $(CORE_DIR)/c_worker.c \
       $(WRAPPERS_DIR)/parsing/parser_wrapper.c \
       $(SRC_PCAP)/parser/parser.c \
       $(SRC_PCAP)/layers/l2/eth_handler.c \
       $(SRC_PCAP)/layers/l3/l3_handler.c \
       $(SRC_PCAP)/layers/l3/ipv4/ipv4.c \
       $(SRC_PCAP)/layers/l3/ipv6/ipv6.c \
       $(SRC_PCAP)/layers/l4/l4_handler.c \
       $(SRC_PCAP)/layers/l4/tcp/tcp_handler.c \
       $(SRC_PCAP)/layers/l4/udp/udp_handler.c \
       $(SRC_PCAP)/layers/l4/icmp/icmp_handler.c \
       $(PCAP_ROOT)/helper/print_packet.c \
       $(SRC_PCAP)/structs/layer4_hash/flow_table.c \
       $(SRC_PCAP)/structs/layer2_hash/mac_table.c \
       $(SRC_PCAP)/structs/patricia_tree/ip_tree.c \
       $(SRC_PCAP)/structs/packet_store/packet_store.c \
       $(SRC_ANALYTICS)/time_series.c \
       $(SRC_ANALYTICS)/pelt.c \
       $(SRC_ANALYTICS)/dbscan.c \
       $(SRC_ANALYTICS)/kd_tree.c \
       $(SRC_ANALYTICS)/metrics.c \
       $(SRC_ANALYTICS)/stats.c \
       $(SRC_ANALYTICS)/intro_select.c

OBJS = $(SRCS:.c=.o)

TARGET = $(CORE_DIR)/c_worker

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean