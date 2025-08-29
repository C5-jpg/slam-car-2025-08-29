#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

// 设置串口属性
int set_serial_attributes(int fd, int speed) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    // 忽略调制解调器控制线，启用接收器
    tty.c_cflag &= ~CSIZE;             // 清除所有数据位设置
    tty.c_cflag |= CS8;                // 8位数据位
    tty.c_cflag &= ~PARENB;            // 无奇偶校验
    tty.c_cflag &= ~CSTOPB;            // 1位停止位
    tty.c_cflag &= ~CRTSCTS;           // 无硬件流控

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 1;   // 非阻塞读取，至少读取1个字符
    tty.c_cc[VTIME] = 5;  // 等待字符的超时时间（0.5秒）

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

// 打印十六进制数据
void print_hex(const char *prefix, const unsigned char *data, int len) {
    printf("%s", prefix);
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

// 验证YDLIDAR X2数据包
void validate_ydlidar_x2_packet(const unsigned char *buffer, int buffer_len) {
    printf("\n=== YDLIDAR X2数据包验证 ===\n");
    
    // 查找包头 (0xAA 0x55)
    int start_idx = -1;
    for (int i = 0; i < buffer_len - 1; ++i) {
        if (buffer[i] == 0xAA && buffer[i+1] == 0x55) {
            start_idx = i;
            break;
        }
    }
    
    if (start_idx == -1) {
        printf("错误：未找到YDLIDAR X2包头 (0xAA 0x55)\n");
        print_hex("接收到的数据：", buffer, buffer_len);
        return;
    }
    
    printf("找到包头位置：%d\n", start_idx);
    
    // 检查最小包长度
    if (buffer_len < start_idx + 10) {
        printf("错误：数据包不完整，需要至少10字节\n");
        return;
    }
    
    // 解析数据包头部
    printf("数据包头部信息：\n");
    printf("  包头 (PH): 0x%02X 0x%02X\n", buffer[start_idx], buffer[start_idx+1]);
    printf("  包类型 (CT): 0x%02X\n", buffer[start_idx+2]);
    printf("  采样数量 (LSN): %d\n", buffer[start_idx+3]);
    
    // 解析起始角和结束角
    unsigned short fsa_raw = (buffer[start_idx+5] << 8) | buffer[start_idx+4];
    unsigned short lsa_raw = (buffer[start_idx+7] << 8) | buffer[start_idx+6];
    printf("  起始角 (FSA): 0x%04X\n", fsa_raw);
    printf("  结束角 (LSA): 0x%04X\n", lsa_raw);
    
    // 计算角度值
    float start_angle = (float)(fsa_raw >> 1) / 64.0f;
    float end_angle = (float)(lsa_raw >> 1) / 64.0f;
    printf("  起始角度值: %.2f°\n", start_angle);
    printf("  结束角度值: %.2f°\n", end_angle);
    
    // 解析校验码
    unsigned short cs_raw = (buffer[start_idx+9] << 8) | buffer[start_idx+8];
    printf("  校验码 (CS): 0x%04X\n", cs_raw);
    
    // 计算校验和
    unsigned short calculated_cs = 0;
    for (int i = 0; i < 8; i++) {
        calculated_cs ^= ((unsigned short *)buffer)[start_idx/2 + i];
    }
    printf("  计算校验和: 0x%04X\n", calculated_cs);
    
    if (cs_raw == calculated_cs) {
        printf("  校验结果：✓ 校验通过\n");
    } else {
        printf("  校验结果：✗ 校验失败\n");
    }
    
    // 解析采样数据
    int sample_count = buffer[start_idx+3];
    int packet_len = 10 + sample_count * 2;
    
    printf("采样数据信息：\n");
    printf("  采样点数: %d\n", sample_count);
    printf("  数据包长度: %d\n", packet_len);
    
    if (buffer_len < start_idx + packet_len) {
        printf("错误：数据包不完整，需要%d字节，实际只有%d字节\n", 
               packet_len, buffer_len - start_idx);
        return;
    }
    
    // 解析前5个采样点
    printf("  前5个采样点数据：\n");
    for (int i = 0; i < 5 && i < sample_count; i++) {
        int data_offset = start_idx + 10 + i * 2;
        unsigned short distance_raw = (buffer[data_offset+1] << 8) | buffer[data_offset];
        float distance = (float)distance_raw / 4.0f;
        printf("    点%d: 原始值=0x%04X, 距离=%.2fmm\n", i+1, distance_raw, distance);
    }
    
    // 计算角度步长
    float angle_step = (end_angle - start_angle) / (sample_count - 1);
    printf("  角度步长: %.4f°\n", angle_step);
    
    // 计算前5个点的角度
    printf("  前5个点的角度：\n");
    for (int i = 0; i < 5 && i < sample_count; i++) {
        float current_angle = start_angle + angle_step * i;
        int data_offset = start_idx + 10 + i * 2;
        unsigned short distance_raw = (buffer[data_offset+1] << 8) | buffer[data_offset];
        float distance = (float)distance_raw / 4.0f;
        
        // 计算角度修正
        float angle_correction = 0.0f;
        if (distance > 0.0f) {
            angle_correction = atan(21.8 * (155.3 - distance) / (155.3 * distance)) * 180.0f / M_PI;
        }
        float corrected_angle = current_angle + angle_correction;
        
        printf("    点%d: 原始角度=%.2f°, 修正角度=%.2f°, 修正值=%.2f°\n", 
               i+1, current_angle, corrected_angle, angle_correction);
    }
    
    printf("=== 数据包验证完成 ===\n\n");
}

// 查找并分析所有可能的数据包
void analyze_ydlidar_x2_stream(const unsigned char *buffer, int buffer_len) {
    printf("\n=== YDLIDAR X2数据流分析 ===\n");
    
    int packet_count = 0;
    int pos = 0;
    
    while (pos < buffer_len - 1) {
        // 查找包头
        if (buffer[pos] == 0xAA && buffer[pos+1] == 0x55) {
            printf("发现第%d个数据包，位置：%d\n", ++packet_count, pos);
            
            // 检查是否有足够的数据
            if (pos + 4 < buffer_len) {
                int sample_count = buffer[pos+3];
                int packet_len = 10 + sample_count * 2;
                
                printf("  采样点数：%d，数据包长度：%d\n", sample_count, packet_len);
                
                if (pos + packet_len <= buffer_len) {
                    // 验证这个数据包
                    validate_ydlidar_x2_packet(buffer + pos, packet_len);
                    pos += packet_len;
                } else {
                    printf("  数据包不完整，跳过\n");
                    pos += 2;
                }
            } else {
                printf("  数据不足，跳过\n");
                pos += 2;
            }
        } else {
            pos++;
        }
    }
    
    printf("=== 数据流分析完成，共发现%d个数据包 ===\n", packet_count);
}

int main(int argc, char *argv[]) {
    const char* portname = "/dev/ttyUSB0";
    if (argc > 1) {
        portname = argv[1];
    }
    
    printf("YDLIDAR X2数据包验证程序\n");
    printf("使用串口：%s\n", portname);
    printf("按Ctrl+C退出程序\n\n");
    
    // 打开串口
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "错误：无法打开串口 %s: %s\n", portname, strerror(errno));
        return 1;
    }
    
    // 设置串口参数
    if (set_serial_attributes(fd, B115200) != 0) {
        close(fd);
        return 1;
    }
    
    printf("串口配置成功，开始接收数据...\n");
    
    unsigned char buffer[8192];
    int buffer_len = 0;
    int packet_count = 0;
    
    while (1) {
        // 读取数据
        int bytes_read = read(fd, buffer + buffer_len, sizeof(buffer) - buffer_len);
        if (bytes_read > 0) {
            buffer_len += bytes_read;
            printf("读取到%d字节数据，缓冲区大小：%d\n", bytes_read, buffer_len);
            
            // 尝试查找并验证数据包
            if (buffer_len >= 10) {
                analyze_ydlidar_x2_stream(buffer, buffer_len);
                
                // 清空缓冲区
                buffer_len = 0;
                packet_count++;
                
                // 每10个数据包后暂停一下
                if (packet_count >= 10) {
                    printf("\n已分析10个数据包，暂停5秒...\n");
                    sleep(5);
                    packet_count = 0;
                }
            }
        } else if (bytes_read < 0) {
            fprintf(stderr, "读取错误：%s\n", strerror(errno));
            sleep(1);
        }
    }
    
    close(fd);
    return 0;
}