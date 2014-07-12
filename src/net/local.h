#ifndef NET_LOCAL_H
#define NET_LOCAL_H

#include "client.h"

#include <vector>

namespace net {

	class Network;

	class Local : public Client {
	public:
		friend class Network;

		Local(Network* network, int id);

		bool pullReceiveData(Packet& packet) override;

		void sendToAll(const Packet& packet);

		void sendToServer(const Packet& packet);

		bool pullReceiveDataFromServer(Packet& packet);

	private:
		Network* network_;
		std::vector<char> sendBuffer_;
		std::vector<char> serverReceiveBuffer_;
	};

} // Namespace net.

#endif // NET_LOCAL_H
