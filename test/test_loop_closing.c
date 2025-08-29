#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "CoreSLAM.h"

#define TEST_SCAN_SIZE 360 

// 串口屬性設置函數 (保持不變)
int set_serial_attributes(int fd, int speed) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) { fprintf(stderr, "FATAL ERROR: from tcgetattr: %s\n", strerror(errno)); return -1; }
    cfsetospeed(&tty, (speed_t)speed); cfsetispeed(&tty, (speed_t)speed);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE; tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB; tty.c_cflag &= ~CSTOPB; tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 0; tty.c_cc[VTIME] = 1;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) { fprintf(stderr, "FATAL ERROR: from tcsetattr: %s\n", strerror(errno)); return -1; }
    return 0;
}

// 數據包解析函數 (已修正指針BUG)
int parse_and_fill_sensor_data(int fd, ts_sensor_data_t *sensor_data){
    static unsigned char buffer[4096];
    static int buffer_len = 0;
    static int current_scan_points = 0;
    static ts_sensor_data_t accumulating_data;
    int bytes_read = read(fd, buffer + buffer_len, sizeof(buffer) - buffer_len);
    if (bytes_read <= 0) return -1;
    buffer_len += bytes_read;
    int start_idx = -1;
    for (int i = 0; i < buffer_len - 1; ++i) { if (buffer[i] == 0xAA && buffer[i+1] == 0x55) { start_idx = i; break; } }
    if (start_idx == -1) return -1;
    if (buffer_len < start_idx + 10) return -1;
    unsigned char packet_type = buffer[start_idx + 2] & 0x01;
    int sample_count = buffer[start_idx + 3];
    int packet_len = 10 + sample_count * 2;
    if (buffer_len < start_idx + packet_len) return -1;
    if (packet_type == 1) {
        if (current_scan_points > 50) {
            accumulating_data.scan_size = current_scan_points;
            memcpy(sensor_data, &accumulating_data, sizeof(ts_sensor_data_t));
            memset(&accumulating_data, 0, sizeof(ts_sensor_data_t));
            current_scan_points = 0;
            for (int i = 0; i < sample_count; ++i) {
                if (current_scan_points >= TEST_SCAN_SIZE) break;
                // ########## 核心BUG修正 ##########
                // 正確地解引用指針，讀取真實的距離值
                unsigned short distance_raw = *(unsigned short*)&buffer[start_idx + 10 + i * 2];
                accumulating_data.d[current_scan_points++] = distance_raw / 4;
            }
            memmove(buffer, buffer + start_idx + packet_len, buffer_len - (start_idx + packet_len));
            buffer_len -= (start_idx + packet_len);
            return 0;
        } else {
            memset(&accumulating_data, 0, sizeof(ts_sensor_data_t));
            current_scan_points = 0;
        }
    }
    for (int i = 0; i < sample_count; ++i) {
        if (current_scan_points >= TEST_SCAN_SIZE) break;
        // ########## 核心BUG修正 ##########
        // 正確地解引用指針，讀取真實的距離值
        unsigned short distance_raw = *(unsigned short*)&buffer[start_idx + 10 + i * 2];
        accumulating_data.d[current_scan_points++] = distance_raw / 4;
    }
    memmove(buffer, buffer + start_idx + packet_len, buffer_len - (start_idx + packet_len));
    buffer_len -= (start_idx + packet_len);
    return -1;
}

int main(int argc, char *argv[])
{
    ts_sensor_data_t sensor_data;
    ts_state_t state;
    ts_robot_parameters_t params;
    ts_laser_parameters_t laser_params;

    ts_map_t *map = (ts_map_t *)malloc(sizeof(ts_map_t));
    ts_map_init(map);
    ts_map_t *empty_map = (ts_map_t *)malloc(sizeof(ts_map_t));
    ts_map_init(empty_map);

    params.r = 32.5;
    params.R = 85.0;
    params.inc = 1000;
    params.ratio = 1.0;
    
    laser_params.offset = 0;
    laser_params.scan_size = TEST_SCAN_SIZE;
    laser_params.angle_min = -180;
    laser_params.angle_max = 180;
    laser_params.detection_margin = 10;
    laser_params.distance_no_detection = 8000;

    ts_position_t start_pos = {TS_MAP_SIZE / 2 * TS_MAP_SCALE, TS_MAP_SIZE / 2 * TS_MAP_SCALE, 0};
    
    memset(&state, 0, sizeof(ts_state_t)); 

    // 根據分析建議，適當調整參數以提高敏感度
    double sigma_xy = 50.0;
    double sigma_theta = 5.0;
    int hole_width = 300;
    ts_state_init(&state, map, &params, &laser_params, &start_pos, sigma_xy, sigma_theta, hole_width, 0);

    const char* portname = "/dev/ttyUSB0"; 
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) { fprintf(stderr, "FATAL ERROR: Error opening %s: %s\n", portname, strerror(errno)); return -1; }
    set_serial_attributes(fd, B115200);
    fprintf(stderr, "Debug: Serial port %s configured successfully.\n", portname);

    fprintf(stderr, "\nDebug: Entering main loop for PURE SCAN MATCHING test...\n");
    int frame_count = 0;
    static unsigned int timestamp = 0;

    while (1)
    {
        if (parse_and_fill_sensor_data(fd, &sensor_data) == 0)
        {
            // ########## 最終解決方案 ##########
            // 1. 提供一個持續增長的時間戳，以激活算法的運動估計功能
            timestamp += 100000; // 模擬100ms的時間間隔
            sensor_data.timestamp = timestamp;
            
            // 2. 保持里程計數據為零，以進行純掃描匹配測試
            sensor_data.q1 = 0;
            sensor_data.q2 = 0;
            
            // 添加掃描數據的詳細調試輸出
            int valid_points = 0;
            int min_dist = 99999, max_dist = 0;
            for (int i = 0; i < sensor_data.scan_size; i++) {
                if (sensor_data.d[i] > 0) {
                    valid_points++;
                    if (sensor_data.d[i] < min_dist) min_dist = sensor_data.d[i];
                    if (sensor_data.d[i] > max_dist) max_dist = sensor_data.d[i];
                }
            }
            fprintf(stderr, "SCAN DEBUG: valid_pts=%d, total_pts=%d, range=[%d, %d] mm\n", 
                    valid_points, sensor_data.scan_size, min_dist, max_dist);
            
            ts_iterative_map_building(&sensor_data, &state);

            fprintf(stdout, "SLAM Pose: x=%8.2f mm, y=%8.2f mm, theta=%6.2f deg\n", 
                    state.position.x, state.position.y, state.position.theta);
            fflush(stdout);

            frame_count++;
            if (frame_count % 150 == 0) {
                fprintf(stderr, "\n--- Saving map to live_map_pc.pgm ---\n");
                ts_save_map_pgm(map, empty_map, "live_map_pc.pgm", TS_MAP_SIZE, TS_MAP_SIZE);
            }
        }
    }

    close(fd);
    free(map);
    free(empty_map);
    return 0;
}
