//#include <iostream>
//
//#ifdef  _WIN32
//#define _WIN32_WINNT 0x0601 // For Windows 7 and above
//#endif
//
//#define ASIO_STANDALONE
//
//#include <asio.hpp>
//#include <asio/ts/buffer.hpp>
//#include <asio/ts/internet.hpp>
//
//std::vector<char> vBuffer(1 * 1024); // 20 KB buffer
//
//void GrabSomeData(asio::ip::tcp::socket& socket) {
//	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()),
//
//		[&](const asio::error_code& ec, std::size_t length) {
//			
//			if (!ec)
//			{
//				std::cout << "\n\nRead " << length << " bytes from socket.\n\n";
//
//				for (int i =0 ; i < length; i++)
//				{
//					std::cout << vBuffer[i];
//				}
//
//				GrabSomeData(socket);
//			}
//
//		});
//}
//
//int main() {
//
//	asio::error_code ec;
//
//	// Create a context 
//	asio::io_context context;
//
//	asio::io_context::work idleWork(context);
//
//	std::thread thrContext = std::thread([&]() {
//		context.run();
//		});
//
//	
//	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49", ec), 80); // Example IP address and port 80
//
//	asio::ip::tcp::socket socket(context);
//
//	socket.connect(endpoint, ec);
//
//	if (!ec) {
//		std::cout << "Connected to " << endpoint.address().to_string() << ":" << endpoint.port() << std::endl;
//	} else {
//		std::cerr << "Failed to connect: " << ec.message() << std::endl;
//		return 1;
//	}
//	
//	if (socket.is_open()) {
//		GrabSomeData(socket);
//
//		std::string sRequest = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
//
//		socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);
//
//		using namespace std::chrono_literals;
//		std::this_thread::sleep_for(2s); // Wait for 2 seconds to allow data to be read
//
//		context.stop();
//		if (thrContext.joinable()) thrContext.join();
//	}
//
//	return 0;
//}