#include "remote.h"

namespace net {

	bool Remote::pullReceiveData(Packet& packet) {
		if (receiveBuffer_.size() > 2) {
			int size = receiveBuffer_[0];
			int remoteId = receiveBuffer_[1];
			packet = Packet(receiveBuffer_.data(), size);
			receiveBuffer_.erase(receiveBuffer_.begin(), receiveBuffer_.begin() + size);
		}
		return true;
	}

	Remote::Remote(int id) : Client(id) {
	}

} // Namespace net.
