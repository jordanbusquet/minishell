TARGET = minishell.exe

SRC_DIR = source
INC_DIR = headers

SRCS = main.c \
       $(SRC_DIR)/window.c \
       $(SRC_DIR)/style.c \
       $(SRC_DIR)/message_start.c \
       $(SRC_DIR)/terminal.c \
       $(SRC_DIR)/contenu.c

GTK_FLAGS = $(shell pkg-config --cflags --libs gtk4 2>/dev/null)
CFLAGS = -I$(INC_DIR) -Wall -Wextra -g

.PHONY: all shell check-gtk run clean fclean re

all: $(TARGET)

shell:
	$(MAKE) -C minishell

check-gtk:
	@pkg-config --exists gtk4 || (echo "Erreur : pkg-config ou GTK 4 est absent. Sous Ubuntu/WSL, installez-les avec : sudo apt install pkg-config libgtk-4-dev" && exit 1)

$(TARGET): $(SRCS) $(INC_DIR)/minishell_gui.h | shell check-gtk
	gcc $(CFLAGS) $(SRCS) -o $(TARGET) $(GTK_FLAGS)
	@echo "Compilation terminée : $(TARGET)"

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
	$(MAKE) -C minishell clean

fclean:
	rm -f $(TARGET)
	$(MAKE) -C minishell fclean

re: fclean all
