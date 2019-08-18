#include"Epoll.h"
const size_t EpollServer::_MAX_EVENT = 100000;

//�������񣬿�ʼ����
void EpollServer::Start()
{
	_listenfd = socket(AF_INET,SOCK_STREAM,0);
	if (_listenfd == -1)
	{
		ErrorDebug("create socket error");
		return;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	//����������any����
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(_port);

	if (bind(_listenfd, (const struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1)
	{
		ErrorDebug("bind socket error");
		return;
	}
	if (listen(_listenfd, 100000) == -1)
	{
		ErrorDebug("listen socket error");
		return;
	}

	//event
	_eventfd = epoll_create(_MAX_EVENT);
	if (_eventfd == -1)
	{
		ErrorDebug("epoll_create error");
		return;
	}

	//����׽���
	OPEvent(_listenfd, EPOLLIN, EPOLL_CTL_ADD);

	//epoll��ʼ����
	EventLoop();
}

//�¼�ѭ��
void EpollServer::EventLoop()
{
	//�¼�����
	struct epoll_event events[_MAX_EVENT]; 
	while (1)
	{
		int size = epoll_wait(_eventfd, events, _MAX_EVENT, -1);
		if (size == -1)
		{
			ErrorDebug("epoll_wait error");
		}
		for (int i = 0; i < size; ++i)
		{
			if (events[i].data.fd == _listenfd)
			{

				struct sockaddr_in addr;
				socklen_t len = sizeof(addr);
				int connectfd = accept(_listenfd,(struct sockaddr*)&addr, &len);
				if (connectfd == -1)
				{
					ErrorDebug("accept error");
					//continue;
				}
				//TraceDebug("client connect");
				ConnectEventHandle(connectfd);
			}
			else if (events[i].events & EPOLLIN)
			{
				
				ReadEventHandle(events[i].data.fd);
			}
			else if (events[i].events & EPOLLOUT)
			{
				WriteEventHandle(events[i].data.fd);
			}
			else
			{
				ErrorDebug("events error");
			}
		}	
	}
}

//����
//void Forwording(Channel* clientChannel, Channel* serverChannel)
//{
//	const size_t buflen = 4096;
//	char buf[buflen];
//	int rlen = recv(clientChannel->_fd, buf, buflen);
//	if (rlen > 0)
//	{
//		int slen = send(serverChannel->_fd, buf, rlen);
//		if (slen < rlen)//��������һ����û�з���ȥ
//		{
//			SendInLoop()
//		}	
//	}
//	else if (rlen == 0)
//}