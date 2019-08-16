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
	
	//�������񣬿�ʼ����
	void Start();

	//�¼�ѭ��
	void EventLoop();

	//���麯�������������д
	virtual void ConnectEventHandle(int fd) = 0;
	virtual void ReadEventHandle(int fd) = 0;
	virtual void WriteEventHandle(int fd) = 0;

private:
	//������
	EpollServer(const EpollServer&);
	EpollServer& operator=(const EpollServer&);

protected:
	int _listenfd;   //�����׽���
	int _port;       //����˿�

	int _eventfd;    //�¼�������
	static const size_t _MAX_EVENT; //����¼�����
};

#endif //__EPOLL_H__