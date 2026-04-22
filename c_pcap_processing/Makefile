CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -Wshadow -g -I. \
         -I./src/parser \
         -I./src/layers/l2 \
         -I./src/layers/l3 \
         -I./src/layers/l3/ipv4 \
         -I./src/layers/l3/ipv6 \
         -I./src/layers/l4 \
         -I./src/layers/l4/tcp \
         -I./src/layers/l4/udp \
         -I./src/layers/l4/icmp \
         -I./helper \
         -I./src/structs/layer4_hash \
         -I./src/structs/layer2_hash \
         -I./src/structs/patricia_tree \
         -I./src/structs/packet_store \
         -D_DEFAULT_SOURCE

LDFLAGS = -lpcap

SRCS = main.c \
       src/parser/parser.c \
       src/layers/l2/eth_handler.c \
       src/layers/l3/l3_handler.c \
       src/layers/l3/ipv4/ipv4.c \
       src/layers/l3/ipv6/ipv6.c \
       src/layers/l4/l4_handler.c \
       src/layers/l4/tcp/tcp_handler.c \
       src/layers/l4/udp/udp_handler.c \
       src/layers/l4/icmp/icmp_handler.c \
       helper/print_packet.c\
       src/structs/layer4_hash/flow_table.c\
       src/structs/layer2_hash/mac_table.c\
       src/structs/patricia_tree/ip_tree.c\
       src/structs/packet_store/packet_store.c\

# יצירת רשימת אובייקטים מתוך ה-SRCS
OBJS = $(SRCS:.c=.o)

TARGET = wiresherlock

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

PCAP_FILE = ../pcap_files/regular_pcap_file.pcap

run: $(TARGET)
	./$(TARGET) $(PCAP_FILE)

check: $(TARGET)
	valgrind --leak-check=full \
	         --show-leak-kinds=all \
	         --track-origins=yes \
	         --verbose \
	         ./$(TARGET) $(PCAP_FILE)