DIROBJ := obj/
DIREXE := exec/
DIRHEA := include/
DIRSRC := src/
DIRPIPES := Pipes/
DIRLIBROS := Libros/
DIRRESULTS := Results/

CC = g++
CFLAGS  = -c -std=c++17 -I$(DIRHEA) -g3 
LDLIBS = -pthread
 
  
all: clean dirs link link1 link2 client server

dirs:
	mkdir -p $(DIROBJ) $(DIREXE) $(DIRRESULTS) $(DIRPIPES)


manager: $(DIROBJ)manager.o 
	$(CC) -o $(DIREXE)$@ $^ $(LDLIBS)


client: $(DIROBJ)client.o 
	$(CC) -o $(DIREXE)$@ $^ $(LDLIBS)	


server: $(DIROBJ)server.o 
	$(CC) -o $(DIREXE)$@ $^ $(LDLIBS)	


$(DIROBJ)%.o: $(DIRSRC)%.cpp
	$(CC) $(CFLAGS) $^ -o $@


test:
	$(DIREXE)manager

link: manager
	ln -s exec/manager ./manager

link1: server
	ln -s exec/server ./server

link2: client
	ln -s exec/client ./client


clean:
	rm -rf *~ core $(DIROBJ) $(DIREXE) $(DIRPIPES) $(DIRRESULTADOS) $(DIRHEA)*~ $(DIRSRC)*~ $(DIRRESULTS) ./manager ./server ./client

