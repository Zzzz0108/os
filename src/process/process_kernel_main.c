#include "../../inc/process_interface.h"
#include "../../inc/process_process.h"
#include "../../inc/mem.h"

/* ���Խ���1 */
void process_A()
{
    while (1)
    {
        // ģ������
        for (int i = 0; i < 1000000; i++);
    }
}

/* ���Խ���2 */
void process_B()
{
    while (1)
    {
        for (int i = 0; i < 1000000; i++);
    }
}

/* �ں���� */
int main()
{
    // 【新增】：系统开机时最先初始化 16 页物理内存池
    if (init_memory_system(16) != 0) {
        os_panic("Memory system initialization failed!");
    }
    
    /* �����������Խ��� */
    os_create_process(process_A);
    os_create_process(process_B);

    /* �ں�ѭ�� */
    while (1)
    {
        os_yield();   // �����ó�CPU
    }

    return 0;
}