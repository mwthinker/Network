#ifndef NET_SERVER_H
#define NET_SERVER_H

#include "client.h"

#include <vector>
#include <memory>

namespace net {

	class Network;

	class Server {
	public:
		friend class Network;

		Server(Network* network);
		virtual ~Server();
		
		std::shared_ptr<Client> pullReceiveData(Packet& packet);

		// Send the current data to all clients.
		void sendToAll(const Packet& packet);

		// Send the current data to the receiver.
		void sendTo(std::shared_ptr<Client> receiver, const Packet& packet);

	private:
		const int SERVER_ID = 0;

		// Only full packages.
		std::vector<char> sendBuffer_;
		std::vector<char> receiveBuffer_;
		Network* network_;
	};

} // Namespace net.

#endif // NET_SERVER_H
