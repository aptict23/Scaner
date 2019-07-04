// boost_client.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#pragma once
#include "pch.h"
#include <iostream>
#include <boost/asio.hpp>
#include <boost\asio\buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>
#include <array>
#include <fstream>

using namespace boost;

class Service
{
public:
	Service(std::shared_ptr<asio::ip::tcp::socket> sock) : m_sock(sock)
	{}

	void StartHanding()
	{
		asio::async_read_until(*m_sock.get(), m_request, '\n', [this](const system::error_code& ec, std::size_t bytes_transferred)
			{
				onRequestReceived(ec, bytes_transferred);
			});
	}
private:
	void onRequestReceived(const boost::system::error_code& ec, std::size_t tes_transferred)
	{
		if (ec.value() != 0)
		{
			std::cout << "Error occured! Error code = " << ec.value() << ". Message: " << ec.message();
			onFinish();
			return;
		}
		std::cout << "Request received" << std::endl;
		m_response = ProcessRequest(m_request);
		asio::async_write(*m_sock.get(), asio::buffer(m_response), [this](const boost::system::error_code& ec, std::size_t bytes_transferred)
			{
				onResponseSent(ec, bytes_transferred);
			});	
	}
	void onResponseSent(const boost::system::error_code& ec, std::size_t bytes_transferred)
	{
		if (ec.value() != 0)
		{
			std::cout << "Error occured! Error code = " << ec.value() << ". Message: " << ec.message();
		}
		std::cout << "Request send" << std::endl;
		onFinish();
	}

	void onFinish()
	{
		delete this;
	}

	std::string ProcessRequest(asio::streambuf& request)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::string response = "Response\n";
		return response;
	}

private:
	std::shared_ptr<asio::ip::tcp::socket> m_sock;
	std::string m_response;
	asio::streambuf m_request;
};

class Acceptor {
public:
	Acceptor(asio::io_service& ios, unsigned int short port_num) :
		m_ios(ios), m_acceptor(ios, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port_num)), m_isStopped(false)
	{}

	void Start()
	{
		m_acceptor.listen();
		InitAccept();
	}

	void Stop()
	{
		m_isStopped.store(true);
	}
private:
	void InitAccept()
	{
		std::shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(m_ios));
		m_acceptor.async_accept(*sock.get(), [this, sock](const boost::system::error_code& error) {
			onAccept(error, sock);
			});
	}

	void onAccept(const boost::system::error_code& ec, std::shared_ptr<asio::ip::tcp::socket>sock)
	{
		if (ec.value() == 0)
		{
			std::cout << "Connect OK" << std::endl;
			(new Service(sock))->StartHanding();
		}
		else
		{
			std::cout << "Error occured! Error code = " << ec.value() << ". Message: " << ec.message();
		}

		if (!m_isStopped.load())
		{
			InitAccept();
		}
		else
		{
			m_acceptor.close();
		}
	}
private:
	asio::ip::tcp::acceptor m_acceptor;
	asio::io_service& m_ios;
	std::atomic<bool>m_isStopped;
};

class Server {
public:
	Server() {
		m_work.reset(new asio::io_service::work(m_ios));
	}

	void Start(unsigned short port_num, unsigned int thread_pool_size)
	{
		assert(thread_pool_size);

		acc.reset(new Acceptor(m_ios, port_num));
		acc->Start();

		for (unsigned int i = 0; i < thread_pool_size; i++)
		{
			std::unique_ptr<std::thread> th(new std::thread([this]() {m_ios.run(); }));
			m_thread_pool.push_back(std::move(th));
		}
	}

	void Stop()
	{
		acc->Stop();
		m_ios.stop();
		for (auto& th : m_thread_pool)
		{
			th->join();
		}
	}
private:
	asio::io_service m_ios;
	std::unique_ptr<asio::io_service::work> m_work;
	std::unique_ptr<Acceptor> acc;
	std::vector<std::unique_ptr<std::thread>> m_thread_pool;
};

const unsigned int DEFAULT_THREAD_POOL_SIZE = 2;

int main()
{
	unsigned short port_num = 27015;

	try {
		Server srv;

		unsigned intthread_pool_size = std::thread::hardware_concurrency() * 2;

		if (intthread_pool_size == 0)
		{
			intthread_pool_size = DEFAULT_THREAD_POOL_SIZE;
		}

		srv.Start(port_num, intthread_pool_size);
		
		std::this_thread::sleep_for(std::chrono::seconds(60));
		srv.Stop();
	}
	catch (system::system_error& e)
	{
		std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what();
	}
	return 0;
}
