all: server client

server:
	$(MAKE) -C Server

client:
	$(MAKE) -C Client


.PHONY: clean all

clean:
	$(MAKE) -C Server clean
	$(MAKE) -C Client clean
