#ifndef C8_AUDIO_H
#define C8_AUDIO_H

typedef struct c8_audio C8Audio;

C8Audio *c8_audio_new(void);
void c8_audio_free(C8Audio *audio);
void c8_audio_play(C8Audio *audio);

#endif
