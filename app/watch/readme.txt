���Ź�ģ��

�����ߣ�	Fanlv
���ڣ�		2004-11-19	ver�� 1.3

���ܣ�

����Ӧ�����������������򣬶�ʱ��RTC(Ӳ��ʱ��)ˢ�µ�ϵͳʱ�ӣ����Ҷ�
ʱ��ϵͳ���и�λ�� ��ʱ��λ���Ź�Ӳ��(ι��)����������Ź�ģ�鲻��ι��ʱ��
ϵͳ�Ḵλ���������Է�ֹ����������

���Ź�ģ��������ʱ�ᶨʱ��ⱻ�������ĳ����״̬�������������ģʽ����
�����������У����Ź������������ó��������������ģʽ���˳���λϵ
ͳ�����Ź�ģ��������б��������ĳ�����SIGTERM�źţ�Լ30���λϵͳ��

Ϊ��ϵͳ���ȶ����У����Ź�ģ����Զ�ʱ��λϵͳ����λϵͳ��ʱ�䵥λ���졣
�����λϵͳʱ��Ϊ�㣬��ʾ����λϵͳ��

Ӧ�ó����������ģʽ��Ӧ�ó����������������ʱ��λϵͳ��ʱ���ǿ����õġ�
���Ź�ģ����������ͨ�����������ļ�(Ĭ���ļ���
/home/et1000/watchdog.conf)�еĲ���.ȷ����������
�������÷�ʽ�μ�watchdog.conf 

Ӧ������������ֳ�����ģʽ��ֻ����һ�Σ��˳����������У��˳���λϵͳ��

Bug revise
Date 2005-2-5		ver: 1.4
bug description: when the RTC time is invalid, the SyncTime() failed close the
RTC file descriptor before return it's caller, and so that other programs
could not reopen the RTC device.
Revision:
	to close the file descriptor when return to the caller
Author: Fanlvu
 
