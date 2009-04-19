/******************************************************************************

	NDIS IM 4.0 / 5.1

	��; : ARP ����ǽ

  	֧��ϵͳ : 2000 sp4 / XP / 2003

    ����  : �ž۳�

*******************************************************************************/

#include "precomp.h"
#pragma hdrstop

// ������ƭ�͹���
BOOLEAN			g_EnableGatewayCheck		= FALSE;

// IP ��ͻ����
BOOLEAN			g_EnableSameIPCheck			= FALSE;

BOOLEAN			g_EnableSendCheck			= FALSE;

//����������Ҫ�������˳���ʱ�򣬼�Ⲣ�ͷ��ڴ�
GATEWAY_ITEM*	g_Gateway_List				= NULL;
LAN_ITEM*		g_Lan_List					= NULL;
WAN_ITEM*		g_Wan_List					= NULL;


static UCHAR	Empty_MacAddress[6] = {0,0,0,0,0,0};

/************************************************************************************



************************************************************************************/

BOOLEAN
InitArpCheck()
{
	return InitGatewayCheck();
}

/************************************************************************************




************************************************************************************/

VOID RemoveAllGatewayInfo()
{
	PVOID			pVoid;
	GATEWAY_ITEM	*pGateway;

	NdisAcquireSpinLock(&GlobalLock);

	g_EnableGatewayCheck = FALSE;

	pGateway = g_Gateway_List;
	while(pGateway)
	{
		pVoid = pGateway;
		pGateway = pGateway->Next;
		ExFreePool(pVoid);
	}

	g_Gateway_List	= NULL;

	NdisReleaseSpinLock(&GlobalLock);

}

/************************************************************************************




************************************************************************************/

VOID RemoveAllLanInfo()
{
	PVOID			pVoid;
	LAN_ITEM*		pLan;

	NdisAcquireSpinLock(&GlobalLock);

	g_EnableSameIPCheck	 = FALSE;

	pLan = g_Lan_List;

	while(pLan)
	{
		pVoid	= pLan;
		pLan	= pLan->Next;
		ExFreePool(pVoid);
	}

	g_Lan_List = NULL;

	NdisReleaseSpinLock(&GlobalLock);

}

/************************************************************************************




************************************************************************************/

VOID
UnInitArpCheck()
{
	GATEWAY_ITEM	*pGateway;
	LAN_ITEM*		pLan;
	WAN_ITEM*		pWan;
	PVOID			pVoid;

	NdisAcquireSpinLock(&GlobalLock);

	//�ر����еļ����Ŀ
	g_EnableGatewayCheck = FALSE;
	g_EnableSameIPCheck	 = FALSE;

	g_EnableSendCheck = FALSE;

	pGateway = g_Gateway_List;
	while(pGateway)
	{
		pVoid = pGateway;
		pGateway = pGateway->Next;
		ExFreePool(pVoid);
	}
	
	pLan = g_Lan_List;

	while(pLan)
	{
		pVoid	= pLan;
		pLan	= pLan->Next;
		ExFreePool(pVoid);
	}

	pWan = g_Wan_List;

	while(pWan)
	{
		pVoid	= pWan;
		pWan	= pWan->Next;
		ExFreePool(pVoid);
	}

	g_Gateway_List	= NULL;
	g_Lan_List		= NULL;
	g_Wan_List		= NULL;

	UninitGatewayCheck();

	//��������ARP��¼��
	g_bRecord_ARP_Reply		= FALSE;
	if(g_Reply_Record)
	{
		g_Reply_Record = NULL;
	}

	NdisReleaseSpinLock(&GlobalLock);

}

/************************************************************************************
	// ���ձ��� ���ܵ����ֹ��� 

	// 1. ������ƭ����(����)

	// 2. IP��ַ��ͻ����
	//    a. ����IP��ַ���ͻ
	//    b. ����������IP��ַ��ͻ
	//	  c. ����IP��ַ���ͻ

************************************************************************************/

RAS_OPT
CheckPacketRecvOpt(
	PADAPTER			pAdapt,
	PNDIS_PACKET		pPacket,
	BOOLEAN				bWanPacket
	)
{
	enum RAS_OPT		RetOpt	= OPT_PASS;

	UINT				PacketSize;

	ARP_PACKET*			ArpPacket = NULL;

	GATEWAY_ITEM*		Gateway	  = NULL;

	LAN_ITEM*			LanItem	  = NULL;

	WAN_ITEM*			WanItem	  = NULL;

	ULONG				i		  = 0;

	BOOLEAN				bSameRecord	= FALSE;

	enum ATTACH_TYPE	AttachType  = ATTACH_NONE;

	ArpPacket = ExAllocatePoolWithTag(NonPagedPool,MAX_ETH_PACKET_SIZE,TAG);

	if(!ArpPacket)
	{
		goto Exit;
	}

	if(ArpPacket)
	{
		CopyPacket2Buffer(pPacket,(PUCHAR)ArpPacket,&PacketSize);

		if( ArpPacket->EthType != ETHERNET_ARP || 
			PacketSize < sizeof(ARP_PACKET)		)
		{
			goto Exit;
		}

		if( ArpPacket->OperateCode != 0x100 &&
			ArpPacket->OperateCode != 0x200 &&
			ArpPacket->OperateCode != 0x300 &&
			ArpPacket->OperateCode != 0x400 )
		{
			KdPrint((" ����ARP/RARPЭ�鹥��"));
			AttachType = WRONG_PROTOCOL_ATTACH;
			RetOpt = OPT_DROP;
			goto Exit;
		}

		//���� IP - Mac ��Ӧ��ѯ��Ľ���
		if(g_ArpFw_ShareMem)
		{
			// ��ѯ�㲥��
			if( ArpPacket->OperateCode == ARP_QUERY										&&
				NdisEqualMemory(ArpPacket->DestMacAddress,Empty_MacAddress,6)			&&
				!NdisEqualMemory(ArpPacket->SourceMacAddress,Empty_MacAddress,6)		&&
				g_ArpFw_ShareMem->ulItemCount < MAX_IP_MAC_ITEM_COUNT					)
			{

				bSameRecord = FALSE;
				for( i = 0 ; i< g_ArpFw_ShareMem->ulItemCount; i++)
				{
					if(NdisEqualMemory( g_ArpFw_ShareMem->Items[i].IPAddress,ArpPacket->SourceIPAddress,4))
					{
						bSameRecord = TRUE;
						break;
					}
				}

				//��ǰû�и�IP��ַ�ļ�¼
				if(!bSameRecord)
				{
					NdisAcquireSpinLock(&GlobalLock);

					memcpy(g_ArpFw_ShareMem->Items[g_ArpFw_ShareMem->ulItemCount].IPAddress,
							ArpPacket->SourceIPAddress,4);
					memcpy(g_ArpFw_ShareMem->Items[g_ArpFw_ShareMem->ulItemCount].MacAddress,
							ArpPacket->SourceMacAddress,6);

					g_ArpFw_ShareMem->ulItemCount ++;

					NdisReleaseSpinLock(&GlobalLock);
				}
				
			}

		}


		// ARP Reply ���ļ�¼
		if(	ArpPacket->OperateCode == ARP_REPLY									&& 
			g_bRecord_ARP_Reply													&&
			NdisEqualMemory(ArpPacket->SourceIPAddress,g_Want_ARP_Reply_IP,4)	)
		{
			bSameRecord = FALSE;

			NdisAcquireSpinLock(&GlobalLock);

			if(g_Reply_Record->ulItemCount < MAX_REPLY_RECORD)
			{
				do
				{
					if(g_Reply_Record->ulItemCount > 0)
					{
						for(i = 0 ; i < g_Reply_Record->ulItemCount; i ++)
						{
							if(NdisEqualMemory(ArpPacket->SourceMacAddress,
								g_Reply_Record->Items[i].MacAddress,6))
							{
								g_Reply_Record->Items[i].RecordCount ++;
								bSameRecord = TRUE;
								break;
							}
						}
					}

					if(!bSameRecord)
					{
						NdisMoveMemory(g_Reply_Record->Items[g_Reply_Record->ulItemCount].IPAddress,
											ArpPacket->SourceIPAddress,4);
						NdisMoveMemory(g_Reply_Record->Items[g_Reply_Record->ulItemCount].MacAddress,
											ArpPacket->SourceMacAddress,6);
						g_Reply_Record->Items[g_Reply_Record->ulItemCount].WanAddress	= pAdapt->bWanAdapter;
						g_Reply_Record->Items[g_Reply_Record->ulItemCount].Gateway		= TRUE;
						g_Reply_Record->Items[g_Reply_Record->ulItemCount].Next			= (IP_ITEM*)pAdapt;
						g_Reply_Record->Items[g_Reply_Record->ulItemCount].RecordCount	= 1;

						g_Reply_Record->ulItemCount ++;
					}
				}
				while(FALSE);
			}

			NdisReleaseSpinLock(&GlobalLock);
		}

		//���α��ARP/RARP Query�����е�ԴMac��ַ�Ƿ�Ϊ��ȷ�����ص�ַ
		if( g_EnableGatewayCheck															&&
			(ArpPacket->OperateCode == ARP_QUERY || ArpPacket->OperateCode == RARP_QUERY )   )
		{
			// ���ص�ַ���,Query�����е�Դ��ַ��ԴMAC��ַ��������ȷ��
			if(!bWanPacket)		// ���������ؼ��
			{
				NdisAcquireSpinLock(&GlobalLock);
				Gateway  = g_Gateway_List;
				while(Gateway)
				{
					if( NdisEqualMemory(ArpPacket->SourceIPAddress,Gateway->IPAddress,4)	&&
						!NdisEqualMemory(ArpPacket->SourceMacAddress,Gateway->MacAddress,6)  )
					{
						// IP��ַ��ͬ,Mac��ַ��ͬ (��ֹ�ð�����ͨ��)
						KdPrint(("α������Query��������"));
						AttachType = GATEWAY_ARP_QUERY_ATTACH;
						RetOpt = OPT_DROP;
						NdisReleaseSpinLock(&GlobalLock);
						goto Exit;					
					}

					Gateway = Gateway->Next;
				}
				NdisReleaseSpinLock(&GlobalLock);
			}
		}

		//α���ARP/RARP Reply���ļ��
		if(	g_EnableGatewayCheck														  &&
			(ArpPacket->OperateCode == ARP_REPLY || ArpPacket->OperateCode == RARP_REPLY) )
		{

			if(!bWanPacket)		// ���������ؼ��
			{
				NdisAcquireSpinLock(&GlobalLock);
				Gateway = g_Gateway_List;
				while(Gateway)
				{
					if(	NdisEqualMemory(Gateway->IPAddress,ArpPacket->SourceIPAddress,4)	&& // ������IP
						!NdisEqualMemory(Gateway->MacAddress,ArpPacket->SourceMacAddress,6)	)	// Mac ��ַ����ͬ,���ع���
					{
						KdPrint(("α������Reply��������"));
						//��ֹ�ð�����ͨ��
						AttachType = GATEWAY_ARP_REPLY_ATTACH;
						RetOpt = OPT_DROP;
						NdisReleaseSpinLock(&GlobalLock);
						goto Exit;
					}
					else if(NdisEqualMemory(Gateway->IPAddress,ArpPacket->DestIPAddress,4)		&&
							!NdisEqualMemory(Gateway->MacAddress,ArpPacket->DestMacAddress,6)	)
					{
						KdPrint(("α������Reply��������"));
						//��ֹ�ð�����ͨ��
						RetOpt = OPT_DROP;
						AttachType = GATEWAY_ARP_REPLY_ATTACH;
						NdisReleaseSpinLock(&GlobalLock);
						goto Exit;
					}

					Gateway = Gateway->Next;
				}

				NdisReleaseSpinLock(&GlobalLock);
			}
		}
		
		//���� IP ��ͻ�������
		if(	g_EnableSameIPCheck														&&
			NdisEqualMemory(ArpPacket->SourceIPAddress,ArpPacket->DestIPAddress,4)	)
		{
			NdisAcquireSpinLock(&GlobalLock);
			if(!pAdapt->bWanAdapter) // ���������
			{
				LanItem = g_Lan_List;
				while(LanItem)
				{
					// IP ��ַ��ͬ �� ԴMac ��ַ��ͬ
					if( NdisEqualMemory(ArpPacket->SourceIPAddress,LanItem->IPAddress,4) &&
						!NdisEqualMemory(ArpPacket->SourceMacAddress,LanItem->MacAddress,6) )
					{
						KdPrint(("α��������IP��ͻ��������"));
						RetOpt = OPT_DROP;
						AttachType = LAN_SAMEIP_ATTACH;
						NdisReleaseSpinLock(&GlobalLock);
						goto Exit;
					}

					LanItem = LanItem->Next;
				}
				// ����������������ͬIP����
				WanItem = g_Wan_List;
				while(WanItem)
				{
					if(NdisEqualMemory(ArpPacket->SourceIPAddress,WanItem->IPAddress,4))
					{
						KdPrint(("α����������IP��ͻ��������"));
						RetOpt = OPT_DROP;
						AttachType = WAN_SAMEIP_ATTACH;
						NdisReleaseSpinLock(&GlobalLock);
						goto Exit;
					}
					WanItem = WanItem->Next;
				}
			}
			NdisReleaseSpinLock(&GlobalLock);
		}

	}

Exit:
	if(ArpPacket)
	{
		if(g_ArpFw_ShareMem && AttachType != ATTACH_NONE)	
		{
			g_ArpFw_ShareMem->NotifyPacket.AttachCount = 1;
			g_ArpFw_ShareMem->NotifyPacket.AttachType  = AttachType;
			g_ArpFw_ShareMem->NotifyPacket.SendPacket  = FALSE;
			g_ArpFw_ShareMem->NotifyPacket.WanPacket   = pAdapt->bWanAdapter;

			RtlCopyMemory((PVOID)&g_ArpFw_ShareMem->NotifyPacket.ArpPacket,
							ArpPacket,sizeof(ARP_PACKET));

			SetUserShareEvent(&g_NotifyEvent);

		}

		ExFreePool(ArpPacket);
		ArpPacket = NULL;
	}

	return RetOpt;
}

/************************************************************************************





************************************************************************************/

enum RAS_OPT
OldRecvPacketOpt(
	PADAPTER				pAdapt,
    IN  PVOID               HeaderBuffer,
    IN  UINT                HeaderBufferSize,
    IN  PVOID               LookAheadBuffer,
    IN  UINT                LookAheadBufferSize,
    IN  UINT                PacketSize)
{

	enum RAS_OPT RetOpt	= OPT_PASS;

	ARP_PACKET			*ArpPacket = NULL;

	GATEWAY_ITEM*		Gateway	  = NULL;

	LAN_ITEM*			LanItem	  = NULL;

	WAN_ITEM*			WanItem	  = NULL;

	ETH_HEADER*			EthHeader = HeaderBuffer;

	ULONG				i		  = 0;

	BOOLEAN				bSameRecord	= FALSE;

	enum ATTACH_TYPE	AttachType  = ATTACH_NONE;

	//����Ƿ��� ARP ����
	if( EthHeader->EthType != ETHERNET_ARP)
	{
		goto OldRecvExit;
	}

	ArpPacket = ExAllocatePoolWithTag(NonPagedPool,MAX_ETH_PACKET_SIZE,TAG);

	if(ArpPacket)
	{
		memcpy(ArpPacket,HeaderBuffer,14);
		memcpy(((PUCHAR)ArpPacket) + 14 ,LookAheadBuffer,LookAheadBufferSize);

		if( ArpPacket->OperateCode != 0x100 &&
			ArpPacket->OperateCode != 0x200 &&
			ArpPacket->OperateCode != 0x300 &&
			ArpPacket->OperateCode != 0x400 )
		{
			KdPrint((" ����ARP/RARPЭ�鹥��"));
			AttachType = WRONG_PROTOCOL_ATTACH;
			RetOpt = OPT_DROP;
			goto OldRecvExit;
		}

		//���� IP - Mac ��Ӧ��ѯ��Ľ���
		if(g_ArpFw_ShareMem)
		{
			// ��ѯ�㲥��
			if( ArpPacket->OperateCode == ARP_QUERY										&&
				NdisEqualMemory(ArpPacket->DestMacAddress,Empty_MacAddress,6)			&&
				!NdisEqualMemory(ArpPacket->SourceMacAddress,Empty_MacAddress,6)		&&
				g_ArpFw_ShareMem->ulItemCount < MAX_IP_MAC_ITEM_COUNT					)
			{

				bSameRecord = FALSE;
				for( i = 0 ; i< g_ArpFw_ShareMem->ulItemCount; i++)
				{
					if(NdisEqualMemory( g_ArpFw_ShareMem->Items[i].IPAddress,ArpPacket->SourceIPAddress,4))
					{
						bSameRecord = TRUE;
						break;
					}
				}
	
				//��ǰû�и�IP��ַ�ļ�¼
				if(!bSameRecord)
				{
					NdisAcquireSpinLock(&GlobalLock);

					memcpy(g_ArpFw_ShareMem->Items[g_ArpFw_ShareMem->ulItemCount].IPAddress,
							ArpPacket->SourceIPAddress,4);
					memcpy(g_ArpFw_ShareMem->Items[g_ArpFw_ShareMem->ulItemCount].MacAddress,
							ArpPacket->SourceMacAddress,6);

					g_ArpFw_ShareMem->ulItemCount ++;

					NdisReleaseSpinLock(&GlobalLock);
				}
				
			}
		}

		// ARP Reply ���ļ�¼
		if(	ArpPacket->OperateCode == ARP_REPLY									&& 
			g_bRecord_ARP_Reply													&&
			NdisEqualMemory(ArpPacket->SourceIPAddress,g_Want_ARP_Reply_IP,4)	)
		{
			bSameRecord = FALSE;

			NdisAcquireSpinLock(&GlobalLock);

			if(g_Reply_Record->ulItemCount < MAX_REPLY_RECORD)
			{
				do
				{
					if(g_Reply_Record->ulItemCount > 0)
					{
						for(i = 0 ; i < g_Reply_Record->ulItemCount; i ++)
						{
							if(NdisEqualMemory(ArpPacket->SourceMacAddress,
								g_Reply_Record->Items[i].MacAddress,6))
							{
								g_Reply_Record->Items[i].RecordCount ++;
								bSameRecord = TRUE;
								break;
							}
						}
					}

					if(!bSameRecord)
					{
						NdisMoveMemory(g_Reply_Record->Items[g_Reply_Record->ulItemCount].IPAddress,
											ArpPacket->SourceIPAddress,4);
						NdisMoveMemory(g_Reply_Record->Items[g_Reply_Record->ulItemCount].MacAddress,
											ArpPacket->SourceMacAddress,6);
						g_Reply_Record->Items[g_Reply_Record->ulItemCount].WanAddress	= pAdapt->bWanAdapter;
						g_Reply_Record->Items[g_Reply_Record->ulItemCount].Gateway		= TRUE;
						g_Reply_Record->Items[g_Reply_Record->ulItemCount].Next			= (IP_ITEM*)pAdapt;
						g_Reply_Record->Items[g_Reply_Record->ulItemCount].RecordCount	= 1;

						g_Reply_Record->ulItemCount ++;
					}
				}
				while(FALSE);
			}

			NdisReleaseSpinLock(&GlobalLock);
		}

		//���α��ARP/RARP Query�����е�ԴMac��ַ�Ƿ�Ϊ��ȷ�����ص�ַ
		if( g_EnableGatewayCheck															&&
			(ArpPacket->OperateCode == ARP_QUERY || ArpPacket->OperateCode == RARP_QUERY )   )
		{
			// ���ص�ַ���,Query�����е�Դ��ַ��ԴMAC��ַ��������ȷ��
			if(!pAdapt->bWanAdapter)		// ���������ؼ��
			{
				NdisAcquireSpinLock(&GlobalLock);
				Gateway  = g_Gateway_List;
				while(Gateway)
				{
					if( NdisEqualMemory(ArpPacket->SourceIPAddress,Gateway->IPAddress,4)	&&
						!NdisEqualMemory(ArpPacket->SourceMacAddress,Gateway->MacAddress,6)  )
					{
						// IP��ַ��ͬ,Mac��ַ��ͬ (��ֹ�ð�����ͨ��)
						KdPrint(("α������Query��������"));
						AttachType = GATEWAY_ARP_QUERY_ATTACH;
						RetOpt = OPT_DROP;
						NdisReleaseSpinLock(&GlobalLock);
						goto OldRecvExit;					
					}

					Gateway = Gateway->Next;
				}
				NdisReleaseSpinLock(&GlobalLock);
			}
		}

		//α���ARP/RARP Reply���ļ��
		if(	g_EnableGatewayCheck														  &&
			(ArpPacket->OperateCode == ARP_REPLY || ArpPacket->OperateCode == RARP_REPLY) )
		{

			if(!pAdapt->bWanAdapter)		// ���������ؼ��
			{
				NdisAcquireSpinLock(&GlobalLock);
				Gateway = g_Gateway_List;
				while(Gateway)
				{
					if(	NdisEqualMemory(Gateway->IPAddress,ArpPacket->SourceIPAddress,4)	&& // ������IP
						!NdisEqualMemory(Gateway->MacAddress,ArpPacket->SourceMacAddress,6)	)	// Mac ��ַ����ͬ,���ع���
					{
						KdPrint(("α������Reply��������"));
						//��ֹ�ð�����ͨ��
						AttachType = GATEWAY_ARP_REPLY_ATTACH;
						RetOpt = OPT_DROP;
						NdisReleaseSpinLock(&GlobalLock);
						goto OldRecvExit;
					}
					else if(NdisEqualMemory(Gateway->IPAddress,ArpPacket->DestIPAddress,4)		&&
							!NdisEqualMemory(Gateway->MacAddress,ArpPacket->DestMacAddress,6)	)
					{
						KdPrint(("α������Reply��������"));
						//��ֹ�ð�����ͨ��
						RetOpt = OPT_DROP;
						AttachType = GATEWAY_ARP_REPLY_ATTACH;
						NdisReleaseSpinLock(&GlobalLock);
						goto OldRecvExit;
					}

					Gateway = Gateway->Next;
				}

				NdisReleaseSpinLock(&GlobalLock);
			}
		}
		
		//���� IP ��ͻ�������
		if(	g_EnableSameIPCheck														&&
			NdisEqualMemory(ArpPacket->SourceIPAddress,ArpPacket->DestIPAddress,4)	)
		{
			NdisAcquireSpinLock(&GlobalLock);
			if(!pAdapt->bWanAdapter) // ���������
			{
				LanItem = g_Lan_List;
				while(LanItem)
				{
					// IP ��ַ��ͬ �� ԴMac ��ַ��ͬ
					if( NdisEqualMemory(ArpPacket->SourceIPAddress,LanItem->IPAddress,4) &&
						!NdisEqualMemory(ArpPacket->SourceMacAddress,LanItem->MacAddress,6) )
					{
						KdPrint(("α��������IP��ͻ��������"));
						RetOpt = OPT_DROP;
						AttachType = LAN_SAMEIP_ATTACH;
						NdisReleaseSpinLock(&GlobalLock);
						goto OldRecvExit;
					}

					LanItem = LanItem->Next;
				}
				// ����������������ͬIP����
				WanItem = g_Wan_List;
				while(WanItem)
				{
					if(NdisEqualMemory(ArpPacket->SourceIPAddress,WanItem->IPAddress,4))
					{
						KdPrint(("α����������IP��ͻ��������"));
						RetOpt = OPT_DROP;
						AttachType = WAN_SAMEIP_ATTACH;
						NdisReleaseSpinLock(&GlobalLock);
						goto OldRecvExit;
					}
					WanItem = WanItem->Next;
				}
			}
			NdisReleaseSpinLock(&GlobalLock);
		}

	}

OldRecvExit:
	if(ArpPacket)
	{
		if(g_ArpFw_ShareMem && AttachType != ATTACH_NONE)	
		{
			g_ArpFw_ShareMem->NotifyPacket.AttachCount = 1;
			g_ArpFw_ShareMem->NotifyPacket.AttachType  = AttachType;
			g_ArpFw_ShareMem->NotifyPacket.SendPacket  = FALSE;
			g_ArpFw_ShareMem->NotifyPacket.WanPacket   = pAdapt->bWanAdapter;

			RtlCopyMemory((PVOID)&g_ArpFw_ShareMem->NotifyPacket.ArpPacket,
							ArpPacket,sizeof(ARP_PACKET));

			SetUserShareEvent(&g_NotifyEvent);

		}

		ExFreePool(ArpPacket);
		ArpPacket = NULL;
	}


	return RetOpt;

}


RAS_OPT
CheckPacketSendOpt(
	PADAPTER			pAdapt,
	PNDIS_PACKET		pPacket,
	BOOLEAN				bWanPacket)
{

	enum RAS_OPT RetOpt	 = OPT_PASS;

	UINT				PacketSize;

	ARP_PACKET			*ArpPacket = NULL;

	//��Ȿ�����ͳ��ı���
//	if(!g_EnableSendCheck)	goto Exit;

	ArpPacket = ExAllocatePoolWithTag(NonPagedPool,MAX_ETH_PACKET_SIZE,TAG);

	if(!ArpPacket)
	{
		goto Exit;
	}

	// ����Ƿ���ARP Response����,��������Query����
	CopyPacket2Buffer(pPacket,(PUCHAR)ArpPacket,&PacketSize);

	if( ArpPacket->EthType != ETHERNET_ARP)
	{
		goto Exit;
	}

	if( g_bRecord_ARP_Reply						&&
		ArpPacket->OperateCode == ARP_QUERY		&&
		NdisEqualMemory(ArpPacket->DestIPAddress,g_Want_ARP_Reply_IP,4)	)
	{
		NdisAcquireSpinLock(&GlobalLock);
		g_Reply_Record->ulQueryCount ++;
		BeginCheckGateway();
		NdisReleaseSpinLock(&GlobalLock);
	}

Exit:
	if(ArpPacket)
	{
		ExFreePool(ArpPacket);
		ArpPacket = NULL;
	}

	return RetOpt;
}

/**************************************************************************************




**************************************************************************************/

VOID CopyPacket2Buffer(
		IN PNDIS_PACKET		pPacket,
		IN OUT PUCHAR		pBuff,
		IN OUT PUINT		pLength) 
{ 
	PNDIS_BUFFER BuffDT;
	PUCHAR BuffVA;
	UINT BuffLen;	
	*pLength = 0;
	BuffLen=0; 

	NdisQueryPacket(pPacket,NULL,NULL,&BuffDT,NULL); 

	while(BuffDT!=(PNDIS_BUFFER)NULL) 
	{ 
		NdisQueryBufferSafe(BuffDT,&BuffVA,&BuffLen,NormalPagePriority);   
		memcpy(pBuff,BuffVA,BuffLen);   
		pBuff		=	pBuff+BuffLen;   
		*pLength	+=	BuffLen;
		NdisGetNextBuffer(BuffDT,&BuffDT); 
	} 

	return; 
} 


/***********************************************************************************************


************************************************************************************************/

void MyFreeNdisSendPacket(PNDIS_PACKET p)
{
	PNDIS_BUFFER b;

	while(1) 
	{
		NdisUnchainBufferAtFront(p, &b);
		if(b != NULL) 
		{
			NdisFreeBuffer(b);
		} else 
		{
			break;
		}
	}
	NdisFreePacket(p);
}

UINT GetPacketSize(PNDIS_PACKET p)
{
	UINT	PacketSize  = 0;
	if(p)
	{
		NdisQueryPacket(p,NULL,NULL,NULL,&PacketSize);
	}
	return PacketSize;
}