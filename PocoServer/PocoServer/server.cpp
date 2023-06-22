#include <iostream>
#include <string>

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Net/NetException.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/DynamicAny.h>

// second
class WebSocketRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	void handleRequest(Poco::Net::HTTPServerRequest& request,
		Poco::Net::HTTPServerResponse& response)
	{
		try
		{
			Poco::Net::WebSocket ws(request, response);

			std::cout << "Successfully connected with the client." << "\n";

			int flags = 0;
			int recvDataSize = 0;
			char finFlags[] = "exit";
			//std::string finFlags = "exit";
			std::string json;
			Poco::JSON::Parser parser;
			Poco::Dynamic::Var result;

			//�ʱ� 1ȸ �˸���
			char tmpBuffer[1024] = { 0, };
			recvDataSize = ws.receiveFrame(tmpBuffer, sizeof(tmpBuffer), flags);

			if (recvDataSize > 0) {
				char log[1024] = "Successfully connected to server";
				ws.sendFrame(log, sizeof(log), flags);
			}

			// �ܼ� ���ڿ� ������ ���
			/*do {
				char buffer[1024] = { 0, };
				recvDataSize = ws.receiveFrame(buffer, sizeof(buffer), flags);

				if (recvDataSize > 0) {
					char log[1024] = { 0, };
					sprintf_s(log, 1024, "Frame received (length=%d, flags=0x%x): %s", recvDataSize, unsigned(flags), buffer);
					std::cout << log << "\n";


					ws.sendFrame(buffer, recvDataSize, flags);
				}
			} while (!(recvDataSize <= 0 || (flags & Poco::Net::WebSocket::FRAME_OP_BITMASK) == Poco::Net::WebSocket::FRAME_OP_CLOSE));*/

			//json ���� ������ �Ľ�
			while (!(recvDataSize <= 0 || (flags & Poco::Net::WebSocket::FRAME_OP_BITMASK) == Poco::Net::WebSocket::FRAME_OP_CLOSE)) {
				char buffer[1024] = { 0, };
				recvDataSize = ws.receiveFrame(buffer, sizeof(buffer), flags);

				if (std::string(buffer) == finFlags) {
					std::cout << "Client terminated connection. \n";
					ws.sendFrame(buffer, recvDataSize, flags);
					break;
				}
				try {
					json = std::string(buffer);
					result = parser.parse(json);

					// person ����ü ����
					Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();
					Poco::Dynamic::Var name = object->get("Name");
					Poco::Dynamic::Var age = object->get("Age");
					Poco::Dynamic::Var addr = object->get("Address");
					std::cout << "�̸�: " << name.toString() << "\n����: " << age.toString() << "\n�ּ�: " << addr.toString() << "\n";

					//{"testInnerObject":{"inner_property":"value"}} ����
					//Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();
					//Poco::Dynamic::Var test = object->get("testInnerObject"); // holds { "property" : "value" }
					//Poco::JSON::Object::Ptr subObject = test.extract<Poco::JSON::Object::Ptr>();
					//test = subObject->get("inner_property");
					//std::string val = test.toString(); // val holds "value"

					//std::cout << "inner_property: " << val << "\n";
					ws.sendFrame(buffer, recvDataSize, flags);
				}
				catch (Poco::Exception ex) {
					//std::cout << ex.displayText() << "\n";
					char errBuffer[1024] = "Please re-enter";
					ws.sendFrame(errBuffer, sizeof(errBuffer), flags);
				}
			}
		}
		catch (Poco::Net::WebSocketException& exc)
		{
			 std::cout << exc.displayText() << "\n";
			// ���� ������ ���� �ٸ� ����
			switch (exc.code())
			{
				case Poco::Net::WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
					response.set("Sec-WebSocket-Version", Poco::Net::WebSocket::WEBSOCKET_VERSION);
					// fallthrough
				case Poco::Net::WebSocket::WS_ERR_NO_HANDSHAKE:
				case Poco::Net::WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
				case Poco::Net::WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
					response.setStatusAndReason(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
					response.setContentLength(0);
					response.send();
			}
		}
	}
};

// ������ �̿��� ���� ��û �� ok ���
class DummyRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:

	void handleRequest(Poco::Net::HTTPServerRequest& request,
		Poco::Net::HTTPServerResponse& response)
	{
		std::cout << "DummyRequestHandler: " << request.getURI() << "\n";

		response.setStatusAndReason(Poco::Net::HTTPResponse::HTTP_OK);
		const char * data = "�˶�� �ְ�.\n";
		response.sendBuffer(data, strlen(data));
	}
};

// first 
class SimpleRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
	Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request)
	{
		std::cout << "SimpleRequestHandlerFactory: " << request.getURI() << "\n";
		// Websocket ��� Ŭ���̾�Ʈ�� ��û�� ����
		if (request.find("Upgrade") != request.end() && Poco::icompare(request["Upgrade"], "websocket") == 0) {
			return new WebSocketRequestHandler;
		}

		return new DummyRequestHandler;
	}
};

int main()
{
	// ���� ���� ����, ip�� localhost(ipconfig or 127.0.0.1), ������ port�� 8080(custom)
	// ip : port = server socket
	Poco::Net::ServerSocket svs(8080);

	// HTTPserver�� ������ �Ķ���� ���� -> ���� ���� �ð� ������ �� ���
	Poco::Net::HTTPServerParams* pParams = new Poco::Net::HTTPServerParams;
	Poco::Timespan timeout(300, 0); // 300�� ���� ���� ������� listening -> default�� 60�ʸ� ��ٸ�
	pParams->setTimeout(timeout);

	// HTTP ���� Ŭ���� �ν��Ͻ�
	//SimpleRequestHandlerFactory() �ν��Ͻ�: Ŭ���̾�Ʈ�κ��� ���� ��û�� ó���ϴ� ����, RequestHandler�� ������ �ִ�.
	//svs: ���� ���� ����
	//pParams : �������� �����ϴ� �Ķ����
	Poco::Net::HTTPServer srv(new SimpleRequestHandlerFactory(), svs, pParams);

	// ���� ����
	srv.start();

	std::cout << "Start Server with Port 8080" << "\n";
	std::cout << "Waiting for client connection...\n";

	// ǥ�� �Է� ��ȯ -> ���� �Է��� ���� ������ ��ٸ���.
	getchar();

	// ���� ���� �ߴ�
	srv.stop();

	return 0;
}