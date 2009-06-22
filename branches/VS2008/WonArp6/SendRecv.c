
/******************************************************************************

	NDIS Filter 6.0

	��; : ARP ����ǽ

  	֧��ϵͳ : Vista / Server 2008

    ����  : �ž۳�

*******************************************************************************/

#include "precomp.h"

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


VOID
FilterReceiveNetBufferLists(
        IN  NDIS_HANDLE         FilterModuleContext,
        IN  PNET_BUFFER_LIST    NetBufferLists,
        IN  NDIS_PORT_NUMBER    PortNumber,
        IN  ULONG               NumberOfNetBufferLists,
        IN  ULONG               ReceiveFlags
         )
/*++

Routine Description:

    FilerReceiveNetBufferLists is an optional function for filter drivers. 
    If provided, this function process receive indications made by underlying 
    NIC or lower level filter drivers. This function  can also be called as a 
    result of loopback. If this handler is NULL, NDIS will skip calling this
    filter when processing a receive indication and will call the next upper 
    filter in the stack with a non-NULL FitlerReceiveNetBufferLists handler 
    or the procotol driver. A filter that doesn't provide a 
    FilterReceiveNetBufferLists handler can not provided a 
    FilterReturnNetBufferLists handler or a initiate a receive indication on 
    its own.

Arguments:

    FilterModuleContext: Pointer to our filter context area.
    NetBufferLists: A linked list of NetBufferLists allocated by underlying driver each containing
                    one NetBuffer.
    PortNumber: Port on which the Receive is indicated
    ReceiveFlags: Flags associated with the Receive such as whether the filter
                  can pend the receive
   

Return Value:

    None
 
--*/
{

    PMS_FILTER          pFilter = (PMS_FILTER)FilterModuleContext;
    NDIS_STATUS         ReturnStatus = NDIS_STATUS_SUCCESS;
    PNET_BUFFER_LIST    NextNetBufferList;
    BOOLEAN             DispatchLevel;
    ULONG               ReturnFlags;
    ULONG               Ref;

	// ++
	PNET_BUFFER_LIST	CurrentBufferList	= NULL;
	PNET_BUFFER			CurrentBuffer		= NULL;
	PNET_BUFFER_DATA	CurrentBufferData	= NULL;
	BOOLEAN				HaveARPPacket		= FALSE;
	PMDL				PacketMdl			= NULL;
	ULONG				DataOffset			= 0;
	ULONG				PacketSize			= 0;
	PUCHAR				PacketData			= NULL;
	ARP_PACKET*			ArpPacket			= NULL;
	GATEWAY_ITEM*		Gateway				= NULL;
	LAN_ITEM*			LanItem				= NULL;
	WAN_ITEM*			WanItem				= NULL;
	ULONG				i					= 0;
	BOOLEAN				bSameRecord			= FALSE;
	enum ATTACH_TYPE	AttachType			= ATTACH_NONE;
	enum RAS_OPT		RetOpt				= OPT_PASS;
	BOOLEAN				bWanAdapter			= FALSE;
    // --

    DEBUGP(DL_TRACE, ("===>ReceiveNetBufferList: NetBufferLists = %p.\n", NetBufferLists));

    do
    {

        DispatchLevel = NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags);

#if DBG
        
        FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);
 
        if (pFilter->State != FilterRunning)
        {
            FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);

            if (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags))
            {   
                ReturnFlags = 0;
                if (NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags))
                {
                    NDIS_SET_RETURN_FLAG(ReturnFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
                }
                
                NdisFReturnNetBufferLists(pFilter->FilterHandle, NetBufferLists, ReturnFlags);
            }
            break;
        }
        FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
#endif

        ASSERT(NumberOfNetBufferLists >= 1);

		//  ++
		CurrentBufferList = NetBufferLists;

		while(CurrentBufferList)
		{
			// Each NET_BUFFER structure packages a packet of network data
			CurrentBuffer = NET_BUFFER_LIST_FIRST_NB(CurrentBufferList);

			while(CurrentBuffer)
			{
				// ��������Ƿ���ARPЭ���
				PacketMdl		= NET_BUFFER_FIRST_MDL(CurrentBuffer);
				DataOffset		= NET_BUFFER_DATA_OFFSET(CurrentBuffer);
				PacketSize	= NET_BUFFER_DATA_LENGTH(CurrentBuffer);
				
				if(PacketMdl && PacketSize)
				{
					PacketData = (UCHAR*)MmGetSystemAddressForMdlSafe(PacketMdl,NormalPagePriority);
					if(PacketData)
					{
						if(DataOffset)
						{
							PacketData = PacketData + DataOffset;
						}
						// PacketData ����������ݣ�PacketSize ����������ݳ���

						do
						{
							ArpPacket = (ARP_PACKET*)PacketData;

							if( ArpPacket->EthType != ETHERNET_ARP || 
								PacketSize < sizeof(ARP_PACKET)		)
							{
								break;
							}
							else
							{
								DEBUGP(DL_TRACE,("ReceiveNetBufferList:�յ�ARP���ݰ�...\n"));
							}

							if( ArpPacket->OperateCode != 0x100 &&
								ArpPacket->OperateCode != 0x200 &&
								ArpPacket->OperateCode != 0x300 &&
								ArpPacket->OperateCode != 0x400 )
							{
								DEBUGP(DL_TRACE,(" ����ARP/RARPЭ�鹥��"));
								AttachType = WRONG_PROTOCOL_ATTACH;
								RetOpt = OPT_DROP;
								goto Exit;
							}

							//���� IP - Mac ��Ӧ��ѯ��Ľ���
							NdisAcquireSpinLock(&GlobalLock);
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
										memcpy(g_ArpFw_ShareMem->Items[g_ArpFw_ShareMem->ulItemCount].IPAddress,
												ArpPacket->SourceIPAddress,4);
										memcpy(g_ArpFw_ShareMem->Items[g_ArpFw_ShareMem->ulItemCount].MacAddress,
												ArpPacket->SourceMacAddress,6);

										g_ArpFw_ShareMem->ulItemCount ++;
										
									}	
								}
							}
							NdisReleaseSpinLock(&GlobalLock);

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
											g_Reply_Record->Items[g_Reply_Record->ulItemCount].WanAddress	= bWanAdapter;
											g_Reply_Record->Items[g_Reply_Record->ulItemCount].Gateway		= TRUE;
											g_Reply_Record->Items[g_Reply_Record->ulItemCount].Next			= NULL;
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
								if(!bWanAdapter)		// ���������ؼ��
								{
									NdisAcquireSpinLock(&GlobalLock);
									Gateway  = g_Gateway_List;
									while(Gateway)
									{
										if( NdisEqualMemory(ArpPacket->SourceIPAddress,Gateway->IPAddress,4)	&&
											!NdisEqualMemory(ArpPacket->SourceMacAddress,Gateway->MacAddress,6)  )
										{
											// IP��ַ��ͬ,Mac��ַ��ͬ (��ֹ�ð�����ͨ��)
											DEBUGP(DL_TRACE,("α������Query��������"));
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
							if(	g_EnableGatewayCheck &&
								(ArpPacket->OperateCode == ARP_REPLY || ArpPacket->OperateCode == RARP_REPLY) )
							{

								if(!bWanAdapter)		// ���������ؼ��
								{
									NdisAcquireSpinLock(&GlobalLock);
									Gateway = g_Gateway_List;

									while(Gateway)
									{
										if(	NdisEqualMemory(Gateway->IPAddress,ArpPacket->SourceIPAddress,4)	&& // ������IP
											!NdisEqualMemory(Gateway->MacAddress,ArpPacket->SourceMacAddress,6)	)	// Mac ��ַ����ͬ,���ع���
										{
											DEBUGP(DL_TRACE,("α������Reply��������"));
											//��ֹ�ð�����ͨ��
											AttachType = GATEWAY_ARP_REPLY_ATTACH;
											RetOpt = OPT_DROP;
											NdisReleaseSpinLock(&GlobalLock);
											goto Exit;
										}
										else if(NdisEqualMemory(Gateway->IPAddress,ArpPacket->DestIPAddress,4)		&&
												!NdisEqualMemory(Gateway->MacAddress,ArpPacket->DestMacAddress,6)	)
										{
											DEBUGP(DL_TRACE,("α������Reply��������"));
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
								if(!bWanAdapter) // ���������
								{
									LanItem = g_Lan_List;
									while(LanItem)
									{
										// IP ��ַ��ͬ �� ԴMac ��ַ��ͬ
										if( NdisEqualMemory(ArpPacket->SourceIPAddress,LanItem->IPAddress,4) &&
											!NdisEqualMemory(ArpPacket->SourceMacAddress,LanItem->MacAddress,6) )
										{
											DEBUGP(DL_TRACE,("α��������IP��ͻ��������"));
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
											DEBUGP(DL_TRACE,("α����������IP��ͻ��������"));
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
						while(FALSE);
					}
				}
				CurrentBuffer = NET_BUFFER_NEXT_NB(CurrentBuffer);
			}
			CurrentBufferList = NET_BUFFER_LIST_NEXT_NBL(CurrentBufferList);
		}
		//  --
            
        // 
        // If necessary, queue the NetBufferList in a local structure for later processing.
        // We may need to travel the list, some of them may not need post processing
        //
        if (pFilter->TrackReceives)
        {
            FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);
            pFilter->OutstandingRcvs += NumberOfNetBufferLists;
            Ref = pFilter->OutstandingRcvs;
            
            FILTER_LOG_RCV_REF(1, pFilter, NetBufferLists, Ref);
            FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
        }

		DEBUGP(DL_TRACE,(" NdisFIndicateReceiveNetBufferLists Run "));

        NdisFIndicateReceiveNetBufferLists(
                   pFilter->FilterHandle,
                   NetBufferLists,
                   PortNumber, 
                   NumberOfNetBufferLists,
                   ReceiveFlags);


        if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags) && 
            pFilter->TrackReceives)
        {
            FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);
            pFilter->OutstandingRcvs -= NumberOfNetBufferLists;
            Ref = pFilter->OutstandingRcvs;
            FILTER_LOG_RCV_REF(2, pFilter, NetBufferLists, Ref);
            FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
        }

		// ++
		break;

Exit:
		KdPrint((" Drop Received Packet "));
		if(ArpPacket)
		{
			NdisAcquireSpinLock(&GlobalLock);

			if(g_ArpFw_ShareMem && AttachType != ATTACH_NONE)	
			{
				g_ArpFw_ShareMem->NotifyPacket.AttachCount = 1;
				g_ArpFw_ShareMem->NotifyPacket.AttachType  = AttachType;
				g_ArpFw_ShareMem->NotifyPacket.SendPacket  = FALSE;
				g_ArpFw_ShareMem->NotifyPacket.WanPacket   = FALSE;

				RtlCopyMemory((PVOID)&g_ArpFw_ShareMem->NotifyPacket.ArpPacket,
								ArpPacket,sizeof(ARP_PACKET));

				SetUserShareEvent(&g_NotifyEvent);

			}
			NdisReleaseSpinLock(&GlobalLock);
		}

		// return this packet
		if (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags))
		{
			ReturnFlags = 0;
			if (NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags))
			{
				NDIS_SET_RETURN_FLAG(ReturnFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
			}
			NdisFReturnNetBufferLists(pFilter->FilterHandle, NetBufferLists, ReturnFlags);
		}
		// --

    } while (FALSE);
    
    DEBUGP(DL_TRACE, ("<===ReceiveNetBufferList: Flags = %8x.\n", ReceiveFlags));
    
}





VOID
FilterSendNetBufferLists(
        IN  NDIS_HANDLE         FilterModuleContext,
        IN  PNET_BUFFER_LIST    NetBufferLists,
        IN  NDIS_PORT_NUMBER    PortNumber,
        IN  ULONG               SendFlags
        )
/*++
 
Routine Description:

    Send Net Buffer List handler
    This function is an optional function for filter drivers. If provided, NDIS
    will call this function to transmit a linked list of NetBuffers, described by a 
    NetBuferList, over the network. If this handler is NULL, NDIS will skip calling
    this fitler when sending a NetBufferList and will call the next lower fitler 
    in the stack with a non_NULL FilterSendNetBufferList handleror the miniport driver.
    A filter that doesn't provide a FilerSendNetBufferList handler can not initiate a 
    send o its own.

Arguments:

    FilterModuleContext: Pointer to our filter context area.
    NetBufferLists: Pointer to a List of NetBufferLists.
    PortNumber - Port Number to which this send is targetted
    SendFlags-  Specifies if the call is at DISPATCH_LEVEL                     
  

Return Value:
 
    NDIS_STATUS_SUCCESS: 
    NDIS_STATUS_PENDING:
    NDIS_STATUS_INVALID_PACKET:
    NDIS_STATUS_RESOURCES:
    NDIS_STATUS_FAILURE:


NOTE: The filter will act like a passthru filter.       
 
--*/
{
    PMS_FILTER          pFilter = (PMS_FILTER)FilterModuleContext;
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    PNET_BUFFER_LIST    CurrNbl;
    BOOLEAN             DispatchLevel;

	// ++
	PNET_BUFFER_LIST	CurrentBufferList	= NULL;
	PNET_BUFFER			CurrentBuffer		= NULL;
	PNET_BUFFER_DATA	CurrentBufferData	= NULL;
	PMDL				PacketMdl			= NULL;
	ULONG				DataOffset			= 0;
	ULONG				PacketSize			= 0;
	PUCHAR				PacketData			= NULL;
	ARP_PACKET*			ArpPacket			= NULL;
	BOOLEAN				bWanAdapter			= FALSE;
    // --

    
    DEBUGP(DL_TRACE, ("===>SendNetBufferList: NBL = %p.\n", NetBufferLists));

    do
    {

       DispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags);
#if DBG
        //
        // we should never get packets to send if we are not in running state
        //
        
        FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);
        //
        // If the filter is not in running state, fail the send
        // 
        if (pFilter->State != FilterRunning)
        {
            FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
            
            CurrNbl = NetBufferLists;

            while (CurrNbl)
            {
                NET_BUFFER_LIST_STATUS(CurrNbl) = NDIS_STATUS_PAUSED;
                CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
            }

            NdisFSendNetBufferListsComplete(pFilter->FilterHandle, 
                        NetBufferLists, 
                        DispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0);
            break;
            
        }
        FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
#endif

		//  ++
		CurrentBufferList = NetBufferLists;

		while(CurrentBufferList)
		{
			// Each NET_BUFFER structure packages a packet of network data
			CurrentBuffer = NET_BUFFER_LIST_FIRST_NB(CurrentBufferList);
			while(CurrentBuffer)
			{
				// ��������Ƿ���ARPЭ���
				PacketMdl		= NET_BUFFER_FIRST_MDL(CurrentBuffer);
				DataOffset		= NET_BUFFER_DATA_OFFSET(CurrentBuffer);
				PacketSize	= NET_BUFFER_DATA_LENGTH(CurrentBuffer);
				
				if(PacketMdl && PacketSize)
				{
					PacketData = (UCHAR*)MmGetSystemAddressForMdlSafe(PacketMdl,NormalPagePriority);
					if(PacketData)
					{
						if(DataOffset)
						{
							PacketData = PacketData + DataOffset;
						}
						// PacketData ����������ݣ�PacketSize ����������ݳ���

						DEBUGP(DL_TRACE,(" PacketData : %p , PacketSize : %d ",PacketData,PacketSize));

						ArpPacket = (ARP_PACKET*)PacketData;

						// ��¼���ػ�Ӧ��ѯ����
						NdisAcquireSpinLock(&GlobalLock);
						if( ArpPacket->EthType == ETHERNET_ARP)
						{
							if( g_bRecord_ARP_Reply						&&
								ArpPacket->OperateCode == ARP_QUERY		&&
								NdisEqualMemory(ArpPacket->DestIPAddress,g_Want_ARP_Reply_IP,4)	)
							{
								g_Reply_Record->ulQueryCount ++;
								//��ʼ��¼���ز�ѯ����
								BeginCheckGateway();
							}
						}
						NdisReleaseSpinLock(&GlobalLock);

					}
				}
				CurrentBuffer = NET_BUFFER_NEXT_NB(CurrentBuffer);
			}
			CurrentBufferList = NET_BUFFER_LIST_NEXT_NBL(CurrentBufferList);
		}
		// --


        if (pFilter->TrackSends)
        {
            FILTER_ACQUIRE_LOCK(&pFilter->Lock, DispatchLevel);
            CurrNbl = NetBufferLists;

            while (CurrNbl)
            {
                pFilter->OutstandingSends++;
                FILTER_LOG_SEND_REF(1, pFilter, CurrNbl, pFilter->OutstandingSends);
                
                CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
            }
            FILTER_RELEASE_LOCK(&pFilter->Lock, DispatchLevel);
        }
        
        // If necessary, queue the NetBufferList in a local structure for later processing
        //
        NdisFSendNetBufferLists(pFilter->FilterHandle, NetBufferLists, PortNumber, SendFlags);        
        
    }while (FALSE);
    
    DEBUGP(DL_TRACE, ("<===SendNetBufferList: Status = %8x.\n", Status));
}


