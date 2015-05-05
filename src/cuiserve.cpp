#include "cuiserve.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
using namespace json_spirit;
bool CUIServer::HasConnection(){
	if(NULL == instance)
		return false;

	return instance->m_bConnect;
}

void CUIServer::StartServer(boost::thread_group & group) {
	group.create_thread(boost::bind(&CUIServer::RunThreadPorc, CUIServer::getInstance()));
}

void CUIServer::StopServer(){
	if(NULL == instance)
		return ;
	instance->m_iosev.stop();
	instance->m_bRunFlag = false;
}

void CUIServer::RunThreadPorc(CUIServer* pThis) {
	if (NULL == pThis)
		return;

	pThis->RunServer();
}

CUIServer* CUIServer::getInstance() {
	if (NULL == instance)
		instance = new CUIServer();
	return instance;
}

void CUIServer::Send(const string& strData) {
	if(NULL == instance)
		return ;
	LogPrint("TOUI","init message: %s\n", strData);
	instance->SendData(strData);
}

bool CUIServer::IsInitalEnd = false;
void CUIServer::SendData(const string& strData) {
	system::error_code ignored_error;
	if(m_bConnect&&m_socket.get() ){
		m_socket->write_some(asio::buffer(strData), ignored_error);
	}
}

void CUIServer::Accept() {
	sock_pt sock(new tcp::socket(m_iosev));
	m_acceptor.async_accept(*sock, bind(&CUIServer::Accept_handler, this, sock));
}

void CUIServer::Accept_handler(sock_pt sock) {
	if(m_bConnect)
	{
		//only save one connection
		Accept();
		return;
	}
	m_socket = sock;
	m_bConnect = true;
	Object obj;
	if (CUIServer::IsInitalEnd == true) {
		obj.push_back(Pair("type", "init"));
		obj.push_back(Pair("msg", "initialize end"));
	} else {
		obj.push_back(Pair("type", "hello"));
		obj.push_back(Pair("msg", "hello asio"));
	}

	Accept();
	sock->async_write_some(asio::buffer(write_string(Value(std::move(obj)),true)), bind(&CUIServer::write_handler, this));
	std::shared_ptr<vector<char> > str(new vector<char>(100, 0));
	memset(data_,0,max_length);
	sock->async_read_some(asio::buffer(data_,max_length),
			bind(&CUIServer::read_handler, this, asio::placeholders::error, data_, sock));
}

void CUIServer::read_handler(const system::error_code& ec, char* pstr, sock_pt sock) {
	if (ec) {
		sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both, const_cast<system::error_code&>(ec));
		sock->close(const_cast<system::error_code&>(ec));
		m_bConnect = false;
		return;
	}
}

void CUIServer::RunServer(){
	Accept();
	m_iosev.run();
	if(instance != NULL && !instance->m_bRunFlag) {
		delete instance;
			instance = NULL;
	}
}

CUIServer* CUIServer::instance = NULL;
