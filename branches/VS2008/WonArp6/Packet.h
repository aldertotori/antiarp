
/******************************************************************************

	NDIS IM 4.0 / 5.1

	��; : ARP ����ǽ

  	֧��ϵͳ : 2000 sp4 / XP / 2003

    ����  : �ž۳�

*******************************************************************************/

#ifndef _PACKET_H
#define _PACKET_H
#if _MSC_VER > 1000
#pragma once
#endif

#ifndef htons
#define htons(x) (((((unsigned short)(x)) >> 8) & 0xff) | ((((unsigned short)(x)) & 0xff) << 8))
#define ntohs(x) (((((unsigned short)(x)) >> 8) & 0xff) | ((((unsigned short)(x)) & 0xff) << 8))
#endif

typedef enum _RAS_OPT
{
	OPT_PASS,		//���а�ͨ��
	OPT_DROP		//��ֹ��ͨ��
} RAS_OPT;


BOOLEAN	
InitArpCheck();

VOID
UnInitArpCheck();

VOID RemoveAllGatewayInfo();

VOID RemoveAllLanInfo();

extern BOOLEAN		g_EnableSendCheck;
extern BOOLEAN		g_EnableSameIPCheck;
extern BOOLEAN		g_EnableGatewayCheck;

//����������Ҫ�������˳���ʱ�򣬼�Ⲣ�ͷ��ڴ�
extern GATEWAY_ITEM*	g_Gateway_List	;
extern LAN_ITEM*		g_Lan_List		;
extern WAN_ITEM*		g_Wan_List		;


#endif

