
/******************************************************************************

	NDIS Filter 6.0

	��; : ARP ����ǽ

  	֧��ϵͳ : Vista / Server 2008

    ����  : �ž۳�

*******************************************************************************/


#define MAX_ETH_PACKET_SIZE		1600

#define		ETHERNET_ARP	0x608		// ��������

#define		ARP_QUERY		0x100		// ��������
#define		ARP_REPLY		0x200		// ��������

#define		RARP_QUERY		0x300		// ��������
#define		RARP_REPLY		0x400		// ��������

typedef struct _ETH_HEADER
{
    UCHAR       DstAddr[6];
    UCHAR       SrcAddr[6];
    USHORT      EthType;				// ��̫��Э�� ARP
} ETH_HEADER;


// �������ص�ַ�������б�
typedef struct	_GATEWAY_ITEM
{
	struct	_GATEWAY_ITEM*		Before;
	struct	_GATEWAY_ITEM*		Next;

	UCHAR						IPAddress[4];
	UCHAR						MacAddress[6];

} GATEWAY_ITEM;


typedef struct	_LAN_ITEM
{
	struct	_LAN_ITEM*			Before;
	struct	_LAN_ITEM*			Next;

	UCHAR						IPAddress[4];
	UCHAR						MacAddress[6];

} LAN_ITEM;

typedef struct	_WAN_ITEM
{
	struct	_WAN_ITEM*			Before;
	struct	_WAN_ITEM*			Next;

	UCHAR						IPAddress[4];
	UCHAR						MacAddress[6];

} WAN_ITEM;

