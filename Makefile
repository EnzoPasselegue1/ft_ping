NAME = ft_ping

CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=200112L -I./includes
LDFLAGS = -lm

SRCS = srcs/main.c \
       srcs/parser.c \
       srcs/dns.c \
       srcs/socket.c \
       srcs/icmp.c \
       srcs/send_receive.c \
	   srcs/display.c 
#       srcs/timing.c \
#       srcs/stats.c \
#       srcs/signal.c \

#       srcs/utils.c

OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re