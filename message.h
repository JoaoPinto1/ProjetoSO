#ifndef MESSAGE_H
#define MESSAGE_H
#define PAYLOADSIZE 6
typedef struct
{
	long msgtype;
	char payload[PAYLOADSIZE];
} msg;

#endif //MESSAGE_H
