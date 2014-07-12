#include "network.h"
#include "client.h"
#include "server.h"
#include "local.h"
#include "server.h"
#include "remote.h"

#include <SDL_net.h>

#include <array>

namespace net {

	Network::Network() {
		server_ = nullptr;
		local_ = nullptr;
		listenSocket_ = nullptr;
		socketSet_ = nullptr;
		lastId_ = 0;
	}

	Network::~Network() {
		if (thread_.joinable()) {
			mutex_.lock();
			active_ = false;
			mutex_.unlock();
			thread_.join();
		}
		if (socketSet_ != nullptr) {
			SDLNet_FreeSocketSet(socketSet_);
		}
	}

	std::shared_ptr<Local> Network::getLocal() {
		return local_;
	}

	std::shared_ptr<Server> Network::createServer(int port) {
		if (server_ == nullptr) {
			lastId_ = 0;
			server_ = std::make_shared<Server>(this);
			local_ = std::make_shared<Local>(this, ++lastId_);
			if (serverListen(port)) {
				socketSet_ = SDLNet_AllocSocketSet(8);
				thread_ = std::thread(&Network::serverRun, this);
			} else {
				return nullptr;
			}
		}
		return server_;
	}

	std::shared_ptr<Server> Network::createLocalServer() {
		if (local_ == nullptr) {
			server_ = std::make_shared<Server>(this);
			local_ = std::make_shared<Local>(this, 1);
			return server_;
		}
		return nullptr;
	}

	std::shared_ptr<Client> Network::pullNewConnections() {
		return nullptr;
	}

	void Network::connectToServer(int port, std::string ip) {
		if (SDLNet_ResolveHost(&ip_, ip.c_str(), port) < 0) {
			fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError());
			return;
		}
		local_ = std::make_shared<Local>(this, 0);
		thread_ = std::thread(&Network::clientRun, this);
	}

	bool Network::serverListen(int port) {
		// Resolving the host using NULL make network interface to listen.
		if (SDLNet_ResolveHost(&ip_, NULL, port) < 0) {
			fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError());
			return false;
		}

		// Open a connection with the IP provided (listen on the host's port).
		listenSocket_ = SDLNet_TCP_Open(&ip_);
		if (listenSocket_ == nullptr) {
			fprintf(stderr, "SDLNet_TCP_Open: %s\n", SDLNet_GetError());
			return false;
		}
		return true;
	}

	void Network::clientRun() {
		TCPsocket socket = SDLNet_TCP_Open(&ip_);
		if (socket) {
			SDLNet_TCP_AddSocket(socketSet_, socket);
			char id;
			SDLNet_TCP_Recv(socket, &id, 1);
			mutex_.lock();
			local_->id_ = id;
			active_ = true;
			mutex_.unlock();
			while (active_) {
				if (SDLNet_SocketReady(socket) != 0) {
					std::array<char, 256> data;
					int receiveSize = SDLNet_TCP_Recv(socket, data.data(), sizeof(data));
					networkBuffer_.receive(data.data(), receiveSize);
					if (networkBuffer_.sendBuffer_.size() > 1) {
						unsigned int packageSize = networkBuffer_.sendBuffer_[0];
						int receiverId = networkBuffer_.sendBuffer_[1];

						// Whole package received?
						if (packageSize < networkBuffer_.sendBuffer_.size()) {
							// Set the correct id. So the remote client see the correct id.
							// Data sent from the server?
							if (receiverId == server_->SERVER_ID) {
								// Insert the data to server buffer from the remote client.
								mutex_.lock();
								networkBuffer_.receiveBuffer_.insert(networkBuffer_.sendBuffer_.end(), networkBuffer_.sendBuffer_.data(), networkBuffer_.sendBuffer_.data() + packageSize);
								mutex_.unlock();
							} else { // Send through to all remote connections!
								// Send to all remote clients.
								for (auto& pair : clients_) {
									// Ignore the sender of the data.
									local_->receiveBuffer_.insert(local_->receiveBuffer_.end(), networkBuffer_.sendBuffer_.begin(), networkBuffer_.sendBuffer_.begin() + packageSize);
								}
							}
							// Remove the data sent. I.e. the whole package.
							networkBuffer_.removeFromSendBuffer(packageSize);
						}
					}
				}
			}
		}
	}

	void Network::serverRun() {
		active_ = true;
		while (active_) {
			serverHandleNewConnection();

			// Receive data to all sockets.
			serverReceiveData();

			// Send local data to everyone.
			serverSendLocalData();

			// Send server data to everyone.
			serverSendServerData();
		}
	}

	void Network::serverHandleNewConnection() {
		// New connection?
		if (TCPsocket socket = SDLNet_TCP_Accept(listenSocket_)) {
			if (IPaddress* remoteIP_ = SDLNet_TCP_GetPeerAddress(socket)) {
				SDLNet_TCP_AddSocket(socketSet_, socket);
				// Todo! Fix correct id!
				clients_[socket].client_ = std::make_shared<Remote>(++lastId_);
			} else {
				fprintf(stderr, "SDLNet_TCP_GetPeerAddress: %s\n", SDLNet_GetError());
			}
		}
	}

	void Network::serverReceiveData() {
		while (SDLNet_CheckSockets(socketSet_, 0) > 0) {
			for (TCPsocket socket : sockets_) {
				// Is ready to receive data?
				if (SDLNet_SocketReady(socket) != 0) {
					std::array<char, 256> data;
					int receiveSize = SDLNet_TCP_Recv(socket, data.data(), sizeof(data));
					Pair& remote = clients_[socket];
					remote.buffer_.receive(data.data(), receiveSize);
					if (remote.buffer_.sendBuffer_.size() > 1) {
						unsigned int packageSize = remote.buffer_.sendBuffer_[0];
						int receiverId = remote.buffer_.sendBuffer_[1];
						
						// Whole package received?
						if (packageSize < remote.buffer_.sendBuffer_.size()) {
							// Set the correct id. So the remote client see the correct id.
							remote.buffer_.sendBuffer_[1] = remote.client_->id_;
							// Data assign to the server?
							if (receiverId == server_->SERVER_ID) {
								// Insert the data to server buffer from the remote client.
								mutex_.lock();
								networkBuffer_.receiveBuffer_.insert(remote.buffer_.sendBuffer_.end(), remote.buffer_.sendBuffer_.data(), remote.buffer_.sendBuffer_.data() + packageSize);
								mutex_.unlock();
							} else { // Send through to all remote connections!
								// Send to all remote clients.
								for (auto& pair : clients_) {
									// Ignore the sender of the data.
									if (socket != pair.first) {
										remote.buffer_.sendBuffer_[1] = remote.client_->id_;
										remote.client_->receiveBuffer_.insert(local_->receiveBuffer_.end(), remote.buffer_.sendBuffer_.begin(), remote.buffer_.sendBuffer_.begin() + packageSize);
										SDLNet_TCP_Send(pair.first, remote.buffer_.sendBuffer_.data(), packageSize);
									}
								}
							}
							// Remove the data sent. I.e. the whole package.
							remote.buffer_.removeFromSendBuffer(packageSize);
						}
					}
				}
				break;
			}
		}
	}

	void Network::serverSendLocalData() {
		unsigned int index = 0;
		int sizeSent = 0;
		mutex_.lock();
		while (local_->sendBuffer_.size() > index) {
			int size = local_->sendBuffer_[index];

			// Data ready to be sent?
			if (local_->sendBuffer_.size() - index >= size + 1) {
				// Send to all clients.
				for (auto& pair : clients_) {
					// Byte 1: SIZE.
					// Byte 2: SENDER_ID.
					// Byte 3 -> SIZE-2: DATA.
					SDLNet_TCP_Send(pair.first, &local_->id_, 1);
					SDLNet_TCP_Send(pair.first, local_->sendBuffer_.data() + index, size + 1);
				}
				index = sizeSent;
				sizeSent += size + 1;
			} else {
				// Abort the while loop.
				break;
			}
		}
		// Remove all data which was sent.
		local_->sendBuffer_.erase(local_->sendBuffer_.begin(), local_->sendBuffer_.begin() + sizeSent);
		mutex_.unlock();
	}

	void Network::serverSendServerData() {
		int index = 0;
		int sizeSent = 0;
		mutex_.lock();
		while (server_->sendBuffer_.size() > index) {
			int size = server_->sendBuffer_[index];

			// Data ready to be sent?
			if (server_->sendBuffer_.size() - index >= size + 1) {
				// Send package to all clients.
				for (auto& pair : clients_) {
					// Byte 1: SIZE.
					// Byte 2: SENDER_ID.
					// Byte 3 -> SIZE-3: DATA.
					char id = 0;
					SDLNet_TCP_Send(pair.first, &id, 1);
					SDLNet_TCP_Send(pair.first, local_->sendBuffer_.data() + index, size + 1);
				}
				index = sizeSent;
				sizeSent += size + 1;
			} else {
				// Abort the while loop.
				break;
			}
		}
		if (sizeSent > 0) {
			// Remove all data which was sent.
			server_->sendBuffer_.erase(local_->sendBuffer_.begin(), local_->sendBuffer_.begin() + sizeSent);
		}
		mutex_.unlock();
	}

	void Network::sendToServer(char senderId, Packet packet) {
		std::lock_guard<std::mutex> lock(mutex_);
		server_->receiveBuffer_.push_back(packet.size() + 2);
		server_->receiveBuffer_.push_back(senderId);
		server_->receiveBuffer_.insert(server_->receiveBuffer_.end(), packet.getData(), packet.getData() + packet.size());
	}

	void Network::sendToClient(char senderId, std::shared_ptr<Client> receiver, Packet packet) {
		std::lock_guard<std::mutex> lock(mutex_);
		receiver->receiveBuffer_.insert(receiver->receiveBuffer_.end(), packet.getData(), packet.getData() + packet.size());
		//receiver->sendBuffer_.insert(receiver->sendBuffer_.end(), data, data + size);
	}

	void Network::sendToAll(char senderId, Packet packet) {
		for (auto& pair : clients_) {
			std::shared_ptr<Client>& client = pair.second.client_;
			if (client->id_ != senderId) {
				server_->receiveBuffer_.push_back(packet.size() + 2);
				server_->receiveBuffer_.push_back(senderId);
				server_->receiveBuffer_.insert(server_->receiveBuffer_.end(), packet.getData(), packet.getData() + packet.size());
			}
		}
		if (local_->id_ != senderId) {
			local_->serverReceiveBuffer_.push_back(packet.size() + 2);
			local_->serverReceiveBuffer_.push_back(senderId);
			local_->serverReceiveBuffer_.insert(local_->serverReceiveBuffer_.end(), packet.getData(), packet.getData() + packet.size());
		}
	}

	std::shared_ptr<Client> Network::getClient(char id) {
		for (auto& pair : clients_) {
			std::shared_ptr<Client>& remote = pair.second.client_;
			if (id == remote->id_) {
				return remote;
			}
		}
		if (id == local_->id_) {
			return local_;
		}
		return nullptr;
	}

} // Namespace net.
