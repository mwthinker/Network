#include "net/network.h"
#include "net/server.h"
#include "net/client.h"
#include "net/local.h"

#include <string>
#include <sstream>
#include <cassert>
#include <iostream>


void test1() {
	net::Network network;
	// Local must be nullptr because no call to create or connect to server.
	assert(network.getLocal() == nullptr);
	std::cout << "Test 1 succeeded, i.e. create a local and get a local client.\n";
}

// Test to receive data from a non network server.
void receiveDataFromServer(std::shared_ptr<net::Server> server, net::Network& network) {
	// Should not fail!
	assert(server != nullptr);
	std::shared_ptr<net::Local> local = network.getLocal();
	// Should not fail!
	assert(local);

	char data[] = {'a','b','c'};

	// Send 'abc'.
	net::Packet packet(data, sizeof(data));
	server->sendToAll(packet);

	packet = net::Packet();
	// Must be ready to receive.
	assert(local->pullReceiveDataFromServer(packet));
	// Must be empty.
	assert(!local->pullReceiveData(packet));
	
	// The data received from the server must be the same as the data sent from the server.
	assert(packet.size() == sizeof(data));
	for (int i = 0; i < sizeof(data); ++i) {
		// Is the same?
		assert(packet[i] == data[i]);
	}

	// Must be empty.
	assert(!local->pullReceiveData(packet));
	assert(!local->pullReceiveDataFromServer(packet));

	char data2[] = {'d', 'e'};
	packet = net::Packet(data2, sizeof(data2));

	// Send 'de'.
	server->sendToAll(packet);

	packet = net::Packet();
	// Must receive a package.
	assert(local->pullReceiveDataFromServer(packet));

	// The data received from the server must be the same as the data sent from the server.
	assert(packet.size() == sizeof(data2));
	for (int i = 0; i < sizeof(data2); ++i) {
		// Is the same?
		assert(packet[i] == data2[i]);
	}

	// Must be empty.
	assert(!local->pullReceiveData(packet));
	assert(!local->pullReceiveDataFromServer(packet));
}

// Test to send data to a non network server.
void sendDataToServer(std::shared_ptr<net::Server> server, net::Network& network) {
	// Should not fail!
	assert(server != nullptr);
	std::shared_ptr<net::Local> local = network.getLocal();
	// Should not fail!
	assert(local);

	char data[] = {'a', 'b', 'c'};
	// Send 'abc'.
	net::Packet packet(data, sizeof(data));
	local->sendToServer(packet);			
	
	packet = net::Packet();
	std::shared_ptr<net::Client> client = server->pullReceiveData(packet);
	// Receives from local.
	assert(client && client->getId() == local->getId());
	
	// The data received from the server must be the same as the data sent from the server.
	assert(packet.size() == sizeof(data));
	for (int i = 0; i < sizeof(data); ++i) {		
		// Is the same?
		assert(packet[i] == data[i]);
	}
		
	// Must be empty.
	assert(!server->pullReceiveData(packet));

	char data2[] = {'d', 'e'};
	// Send 'de'.
	packet = net::Packet(data2, sizeof(data2));
	local->sendToServer(packet);

	packet = net::Packet();
	// Receives from local.
	client = server->pullReceiveData(packet);
	assert(client && client->getId() == local->getId());

	// The data received from the server must be the same as the data sent from the server.
	assert(packet.size() == sizeof(data2));
	for (int i = 0; i < sizeof(data2); ++i) {
		// Is the same?
		assert(packet[i] == data2[i]);
	}
}

// Test to receive data from a non network server.
void test2() {
	net::Network network;
	std::shared_ptr<net::Server> server = network.createLocalServer();
	sendDataToServer(server, network);
	std::cout << "Test 2 succeeded, i.e. to receive data from a non network server.\n";
}

// Test to send data to a non network server.
void test3() {
	net::Network network;
	std::shared_ptr<net::Server> server = network.createLocalServer();
	receiveDataFromServer(server, network);
	std::cout << "Test 3 succeeded, i.e. to send data to a non network server.\n";
}

// Test the network server without remote connections.
void test4() {
	SDLNet_Init();
	net::Network network;
	std::shared_ptr<net::Server> server = network.createServer(12457);
	
	// Server valid.
	assert(server);

	sendDataToServer(server, network);
	receiveDataFromServer(server, network);
	
	SDLNet_Quit();
	std::cout << "Test 4 succeeded, i.e. to send/receive data to/from a network server without remote connections.\n";
}

// Test the network server with remote connections.
void test5() {
	SDLNet_Init();
	net::Network network1;
	std::shared_ptr<net::Server> server = network1.createServer(12457);

	net::Network network2;
	network2.connectToServer(12457, "localhost");
	
	// Server valid.
	assert(server);

	SDLNet_Quit();
	std::cout << "Test 5 succeeded, i.e. to send/receive data to/from a network server with remote connections.\n";
}

int main(int argc, char** argv) {
	test1();
	test2();
	test3();
	test4();
	test5();

	std::cout << "All test succeeded!\n";
	return 0;
}
