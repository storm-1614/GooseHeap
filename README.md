# GooseHeap OS

GooseHeap(鹅堆) OS 一个运行在 Arduino UNO (ATmega328P ) 上的两个任务轮转的最简 RTOS。  
使用裸机 C + avr-libc 开发。  

> 最开始：  
> 在 Arduino UNO(ATmega328P) 拿裸机 C 语言在 avr-libc 的基础上，基于 UART 做了个串口 Shell。  
> 未来可能慢慢做成类似 FreeRTOS 那样的吧，最大憧憬。  

慢慢有了 RTOS 雏形。初步实现两个任务的调度。  
## 开发环境
我是用的 ArchLinux，其他发行版我让 chatGPT 写的……  

ArchLinux:  
```
sudo pacman -S avr-gcc avr-libc avrdude make picocom
```

Debian / Ubuntu:  
```
sudo apt update
sudo apt install gcc-avr avr-libc binutils-avr avrdude make
```

Windows 下感觉很麻烦，没探索。  

## 已完成
- AVR 裸机 C 开发
    * 不依赖 Arduino Framework
    * 直接通过 avr-libc 写寄存器操作硬件
    * 实现 GPIO 初始化控制
- UART 驱动
    * 9600 band, 8N1
    * 基于 USART_RX 和 USART_UDRE 中断的搜发
    * RX/TX 环形缓冲区
- 系统定时器
    * 使用 Timer1
    * 每 1 ms 产生一次定时器中断
    * 维护 32 位全局系统 tick
- UART 交互式 Shell
    * 支持基于行的命令输入与回显
    * 支持命令参数解析
- 任务切换
    * 独立任务栈
    * 初始化、保存、恢复 CPU context
    * 最简的 `os_start_first`, `os_yield()`  
    * 两任务循环调度

## 计划


