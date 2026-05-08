#include "tanish/libtanish.h"
#include "media/player.hpp"
#include "media/types.hpp"

extern "C" {

struct tanish_player_t {
    media::Player* internal_player;
};

tanish_player_t* tanish_player_create(void) {
    auto* p = new tanish_player_t;
    p->internal_player = new media::Player();
    return p;
}

void tanish_player_release(tanish_player_t* player) {
    if (player) {
        delete player->internal_player;
        delete player;
    }
}

void tanish_player_open(tanish_player_t* player, const char* uri) {
    if (player && player->internal_player && uri) {
        player->internal_player->open(media::MediaUri{uri});
    }
}

void tanish_player_play(tanish_player_t* player) {
    if (player && player->internal_player) {
        player->internal_player->play();
    }
}

void tanish_player_pause(tanish_player_t* player) {
    if (player && player->internal_player) {
        player->internal_player->pause();
    }
}

void tanish_player_stop(tanish_player_t* player) {
    if (player && player->internal_player) {
        player->internal_player->stop();
    }
}

double tanish_player_get_position(tanish_player_t* player) {
    if (player && player->internal_player)
        return player->internal_player->position_seconds();
    return 0.0;
}

double tanish_player_get_duration(tanish_player_t* player) {
    if (player && player->internal_player)
        return player->internal_player->duration_seconds();
    return 0.0;
}

void tanish_player_set_volume(tanish_player_t* player, float volume) {
    if (player && player->internal_player)
        player->internal_player->set_volume(volume);
}

} // extern "C"
