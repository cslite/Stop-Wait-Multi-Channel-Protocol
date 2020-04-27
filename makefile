exe: utils.c utils.h arqClient.c arqServer.c packet.h config.h
	gcc arqClient.c utils.c -o client
	gcc arqServer.c utils.c -o server

