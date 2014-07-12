#include "server.h"
#include "client.h"
#include "network.h"

namespace net {

	// SERVER ID = 0;
	// Protocol.
	// Byte:
	// 1: PACKAGE_SIZE
	// 2: CLIENT_ID
	// 3 -> PACKAGE_SIZE: DATA
	Server::Server(Network* network) {
		network_ = network;
	}

	Server::~Server() {
	}

	std::shared_ptr<Client> Server::pullReceiveData(Packet& packet) {
		std::lock_guard<std::mutex> lock(network_->mutex_);
		if (receiveBuffer_.size() > 2) {
			int size = receiveBuffer_[0];
			int id = receiveBuffer_[1];
			packet = Packet(receiveBuffer_.data() + 2, size - 2);
			receiveBuffer_.erase(receiveBuffer_.begin(), receiveBuffer_.begin() + size);
			return network_->getClient(id);
		}
		return nullptr;
	}

	void Server::sendToAll(const Packet& packet) {
		network_->sendToAll(SERVER_ID, packet);
	}

	void Server::sendTo(std::shared_ptr<Client> receiver, const Packet& packet) {
		network_->sendToClient(SERVER_ID, receiver, packet);
	}

} // Namespace net.
