#ifndef RCSERVO_H_
#define RCSERVO_H_


#define MIN_WIDTH 700//최소 펄스폭
#define MAX_WIDTH 2300//최대 펄스폭
#define NEUTRAL_WIDTH (MAX_WIDTH + MIN_WIDTH) /2
//중립시 펄스폭

void RCServoInit(unsigned short period);
void RCServoSetOnWidth(unsigned short onWidth);



#endif /* RCSERVO_H_ */