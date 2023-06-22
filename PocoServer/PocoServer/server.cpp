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

			//초기 1회 알림용
			char tmpBuffer[1024] = { 0, };
			recvDataSize = ws.receiveFrame(tmpBuffer, sizeof(tmpBuffer), flags);

			if (recvDataSize > 0) {
				char log[1024] = "Successfully connected to server";
				ws.sendFrame(log, sizeof(log), flags);
			}

			// 단순 문자열 데이터 통신
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

			//json 형식 데이터 파싱
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

					// person 구조체 형식
					Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();
					Poco::Dynamic::Var name = object->get("Name");
					Poco::Dynamic::Var age = object->get("Age");
					Poco::Dynamic::Var addr = object->get("Address");
					std::cout << "이름: " << name.toString() << "\n나이: " << age.toString() << "\n주소: " << addr.toString() << "\n";

					//{"testInnerObject":{"inner_property":"value"}} 형식
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
			// 에러 종류에 따라 다른 동작
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

// 웹소켓 이외의 접속 요청 시 ok 출력
class DummyRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:

	void handleRequest(Poco::Net::HTTPServerRequest& request,
		Poco::Net::HTTPServerResponse& response)
	{
		std::cout << "DummyRequestHandler: " << request.getURI() << "\n";

		response.setStatusAndReason(Poco::Net::HTTPResponse::HTTP_OK);
		const char * data = "알라뷰 밍경.\n";
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
		// Websocket 기반 클라이언트의 요청에 반응
		if (request.find("Upgrade") != request.end() && Poco::icompare(request["Upgrade"], "websocket") == 0) {
			return new WebSocketRequestHandler;
		}

		return new DummyRequestHandler;
	}
};

int main()
{
	// 서버 소켓 생성, ip는 localhost(ipconfig or 127.0.0.1), 서버의 port는 8080(custom)
	// ip : port = server socket
	Poco::Net::ServerSocket svs(8080);

	// HTTPserver에 전달할 파라미터 설정 -> 서버 지속 시간 설정할 때 사용
	Poco::Net::HTTPServerParams* pParams = new Poco::Net::HTTPServerParams;
	Poco::Timespan timeout(300, 0); // 300초 동안 서버 열어놓고 listening -> default로 60초만 기다림
	pParams->setTimeout(timeout);

	// HTTP 서버 클래스 인스턴스
	//SimpleRequestHandlerFactory() 인스턴스: 클라이언트로부터 오는 요청을 처리하는 역할, RequestHandler를 가지고 있다.
	//svs: 서버 단의 소켓
	//pParams : 서버에게 전달하는 파라미터
	Poco::Net::HTTPServer srv(new SimpleRequestHandlerFactory(), svs, pParams);

	// 서버 시작
	srv.start();

	std::cout << "Start Server with Port 8080" << "\n";
	std::cout << "Waiting for client connection...\n";

	// 표준 입력 반환 -> 뭔가 입력이 들어올 때까지 기다린다.
	getchar();

	// 서버 동작 중단
	srv.stop();

	return 0;
}