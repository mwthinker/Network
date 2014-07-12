#include "local.h"
#include "client.h"
#include "network.h"

namespace net {

	Local::Local(Network* network, int id) : Client(id) {
		network_ = network;
	}

	bool Local::pullReceiveData(Packet& packet) {
		std::lock_guard<std::mutex> lock(network_->mutex_);
		if (receiveBuffer_.size() > 2) {
			int size = receiveBuffer_[0];
			int remoteId = receiveBuffer_[1];
			packet = Packet(receiveBuffer_.data() + 2, size - 2);
			receiveBuffer_.erase(receiveBuffer_.begin(), receiveBuffer_.begin() + size);
			return true;
		}
		return false;
	}

	void Local::sendToAll(const Packet& packet) {
		network_->sendToAll(getId(), packet);
	}

	void Local::sendToServer(const Packet& packet) {
		network_->sendToServer(getId(), packet);
	}

	bool Local::pullReceiveDataFromServer(Packet& packet) {
		std::lock_guard<std::mutex> lock(network_->mutex_);
		if (serverReceiveBuffer_.size() > 2) {
			int size = serverReceiveBuffer_[0];
			int remoteId = serverReceiveBuffer_[1];
			packet = Packet(serverReceiveBuffer_.data() + 2, size - 2);
			serverReceiveBuffer_.erase(serverReceiveBuffer_.begin(), serverReceiveBuffer_.begin() + size);
			return true;
		}
		return false;
	}

} // Namespace net.
