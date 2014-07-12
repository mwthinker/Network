#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "packet.h"

#include <cassert>
#include <queue>

#include <vector>
#include <array>

namespace net {

	class Client {
	public:
		friend class Network;

		Client() {
			id_ = 0;
		}

		virtual ~Client() {
		}

		virtual bool pullReceiveData(Packet& packet) {
			return true;
		}

		inline int getId() const {
			return id_;
		}

	protected:
		Client(int id) : id_(id) {
		}

		void receiveData(const std::array<char, 256>::const_iterator& begin, const std::array<char, 256>::const_iterator& end) {
			receiveBuffer_.push_back(end - begin);
			receiveBuffer_.insert(receiveBuffer_.end(), begin, end);
		}

		std::vector<char> receiveBuffer_;		
	
	private:
		int id_;
	};

} // Namespace net.

#endif // NET_CLIENT_H
