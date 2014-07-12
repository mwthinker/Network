#ifndef NET_NETWORK_H
#define NET_NETWORK_H

#include "packet.h"

#include <SDL_net.h>

#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <memory>

namespace net {

	class Local;
	class Server;
	class Remote;
	class Client;

	class Network {
	public:
		friend class Local;
		friend class Server;
		friend class Remote;

		Network();
		~Network();

		// Get the local client which serve as the local receiver and sender.
		// A call must have been made to create a server or connect to a server.
		// Otherwise a nullptr is return.
		std::shared_ptr<Local> getLocal();

		// Create a server on the open port. Return the server. Nullpointer return
		// on error.
		std::shared_ptr<Server> createServer(int port);

		// Create a "local" server, i.e. a server without internet and remote connections.
		std::shared_ptr<Server> createLocalServer();

		// Return the latest client connected. Return null when there are no more 
		// clients to pull.
		std::shared_ptr<Client> pullNewConnections();

		// Connect to a server with the port and ip provided.
		void connectToServer(int port, std::string ip);

	private:
		class Buffer {
		public:
			void receive(const char data[], int size) {
				receiveBuffer_.insert(receiveBuffer_.end(), data, data + size);
				sendBuffer_.insert(receiveBuffer_.end(), data, data + size);
			}
			void removeFromSendBuffer(int size) {
				sendBuffer_.erase(sendBuffer_.begin(), sendBuffer_.begin() + size);
			}

			std::vector<char> receiveBuffer_;
			std::vector<char> sendBuffer_;
		};

		class Pair {
		public:
			Pair() {
				client_ = nullptr;
			}

			Pair(const std::shared_ptr<Client>& client) : client_(client) {
			}

			std::shared_ptr<Client> client_;
			Buffer buffer_;
		};

		void clientRun();

		void serverRun();
		bool serverListen(int port);
		void serverHandleNewConnection();
		void serverReceiveData();
		void serverSendLocalData();
		void serverSendServerData();

		// Must be a whole package.
		void sendToServer(char senderId, Packet packet);
		// Must be a whole package.
		void sendToAll(char senderId, Packet packet);
		// Must be a whole package.
		void sendToClient(char senderId, std::shared_ptr<Client> receiver, Packet packet);

		std::shared_ptr<Client> getClient(char id);

		std::shared_ptr<Server> server_;
		Buffer networkBuffer_;

		std::shared_ptr<Local> local_;

		std::vector<TCPsocket> sockets_;
		TCPsocket listenSocket_;
		SDLNet_SocketSet socketSet_;
		IPaddress ip_;
		bool active_;
		int lastId_;

		std::map<TCPsocket, Pair> clients_;
		std::thread thread_;
		std::mutex mutex_;
	};

} // Namespace net.

#endif // NET_NETWORK_H
