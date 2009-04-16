
#ifndef _SHARE_STRUCT_H
#define _SHARE_STRUCT_H

#if _MSC_VER > 1000
#pragma once
#endif

// ����¼RARP/ARP��Ӧ��
#define		MAX_REPLY_RECORD					100

//���ڼ�¼�����������л��������ص�ַ�Ĳ�ѯ�����ļ�¼�����ڼ�����˭�ڹ���������
#define		MAX_IP_MAC_ITEM_COUNT				1024

enum ATTACH_TYPE
{
	ATTACH_NONE,			   // �޹�����Ϊ
	GATEWAY_ARP_QUERY_ATTACH,  // α�����ز�ѯ������
	GATEWAY_ARP_REPLY_ATTACH,  // α�����ػ�Ӧ������
	LAN_SAMEIP_ATTACH,		   // ������IP��ͻ����
	WAN_SAMEIP_ATTACH,		   // ������IP��ͻ����
	WRONG_PROTOCOL_ATTACH,	   // �����Э�鹥��
	UNKNOWN_ATTACH,

};

//#pragma pack ( push , 1 )

typedef struct _ARP_PACKET
{
    UCHAR       DstAddr[6];
    UCHAR       SrcAddr[6];
    USHORT      EthType;				// ��̫��Э�� ARP

	//ARP Packet
	USHORT		HardwareType;			// 0x1				// 2
	USHORT		ProtocolType;			// ntohs(0x800)		// 4

	UCHAR		HardwareSize;			// 6 Mac ��ַ����	// 5
	UCHAR		ProtocolSize;			// 4 IPV4����		// 6

	USHORT		OperateCode;			// 1 Query  , 2 Repley	//	8

	UCHAR		SourceMacAddress[6];	// �������������ַ		//	14
	UCHAR		SourceIPAddress[4];		// ��������Դ��ַ		//	18

	UCHAR		DestMacAddress[6];		// Ŀ�ĵ�ַ(Query ����ʱȫΪ0,RepleyʱΪ����������ַ	// 24
	UCHAR		DestIPAddress[4];		// Ŀ��IP��ַ			//  28

} ARP_PACKET;

typedef struct _NOTIFY_PACKET
{
	struct _NOTIFY_PACKET*	Next;				// ��һ����
	PVOID				pAdapt;					// ���ĸ��������İ�
	BOOLEAN				SendPacket;				// �Ƿ��Ǳ������ͳ�ȥ�İ�
	BOOLEAN				WanPacket;				// �Ƿ���Wan�ڰ�
	enum ATTACH_TYPE	AttachType;				// ��������
	ULONG				AttachCount;			// �������Ĵ���

	// ARP ������
    ARP_PACKET			ArpPacket;

} NOTIFY_PACKET;

typedef struct _IP_ITEM
{
	struct _IP_ITEM*	Next;
	ULONG				RecordCount;	// �ڵȴ���ʱ�����յ� Reply���ĵĸ���
	ULONG				QueryCount;		// �ڵȴ���ʱ����ϵͳ���� Query���ĵĸ���,����α��ARP���ؼ����
	BOOLEAN				WanAddress;		// �Ƿ��ǹ�������ַ
	BOOLEAN				Gateway;		// �Ƿ������ص�ַ
	UCHAR				IPAddress[4];	// ������
	UCHAR				MacAddress[6];	// Mac Address

} IP_ITEM;

typedef struct _REPLY_RECORD
{
	ULONG		ulQueryCount;	//�ڵȴ���ʱ���ڣ�ִ���˼�����ѯ������
	ULONG		ulItemCount;
	IP_ITEM		Items[MAX_REPLY_RECORD];
} REPLY_RECORD,*PREPLY_RECORD;

typedef struct _IP_MAC_ITEM
{
	UCHAR				IPAddress[4];	// ������
	UCHAR				MacAddress[6];	// Mac Address
} IP_MAC_ITEM;

typedef struct _ARPFW_SHARE_MEM
{
	HANDLE			NotifyEvent;
	ULONG			ulItemCount;
	IP_MAC_ITEM		Items[MAX_IP_MAC_ITEM_COUNT];
	NOTIFY_PACKET	NotifyPacket;
	//ִ�в�ѯʱ���Ǵ��ں��Զ�ʶ�����������MAC��ַ
	REPLY_RECORD	Replay;
} ARPFW_SHARE_MEM,*PARPFW_SHARE_MEM;

//#pragma pack ( pop , 1 )

#endif

