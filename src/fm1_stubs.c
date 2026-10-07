// SPDX-License-Identifier: GPL-2.0-or-later
// FM-1 Doom: one arena, no finale and no intermission (their code and graphics do not fit the flash). The
// level's exit restarts it (g_game.c G_DoWorldDone keeps MAP01).
#ifdef FM1_DEVICE
#include "doomtype.h"
#include "d_event.h"
#include "d_player.h"
#include "g_game.h"

void F_StartFinale(void) { G_WorldDone(); }
boolean F_Responder(event_t *ev) { (void)ev; return false; }
void F_Ticker(void) {}
void F_Drawer(void) {}
void WI_Start(wbstartstruct_t *wb) { (void)wb; G_WorldDone(); }
void WI_Ticker(void) {}
void WI_Drawer(void) {}
void WI_End(void) {}
void StatCopy(wbstartstruct_t *stats) { (void)stats; }
void StatDump(void) {}

/* the FM-1's HUD (doom_glue.c): the player's health, armour, the ammo of the weapon in hand, the weapon */
#include "doomstat.h"
#include "d_items.h"
extern char *fm1_message;
extern int fm1_message_gen;
void fm1_hud(int *v, const char **msg, int *msg_gen)
{
    player_t *p = &players[consoleplayer];
    ammotype_t a = weaponinfo[p->readyweapon].ammo;
    v[0] = p->health;
    v[1] = p->armorpoints;
    v[2] = a == am_noammo ? -1 : p->ammo[a];
    v[3] = p->readyweapon;
    v[4] = gamestate == GS_LEVEL && p->playerstate == PST_DEAD;
    *msg = fm1_message;
    *msg_gen = fm1_message_gen;
}
#endif
