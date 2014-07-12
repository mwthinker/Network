#ifndef NET_REMOTE_H
#define NET_REMOTE_H

#include "client.h"

namespace net {

	class Remote : public Client {
	public:
		friend class Network;

		bool pullReceiveData(Packet& packet) override;
	
		Remote(int id);
	private:
		
	};

} // Namespace net.

#endif // NET_REMOTE_H
