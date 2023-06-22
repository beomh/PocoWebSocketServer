#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPClientSession.h>

#include <Poco/Net/WebSocket.h>
#include <Poco/Net/NetException.h>
#include <Poco/JSON/Object.h>

int main()
{
	int flags;
	
	try
	{
		Poco::Net::HTTPClientSession cs("127.0.0.1", 8080);
		Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, "/ws", "HTTP/1.1");
		Poco::Net::HTTPResponse response;
		
		std::string json = "";
		
		std::string personFinFlag = "";
		Poco::JSON::Object person; // 빈 객체 생성
		std::string tmpStr;

		Poco::Net::WebSocket * ws = new Poco::Net::WebSocket(cs, request, response);

		std::string payload = "Hello Server";
		ws->sendFrame(payload.data(), payload.size(), Poco::Net::WebSocket::FRAME_TEXT);

		char tmpBuffer[1024] = { 0, };
		int recvDataSize = ws->receiveFrame(tmpBuffer, sizeof(tmpBuffer), flags);
		std::cout << "[recv]" << tmpBuffer << "\n";

		//std::cout << "Enter {\"testInnerObject\":{\"inner_property\":\"value\"}}\n";
		//std::cout << "Enter \"exit\" to exit\n";

		while (true)
		{
			json = "";
			//std::getline(std::cin, json);
			std::cout << "Enter \"exit\" to exit, or not : "; std::getline(std::cin, personFinFlag);
			if (personFinFlag == "exit") {
				ws->sendFrame(personFinFlag.data(), personFinFlag.size(), Poco::Net::WebSocket::FRAME_TEXT);
				break;
			}
			std::cout << "Enter Name: "; std::getline(std::cin, tmpStr); person.set("Name", tmpStr);
			std::cout << "Enter Age: ";	std::getline(std::cin, tmpStr);	person.set("Age", std::stoi(tmpStr));
			std::cout << "Enter Address: ";	std::getline(std::cin, tmpStr);	person.set("Address", tmpStr);

			// 구조화된 포맷 출력
			std::cout << "person: ";
			person.stringify(std::cout, 2);
			std::cout << "\n";

			Poco::JSON::Object::Ptr ptr;
			ptr = new Poco::JSON::Object(person);

			// 문자열 포맷 출력
			Poco::DynamicStruct person_struct = Poco::JSON::Object::makeStruct(ptr);
			std::cout << "\nperson_struct: " << person_struct.toString() << "\n";

			json = person_struct.toString();

			ws->sendFrame(json.data(), json.size(), Poco::Net::WebSocket::FRAME_TEXT);

			char buffer[1024] = { 0, };
			int recvDataSize = ws->receiveFrame(buffer, sizeof(buffer), flags);
			if (recvDataSize > 0)
			{
				std::cout << "[recv] " << buffer << "\n";
			}
			// 한 줄 입력용
			//if (json == "exit")
			//{
			//	break;
			//}
			/*if (personFinFlag == "exit") {
				break;
			}*/
		}

		ws->shutdown();
	}
	catch (Poco::Exception ex)
	{
		std::cout << ex.displayText() << "\n";
		return 0;
	}

	return 0;
}