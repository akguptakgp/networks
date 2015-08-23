compile:	fis_server.out peer_client.out peer_server.out
fis_server.out: FIS_Server.cpp
	g++ -w -o fis_server.out FIS_Server.cpp
peer_client.out: Peer_Client.cpp	
	g++ -w -o peer_client.out Peer_Client.cpp
peer_server.out: Peer_Server.cpp
	g++ -w -o peer_server.out Peer_Server.cpp
clean:
	rm *.out 