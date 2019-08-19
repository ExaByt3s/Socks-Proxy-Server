#ifndef __TRANSFER_H__
#define __TRANSFER_H__

#include"Epoll.h"

class TransferServer : public EpollServer
{
public:
	TransferServer(const char* socks5Ip, int socks5Port, int selfPort= 8000)
		:EpollServer(selfPort)
	{
		memset(&_socks5addr, 0, sizeof(struct sockaddr_in));
		_socks5addr.sin_family = AF_INET;
		_socks5addr.sin_port = htons(socks5Port);
		_socks5addr.sin_addr.s_addr = inet_addr(socks5Ip);
	}

	int AuthHandle(int fd);
	int EstablishmentHandle(int fd);

	virtual void ConnectEventHandle(int fd);
	virtual void ReadEventHandle(int fd);
	//virtual void WriteEventHandle(int fd);

protected:
	struct sockaddr_in _socks5addr;    //socks5  server addr
};

#endif //__TRANSFER_H__