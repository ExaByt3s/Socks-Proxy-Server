#include"Sock5.h"

void Sock5Server::ConnectEventHandle(int connectfd)
{
	//�������Ӽ��ͻ��˷���ͨ��
	Connect* con = new Connect;
	con->_clientChannel._fd = connectfd;
	con->_clientChannel._event = EPOLLIN;
	con->_state = AUTH;
	con->_ref++;
	_fdConnectMap[connectfd] = con;
	
	//���connectfd��epoll���������¼�
	SetNonblocking(connectfd);
	OPEvent(connectfd, EPOLLIN, EPOLL_CTL_ADD);
}

//AUTH --�����֤
//0 ��ʾ����û���������ȴ�
//1 ��ʾ�ɹ�
//-1 ʧ��
int Sock5Server::AuthHandle(int fd)
{
	char buf[260];
	int rlen = recv(fd, buf, 260, MSG_PEEK);

	if (rlen < 3)
		return 0;
	else if (rlen <= 0)
	{
		return -1;
	}
	else
	{
		recv(fd, buf, rlen, 0);
		Decrypt(buf, rlen);
		if (buf[0] != 0x05)//socks5��ʽ����
		{
			ErrorDebug("not socks5");
			return -1;
		}
		return 1;
	}
}

//�������� ��������
//-1       ʧ�� 
//-2      ����û��
//serverfd  �ɹ�
int Sock5Server::EstablishmentHandle(int fd)
{
	char buf[256];
	int rlen = recv(fd, buf, 256, MSG_PEEK);
	if (rlen <= 0)
	{
		return -1;
	}
	else if (rlen < 10)
	{
		return -2;
	}
	else
	{
		char ip[4];
		char port[2];

		recv(fd, buf, 4, 0);
		Decrypt(buf, 4);

		char addresstype = buf[3];
		if (addresstype == 0x01)//ipv4
		{
			recv(fd, ip, 4, 0);
			Decrypt(ip, 4);

			recv(fd, port, 2, 0);
			Decrypt(port, 2);
		}
		else if (addresstype == 0x03)//domainname
		{
			char len = 0;
			//recv  domainname
			recv(fd, &len, 1, 0);
			Decrypt(&len,1);
			recv(fd, buf, len, 0);
			buf[len] = '\0';
			TraceDebug("encry domainname:%s", buf);
			
			Decrypt(buf, len);
			//recv port
			recv(fd, port, 2, 0);
			Decrypt(port, 2);
			TraceDebug("Decrypt domainname:%s", buf);

			struct hostent* hostptr = gethostbyname(buf);
			memcpy(ip, hostptr->h_addr, hostptr->h_length);
		}
		else if (addresstype == 0x04) //ipv6
		{
			ErrorDebug("not support ipv6");
			return -1;
		}
		else
		{
			ErrorDebug("invalid address type error");
			return -1;
		}

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr.s_addr, ip, 4);
		addr.sin_port = *((uint16_t*)port);

		int serverfd = socket(AF_INET, SOCK_STREAM, 0);
		if (serverfd < 0)
		{
			ErrorDebug("server socket error");
			return -1;
		}
		if (connect(serverfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		{
			ErrorDebug("connect error");
			close(serverfd);
			return -1;
		}
		return serverfd;
	}
}

void Sock5Server::ReadEventHandle(int connectfd)
{
	TraceDebug("recv event:%d", connectfd);
	map<int, Connect*>::iterator it = _fdConnectMap.find(connectfd);
	if (it != _fdConnectMap.end())
	{
		Connect* con = it->second;
		if (con->_state == AUTH)
		{
			char reply[2];  //�ظ��ֽ�
			reply[0] = 0x05;
			int ret = AuthHandle(connectfd);//�����֤Auth
			if (ret == 0)
			{
				return;
			}
			else if (ret == 1)
			{
				reply[1] = 0x00;
				con->_state = ESTABLISHMENT;
			}
			else if (ret == -1)
			{
				reply[1] = 0xFF;
				RemoveConnect(connectfd);
			}

			Encry(reply, 2);
			if (send(connectfd, reply, 2, 0) != 2)
			{
				ErrorDebug("auth reply");
			}
		}
		else if (con->_state == ESTABLISHMENT)
		{
			//�ظ�
			char reply[10] = { 0 };
			reply[0] = 0x05;

			int serverfd = EstablishmentHandle(connectfd);
			if (serverfd == -1)
			{
				reply[1] = 0x01;
				RemoveConnect(connectfd);
			}
			else if (serverfd == -2)
			{
				return;
			}
			else
			{
				reply[1] = 0x00;
				reply[3] = 0x01;
			}

			Encry(reply, 10);
			if (send(connectfd, reply, 10, 0) != 10)
			{
				ErrorDebug("Establishment reply error");
			}

			if (serverfd  >= 0)
			{
				SetNonblocking(serverfd);
				OPEvent(serverfd, EPOLLIN, EPOLL_CTL_ADD);
				con->_serverChannel._fd = serverfd;
				_fdConnectMap[serverfd] = con;
				con->_ref++;
				con->_state = FORWARDING;
			}
		}
		else if (con->_state == FORWARDING)
		{
			Channel* clientChannel = &con->_clientChannel;
			Channel* serverChannel = &con->_serverChannel;

			bool sendencry = false, recvdecrypt = true;
			if (connectfd == serverChannel->_fd)
			{
				swap(clientChannel, serverChannel);
				swap(sendencry, recvdecrypt);
			}
				
			//client  ->  server 
			Forwarding(clientChannel, serverChannel, sendencry, recvdecrypt);
		}
		else
		{
			assert(false);
		}
	}
	else
	{
		assert(false);
	}
}

int main()
{
	Sock5Server server(8001);
	server.Start();
}