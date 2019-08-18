#ifndef __EPOLL_H__
#define __EPOLL_H__

#include"Header.h"

class EpollServer
{
public:
	EpollServer(int port)
		: _listenfd(-1)
		, _port(port)
		, _eventfd(-1)
	{}

	virtual ~EpollServer()
	{
		if (_listenfd != -1)
			close(_listenfd);
	}

	//epoll_ctl�¼�
	void OPEvent(int fd, int events, int how)
	{
		struct epoll_event ev;
		ev.events = events;
		ev.data.fd = fd;
		if (epoll_ctl(_eventfd, how, fd, &ev) < 0 )
		{
			ErrorDebug("epoll_ctl.fd:&d + how:%d", fd, how);
		}
	}

	//���÷�����
	void SetNonblocking(int fd)
	{
		int flags, s;
		flags = fcntl(fd, F_GETFL, 0);
		if (flags == -1)
			ErrorDebug("SetNonblocking:F_GETFL");
		flags |= O_NONBLOCK;
		s = fcntl(fd, F_SETFL, flags);
		if (s == -1)
			ErrorDebug("SetNonblocking : F_SETFL");
	}
	
	//�������񣬿�ʼ����
	void Start();

	//�¼�ѭ��
	void EventLoop();

	//���麯�������������д
	virtual void ConnectEventHandle(int fd) = 0;
	virtual void ReadEventHandle(int fd) = 0;
	virtual void WriteEventHandle(int fd) = 0;

	//����
	//void Forwording(Channel* clientChannel, Channel* serverChannel);

	//״̬
	enum Socks5State
	{
		AUTH,            //�����֤
		ESTABLISHMENT,	 //��������
		FORWARDING,		 //ת��
	};

	//ͨ��    --����δ��ȫ���ܵ�����
	struct Channel
	{
		int _fd;		//������
		int _event;	    //�¼�
		string _buffer; //������

		Channel()
			: _fd(-1)
			, _event(0)
		{}
	};

	//����
	struct Connect
	{
		Socks5State _state;		 //���ӵ�״̬
		Channel _clientChannel;  //�ͻ���ͨ��
		Channel _serverChannel;  //������ͨ��
		int _ref;

		Connect()
			:_state(AUTH)
			, _ref(0)
		{}
	};

private:
	//������
	EpollServer(const EpollServer&);
	EpollServer& operator=(const EpollServer&);

protected:
	int _listenfd;   //�����׽���
	int _port;       //����˿�

	int _eventfd;    //�¼�������
	static const size_t _MAX_EVENT; //����¼�����

	map<int, Connect*> _fdConnectMap;  //fdӳ�����ӵ�map����
};

#endif //__EPOLL_H__