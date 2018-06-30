#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
typedef unsigned size_t;

void ubx_frame_received(size_t len, uint8_t *msg)
{

}
void nmea_frame_received(size_t len, uint8_t *msg)
{

}

uint16_t calculate_checksum(uint8_t *read_buf, int len)
{

}
uint8_t UBX_HEADER[] = {0xB5, 0x62};
#define NMEA_START '$'

typedef enum {
    INIT = 0,
    READ_UBX_HEADER,
    READ_UBX_DATA,
    READ_UBX_CHECKSUM,
    READ_NMEA_DATA,
    READ_NMEA_CHECKSUM,
    READ_NMEA_CRLF
} read_state_e ;

typedef enum {
    UBX_SECOND_SYNC = 1,
    UBX_CLASS       = 2,
    UB0X_ID         = 3,
    UBX_LEN_START   = 4,
    UBX_LEN_END     = 5,
} ubox_header_pos_e;

#define UBX_CHECKSUM_START 0
#define UBX_CHECKSUM_END   1
#define NMEA_CHECKSUM_START 0
#define NMEA_CHECKSUM_END   1

uint8_t pos=0;
read_state_e read_state = INIT;
#define HDR_SIZE 32
uint8_t header_buf[HDR_SIZE];
#define CK_SIZE 2
uint8_t two_byte_array[CK_SIZE];
uint16_t checksum;
uint8_t two_byte_pos = 0;
uint8_t crlf_pos = 0;
#define NMEA_BUF_SIZE 82
uint8_t *read_buf;
uint16_t len;
#define CKS_INFO 4
#define NMEA_CHECKSUM_CHAR '*'

#define CHECK_TWO_BYTE_POS() \
            if (two_byte_pos < 2) {\
                two_byte_array[two_byte_pos]  = byte;\
                two_byte_pos++;\
            }


void reset_all()
{
    memset(read_buf, 0, len);
    len = 0;
    pos = 0;
    two_byte_pos = 0;
    checksum = 0;
    crlf_pos = 0;
    memset(header_buf, 0, HDR_SIZE);
    memset(two_byte_array, 0, CK_SIZE);
}

void recv_byte(uint8_t byte)
{
    switch(read_state) {
        case INIT:
            if (byte == NMEA_START) {
                read_state = READ_NMEA_DATA;
                pos = 0;
                read_buf = malloc(NMEA_BUF_SIZE);
            } else if (byte == UBX_HEADER[pos]) {
                    read_state = READ_UBX_HEADER;
                    header_buf[pos] = byte;
                    pos++;
            }
            break;
        case READ_UBX_HEADER:
            // the first sync character should be read
            if(pos == UBX_SECOND_SYNC) {
                if (byte == UBX_HEADER[pos]) {
                    header_buf[pos] = byte;
                } else {
                    //error and return
                }
            } else {
                header_buf[pos] = byte;
                if (pos == UBX_LEN_END) {
                    // assuming little endian
                    len = (header_buf[UBX_LEN_START] << 8 | header_buf[UBX_LEN_END]);
                    read_state = READ_UBX_DATA;
                    // foregoing error check for now.
                    read_buf = malloc(len+CKS_INFO);
                    memcpy(read_buf, header_buf+2, CKS_INFO);
                    pos = CKS_INFO;
                } else {
                    pos++;
                }
            }
            break;
        case READ_UBX_DATA:
            read_buf[pos] = byte;
            if (pos < len-1) {
                pos++;
            } else if (pos == len){
                read_state = READ_UBX_CHECKSUM;
                two_byte_pos = 0;
            }
            break;
        case READ_UBX_CHECKSUM:
            CHECK_TWO_BYTE_POS();
            if (two_byte_pos == 2) {
                checksum = (two_byte_array[UBX_CHECKSUM_START] << 8 | two_byte_array[UBX_CHECKSUM_END]);
                if(checksum == calculate_checksum(read_buf, len + CKS_INFO)) {
                    ubx_frame_received(len,read_buf+CKS_INFO);
                    read_state = INIT;
                    reset_all();
                } else {
                    printf("error in current stream of message.\n");
                    reset_all();
                }
            }
            break;
        case READ_NMEA_DATA:
            if (byte != NMEA_CHECKSUM_CHAR) {
                read_buf[pos] = byte;
                pos++;
            } else {
                len = pos;
                read_state = READ_NMEA_CHECKSUM;
                two_byte_pos = 0;
            }
            break;
        case READ_NMEA_CHECKSUM:
            CHECK_TWO_BYTE_POS();
            if (two_byte_pos == 2) {
                read_state = READ_NMEA_CRLF;
                two_byte_pos = 0;
            }
            break;
        case READ_NMEA_CRLF:
            CHECK_TWO_BYTE_POS();
            if (two_byte_pos == 2) {
                checksum = (two_byte_array[UBX_CHECKSUM_START] << 8 | two_byte_array[UBX_CHECKSUM_END]);
                if (checksum == calculate_checksum(read_buf, len)) {
                    nmea_frame_received(len, read_buf);
                    read_state = INIT;
                    reset_all();
                } else {
                    printf("error in current stream of message.\n");
                    reset_all();
                }
            }
            break;
        default:
            break;
    }
}

int main() {
    printf("Hello, World!\n");
    return 0;
}