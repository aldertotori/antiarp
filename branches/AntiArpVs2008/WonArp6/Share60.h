
/******************************************************************************

	NDIS Filter 6.0

	��; : ARP ����ǽ

  	֧��ϵͳ : Vista / Server 2008

    ����  : �ž۳�

*******************************************************************************/

#ifndef SHARE6_H
#define	SHARE6_H

#ifndef FILE_DEVICE_PHYSICAL_NETCARD
#define FILE_DEVICE_PHYSICAL_NETCARD		0x00000017
#endif

#include "../Share/ShareStruct.h"

#define _NDIS_CONTROL_CODE(request,method) \
            CTL_CODE(FILE_DEVICE_PHYSICAL_NETCARD, request, method, FILE_ANY_ACCESS)

// ����IP��Ϣ
#define IOCTL_ADDIPINFO_60					_NDIS_CONTROL_CODE(0x1, METHOD_BUFFERED)
// �Ƴ�����IP��Ϣ	
#define	IOCTL_REMOVE_ALL_LAN_INFO_60		_NDIS_CONTROL_CODE(0x2, METHOD_BUFFERED)
// �Ƴ����ر�����Ϣ
#define IOCTL_REMOVE_ALL_GATEWAY_INFO_60	_NDIS_CONTROL_CODE(0x3, METHOD_BUFFERED)
// ��ʼ��IP��ַ��ѯ��Ӧ��MAC��ַ
#define IOCTL_BEGIN_ARP_QUERY_60			_NDIS_CONTROL_CODE(0x4, METHOD_BUFFERED)
// ������IP��ַ��ѯ��Ӧ��MAC��ַ
//#define IOCTL_END_ARP_QUERY_60				_NDIS_CONTROL_CODE(0x5, METHOD_BUFFERED)
// ��ȡ��¼�������ػ�Ӧ����Ϣ
//#define IOCTL_READ_REPLY_RECORD_60			_NDIS_CONTROL_CODE(0x6, METHOD_BUFFERED)
// �����հ��������
#define IOCTL_ENABLE_GATEWAY_CHECK_60		_NDIS_CONTROL_CODE(0x7, METHOD_BUFFERED)
// ����IP��ͻ�������
#define IOCTL_ENABLE_SAMEIP_CHECK_60		_NDIS_CONTROL_CODE(0x9, METHOD_BUFFERED)
// �����հ��������
#define IOCTL_DISABLE_GATEWAY_CHECK_60		_NDIS_CONTROL_CODE(0xA, METHOD_BUFFERED)
// ����IP��ͻ�������
#define IOCTL_DISABLE_SAMEIP_CHECK_60		_NDIS_CONTROL_CODE(0xC, METHOD_BUFFERED)
//���ù������
#define IOCTL_SET_SHARE_MEMORY_60 			_NDIS_CONTROL_CODE(0xF, METHOD_IN_DIRECT)
//����������
#define IOCTL_CLEAR_SHARE_MEMORY_60 		_NDIS_CONTROL_CODE(0x10, METHOD_IN_DIRECT)





#endif

