#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tanish_player_t tanish_player_t;

// Creates a new media player instance
tanish_player_t* tanish_player_create(void);

// Destroys the media player instance
void tanish_player_release(tanish_player_t* player);

// Opens a media URI (file path or network stream)
void tanish_player_open(tanish_player_t* player, const char* uri);

// Starts or resumes playback
void tanish_player_play(tanish_player_t* player);

// Pauses playback
void tanish_player_pause(tanish_player_t* player);

// Stops playback and clears the pipeline
void tanish_player_stop(tanish_player_t* player);

#ifdef __cplusplus
}
#endif
