NAME = "main"

mac:
	@echo "Compiling for OSX..."
	g++ -o $(NAME) $(NAME).cpp -I/usr/X11R6/include -L/usr/X11R6/lib -lX11
	./$(NAME) 80 160 20 40

linux: 
	@echo "Compiling for Linux and Running..."
	g++ -o $(NAME) $(NAME).cpp -L/usr/X11R6/lib -lX11 -lstdc++ 
	./$(NAME) 80 150 20 40


