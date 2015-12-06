#include "stdafx.h"
#include "../Common/Common.h"
#include "../packet/Category.h"
#include "../packet/logingate_protocol.h"

DWORD WINAPI	ThreadFuncForMsg(LPVOID lpParameter);

BOOL			CheckSocketError(LPARAM lParam);

VOID WINAPI		OnTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

void			OnCommand(WPARAM wParam, LPARAM lParam);
BOOL			InitThread(LPTHREAD_START_ROUTINE lpRoutine);

extern SOCKET					g_csock;

extern HWND						g_hMainWnd;
extern HWND						g_hStatusBar;
extern HWND						g_hToolBar;

HANDLE							g_hMsgThread = INVALID_HANDLE_VALUE;

CWHQueue						g_xMsgQueue;

extern CRandom					g_pRandom;

static char	WorkBuff[8192];
static int	nWorkBuffLen;

LPARAM OnClientSockMsg(WPARAM wParam, LPARAM lParam)
{
	switch (WSAGETSELECTEVENT(lParam))
	{
		case FD_CONNECT:
		{
			DWORD dwThreadIDForMsg = 0;

			if (CheckSocketError(lParam))
			{
				dwThreadIDForMsg = 0;

				if (InitThread(ThreadFuncForMsg))
				{
					KillTimer(g_hMainWnd, _ID_TIMER_CONNECTSERVER);
				
					SetTimer(g_hMainWnd, _ID_TIMER_KEEPALIVE, 5000, (TIMERPROC)OnTimerProc);

					InsertLogMsg(IDS_CONNECT_LOGINSERVER);
					SendMessage(g_hStatusBar, SB_SETTEXT, MAKEWORD(1, 0), (LPARAM)_TEXT("Connected"));
				}
			}
			else
			{
				closesocket(g_csock);
				g_csock = INVALID_SOCKET;

				SetTimer(g_hMainWnd, _ID_TIMER_CONNECTSERVER, 10000, (TIMERPROC)OnTimerProc);
			}

			break;
		}
		case FD_CLOSE:
		{
			closesocket(g_csock);
			g_csock = INVALID_SOCKET;

			//OnCommand(IDM_STOPSERVICE, 0);
			//等待连接
			SetTimer(g_hMainWnd, _ID_TIMER_CONNECTSERVER, 10000, (TIMERPROC)OnTimerProc);

			break;
		}
		case FD_READ:
		{
			int		nSocket = 0;
			char	*pszFirst = NULL, *pszEnd = NULL;

			UINT nRecv = 0;

			ioctlsocket((SOCKET)wParam, FIONREAD, (u_long *)&nRecv);

			if (nRecv)
			{
				char *pszPacket = new char[nRecv + 1];

				nRecv = recv((SOCKET)wParam, pszPacket, nRecv, 0);

				pszPacket[nRecv] = '\0';

				if (!(g_xMsgQueue.PushQ((BYTE *)pszPacket)))
					InsertLogMsg(_TEXT("[INFO] Not enough queue(g_xMsgQueue) buffer."));
			}

			break;
		}
	}

	return 0L;
}

//logingateto
void SendExToServer(Packet*pPacket)
{
	if (pPacket == NULL)
		return;

	 int nLen = pPacket->dlen;

	if (nLen >= TCP_PACKET_SIZE)
		return;

	uint16 crc = g_pRandom.Random_Int(0,65535);//CrcHelper::GetCrc16(pPacket->data,nLen);
	pPacket->crc = crc;

	//发送给loginSrv更新
	DWORD	dwSendBytes;
	WSABUF	buf;

	buf.len = pPacket->dlen + pPacket->hlen;
	buf.buf = (char*)pPacket;

	pPacket->dlen = crc ^ pPacket->dlen;

	if ( WSASend(g_csock, &buf, 1, &dwSendBytes, 0, NULL, NULL) == SOCKET_ERROR )
	{
		int nErr = WSAGetLastError();
	}
}

void SendExToServer( uint8 Category,uint8 protocol )
{
	BuildCmdPacketEx(pPacket,Category,protocol,buffer,64);

	int nLen = pPacket->dlen;

	if (nLen >= TCP_PACKET_SIZE)
		return;

	uint16 crc = g_pRandom.Random_Int(0,65535);
	pPacket->crc = crc;

	//发送给loginSrv更新
	DWORD	dwSendBytes;
	WSABUF	buf;

	buf.len = pPacket->dlen + pPacket->hlen;
	buf.buf = (char*)pPacket;

	pPacket->dlen = crc ^ pPacket->dlen;

	if ( WSASend(g_csock, &buf, 1, &dwSendBytes, 0, NULL, NULL) == SOCKET_ERROR )
	{
		int nErr = WSAGetLastError();
	}
}