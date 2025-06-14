#include <iostream>
#include <olc_net.h>

enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};

class CustomClient : public olc::net::client_interface<CustomMsgTypes>
{
public:
	void PingServer()	
	{
		olc::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerPing;

		// Caution with this...
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();		

		msg << timeNow;
		Send(msg);
	}

	void MessageAll()
	{
		olc::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::MessageAll;		
		Send(msg);
	}
};

//int main()
//{
//	CustomClient c;
//	c.Connect("127.0.0.1", 60000);
//
//	bool bQuit = false;
//	while (!bQuit)
//	{
//
//		if (c.IsConnected())
//		{
//			if (!c.Incoming().empty())
//			{
//
//
//				auto msg = c.Incoming().pop_front().msg;
//
//				switch (msg.header.id)
//				{
//				case CustomMsgTypes::ServerAccept:
//				{
//					// Server has responded to a ping request				
//					std::cout << "Server Accepted Connection\n";
//				}
//				break;
//
//
//				case CustomMsgTypes::ServerPing:
//				{
//					// Server has responded to a ping request
//					std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
//					std::chrono::system_clock::time_point timeThen;
//					msg >> timeThen;
//					std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
//				}
//				break;
//
//				case CustomMsgTypes::ServerMessage:
//				{
//					// Server has responded to a ping request	
//					uint32_t clientID;
//					msg >> clientID;
//					std::cout << "Hello from [" << clientID << "]\n";
//				}
//				break;
//				}
//			}
//		}
//		else
//		{
//			std::cout << "Server Down\n";
//			bQuit = true;
//		}
//
//	}
//
//	return 0;
//}
