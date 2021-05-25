#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>

// --- general ---

long frames;
long score;
GtkWidget *game_layer;

void must_init(bool test, const char *description)
{
  if(test) return;

  printf("couldn't initialize %s\n", description);
  exit(1);
}

int between(int lo, int hi)
{
  return lo + (rand() % (hi - lo));
}

float between_f(float lo, float hi)
{
  return lo + ((float)rand() / (float)RAND_MAX) * (hi - lo);
}

bool collide(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
{
  if(ax1 > bx2) return false;
  if(ax2 < bx1) return false;
  if(ay1 > by2) return false;
  if(ay2 < by1) return false;

  return true;
}


// --- display ---

#define BUFFER_W 320
#define BUFFER_H 240

#define DISP_SCALE 2
#define DISP_W (BUFFER_W * DISP_SCALE)
#define DISP_H (BUFFER_H * DISP_SCALE)


// --- keyboard ---

#define KEY_SEEN     1
#define KEY_RELEASED 2

enum Keytype{
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_SPACE,
  KEY_MAX
};

unsigned char key[KEY_MAX];


// --- sprites ---

#define SHIP_W 24
#define SHIP_H 26

#define SHIP_SHOT_W 4
#define SHIP_SHOT_H 18

#define LIFE_W 12
#define LIFE_H 12

const int ALIEN_W[] = {28, 26, 90};
const int ALIEN_H[] = {18, 20, 54};

#define ALIEN_BUG_W      ALIEN_W[0]
#define ALIEN_BUG_H      ALIEN_H[0]
#define ALIEN_ARROW_W    ALIEN_W[1]
#define ALIEN_ARROW_H    ALIEN_H[1]
#define ALIEN_THICCBOI_W ALIEN_W[2]
#define ALIEN_THICCBOI_H ALIEN_H[2]

#define ALIEN_SHOT_W 8
#define ALIEN_SHOT_H 8

#define EXPLOSION_FRAMES 4
#define SPARKS_FRAMES    3


typedef struct SPRITES
{
  GdkTexture* ship_shot[2];
  GdkTexture* life;

  GdkTexture* alien[3];
  GdkTexture* alien_shot;

  GdkTexture* explosion[EXPLOSION_FRAMES];
  GdkTexture* sparks[SPARKS_FRAMES];

  GdkTexture* star;
} SPRITES;
SPRITES sprites;

GdkTexture* sprite_grab(const char* sprite_name)
{
  GFile* file = g_file_new_for_path(sprite_name);
  GdkTexture* sprite = gdk_texture_new_from_file(file, NULL);
  g_object_unref(file);
  return sprite;
}

void sprites_init()
{
  sprites.life = sprite_grab("life.png");

  sprites.ship_shot[0] = sprite_grab("shipshot0.png");
  sprites.ship_shot[1] = sprite_grab("shipshot1.png");

  sprites.alien[0] = sprite_grab("alienbug.png");
  sprites.alien[1] = sprite_grab("alienarrow.png");
  sprites.alien[2] = sprite_grab("alienthiccboi.png");

  sprites.alien_shot = sprite_grab("alienshot.png");

  sprites.explosion[0] = sprite_grab("explosion0.png");
  sprites.explosion[1] = sprite_grab("explosion1.png");
  sprites.explosion[2] = sprite_grab("explosion2.png");
  sprites.explosion[3] = sprite_grab("explosion3.png");

  sprites.sparks[0] = sprite_grab("sparks0.png");
  sprites.sparks[1] = sprite_grab("sparks1.png");
  sprites.sparks[2] = sprite_grab("sparks2.png");

  sprites.star = sprite_grab("star.png");
}

void sprites_deinit()
{
  g_object_unref(sprites.life);

  g_object_unref(sprites.ship_shot[0]);
  g_object_unref(sprites.ship_shot[1]);

  g_object_unref(sprites.alien[0]);
  g_object_unref(sprites.alien[1]);
  g_object_unref(sprites.alien[2]);
  
  g_object_unref(sprites.alien_shot);

  g_object_unref(sprites.sparks[0]);
  g_object_unref(sprites.sparks[1]);
  g_object_unref(sprites.sparks[2]);

  g_object_unref(sprites.explosion[0]);
  g_object_unref(sprites.explosion[1]);
  g_object_unref(sprites.explosion[2]);
  g_object_unref(sprites.explosion[3]);

  g_object_unref(sprites.star);
}


// --- fx ---

typedef struct FX
{
  int x, y;
  int frame;
  bool spark;
  bool used;
  GtkWidget* picture;
} FX;

#define FX_N 128
FX fx[FX_N];

void fx_init()
{
  for(int i = 0; i < FX_N; i++)
  {
    fx[i].used = false;
    fx[i].picture = gtk_picture_new();
    gtk_fixed_put(GTK_FIXED(game_layer), fx[i].picture, 0, 0);
  }
}

void fx_add(bool spark, int x, int y)
{
  for(int i = 0; i < FX_N; i++)
  {
    if(fx[i].used)
      continue;

    fx[i].x = x;
    fx[i].y = y;
    fx[i].frame = 0;
    fx[i].spark = spark;
    fx[i].used = true;
    gtk_widget_show(fx[i].picture);
    return;
  }
}

void fx_update()
{
  for(int i = 0; i < FX_N; i++)
  {
    if(!fx[i].used)
      continue;

    fx[i].frame++;

    if((!fx[i].spark && (fx[i].frame == (EXPLOSION_FRAMES * 2)))
    || ( fx[i].spark && (fx[i].frame == (SPARKS_FRAMES * 2)))
    )
    {
      fx[i].used = false;
      gtk_widget_hide(fx[i].picture);
    }
  }
}

void fx_draw()
{
  for(int i = 0; i < FX_N; i++)
  {
    if(!fx[i].used)
      continue;

    int frame_display = fx[i].frame / 2;
    GdkTexture* bmp =
      fx[i].spark
      ? sprites.sparks[frame_display]
      : sprites.explosion[frame_display]
    ;
    
    int width = gdk_texture_get_width(bmp) * DISP_SCALE;
    int height = gdk_texture_get_height(bmp) * DISP_SCALE;
    int x = fx[i].x - (width / 2);
    int y = fx[i].y - (height / 2);
    
    gtk_picture_set_paintable(GTK_PICTURE(fx[i].picture), GDK_PAINTABLE(bmp));
    gtk_widget_set_size_request(fx[i].picture, width, height);
    gtk_fixed_move(GTK_FIXED(game_layer), fx[i].picture, x, y);
  }
}


// --- shots ---

typedef struct SHOT
{
  int x, y, dx, dy;
  int frame;
  bool ship;
  bool used;
  GtkWidget* picture;
} SHOT;

#define SHOTS_N 128
SHOT shots[SHOTS_N];

void shots_init()
{
  for(int i = 0; i < SHOTS_N; i++)
  {
    shots[i].used = false;
    shots[i].picture = gtk_picture_new();
    gtk_fixed_put(GTK_FIXED(game_layer), shots[i].picture, 0, 0);
  }
}

bool shots_add(bool ship, bool straight, int x, int y)
{
  for(int i = 0; i < SHOTS_N; i++)
  {
    if(shots[i].used)
      continue;

    shots[i].ship = ship;

    if(ship)
    {
      shots[i].x = x - (SHIP_SHOT_W / 2);
      shots[i].y = y;
    }
    else // alien
    {
      shots[i].x = x - (ALIEN_SHOT_W / 2);
      shots[i].y = y - (ALIEN_SHOT_H / 2);

      if(straight)
      {
        shots[i].dx = 0;
        shots[i].dy = 4;
      }
      else
      {
        shots[i].dx = between(-4, 4);
        shots[i].dy = between(-4, 4);
      }

      // if the shot has no speed, don't bother
      if(!shots[i].dx && !shots[i].dy)
        return true;

      shots[i].frame = 0;
    }

    shots[i].frame = 0;
    shots[i].used = true;
    gtk_widget_show(shots[i].picture);

    return true;
  }
  return false;
}

void shots_update()
{
  for(int i = 0; i < SHOTS_N; i++)
  {
    if(!shots[i].used)
      continue;

    if(shots[i].ship)
    {
      shots[i].y -= 5;

      if(shots[i].y < -SHIP_SHOT_H)
      {
        shots[i].used = false;
        gtk_widget_hide(shots[i].picture);
        continue;
      }
    }
    else // alien
    {
      shots[i].x += shots[i].dx;
      shots[i].y += shots[i].dy;

      if((shots[i].x < -ALIEN_SHOT_W)
      || (shots[i].x > DISP_W)
      || (shots[i].y < -ALIEN_SHOT_H)
      || (shots[i].y > DISP_H)
      ) {
        shots[i].used = false;
        gtk_widget_hide(shots[i].picture);
        continue;
      }
    }

    shots[i].frame++;
  }
}

bool shots_collide(bool ship, int x, int y, int w, int h)
{
  for(int i = 0; i < SHOTS_N; i++)
  {
    if(!shots[i].used)
      continue;

    // don't collide with one's own shots
    if(shots[i].ship == ship)
      continue;

    int sw, sh;
    if(ship)
    {
      sw = ALIEN_SHOT_W;
      sh = ALIEN_SHOT_H;
    }
    else
    {
      sw = SHIP_SHOT_W;
      sh = SHIP_SHOT_H;
    }

    if(collide(x, y, x+w, y+h, shots[i].x, shots[i].y, shots[i].x+sw, shots[i].y+sh))
    {
      fx_add(true, shots[i].x + (sw / 2), shots[i].y + (sh / 2));
      shots[i].used = false;
      gtk_widget_hide(shots[i].picture);
      return true;
    }
  }

  return false;
}

void shots_draw()
{
  for(int i = 0; i < SHOTS_N; i++)
  {
    if(!shots[i].used)
      continue;

    int frame_display = (shots[i].frame / 2) % 2;

    if(shots[i].ship)
    {
      gtk_picture_set_paintable(GTK_PICTURE(shots[i].picture), 
        GDK_PAINTABLE(sprites.ship_shot[frame_display]));
      gtk_widget_set_size_request(shots[i].picture, SHIP_SHOT_W, SHIP_SHOT_H);
      gtk_fixed_move(GTK_FIXED(game_layer), shots[i].picture, shots[i].x, shots[i].y);
    }
    else // alien
    {
      gtk_picture_set_paintable(GTK_PICTURE(shots[i].picture), 
        GDK_PAINTABLE(sprites.alien_shot));
      gtk_widget_set_size_request(shots[i].picture, ALIEN_SHOT_W, ALIEN_SHOT_H);
      gtk_fixed_move(GTK_FIXED(game_layer), shots[i].picture, shots[i].x, shots[i].y);
    }
  }
}


// --- ship ---

#define SHIP_SPEED 6
#define SHIP_MAX_X (DISP_W - SHIP_W)
#define SHIP_MAX_Y (DISP_H - SHIP_H)

typedef struct SHIP
{
  int x, y;
  int shot_timer;
  int lives;
  int respawn_timer;
  int invincible_timer;
  GtkWidget* picture;
} SHIP;
SHIP ship;

void ship_init()
{
  ship.x = (DISP_W / 2) - (SHIP_W / 2);
  ship.y = (DISP_H / 2) - (SHIP_H / 2);
  ship.shot_timer = 0;
  ship.lives = 3;
  ship.respawn_timer = 0;
  ship.invincible_timer = 120;
  
  ship.picture = gtk_picture_new_for_filename("ship.png");
  gtk_widget_set_size_request(ship.picture, SHIP_W, SHIP_H);
  gtk_fixed_put(GTK_FIXED(game_layer), ship.picture, ship.x, ship.y);
}

void ship_update()
{
  if(ship.lives < 0)
    return;

  if(ship.respawn_timer)
  {
    ship.respawn_timer--;
    return;
  }
  
  if(key[KEY_LEFT])
    ship.x -= SHIP_SPEED;
  if(key[KEY_RIGHT])
    ship.x += SHIP_SPEED;
  if(key[KEY_UP])
    ship.y -= SHIP_SPEED;
  if(key[KEY_DOWN])
    ship.y += SHIP_SPEED;

  if(ship.x < 0)
    ship.x = 0;
  if(ship.y < 0)
    ship.y = 0;

  if(ship.x > SHIP_MAX_X)
    ship.x = SHIP_MAX_X;
  if(ship.y > SHIP_MAX_Y)
    ship.y = SHIP_MAX_Y;

  if(ship.invincible_timer)
    ship.invincible_timer--;
  else
  {
    if(shots_collide(true, ship.x, ship.y, SHIP_W, SHIP_H))
    {
      int x = ship.x + (SHIP_W / 2);
      int y = ship.y + (SHIP_H / 2);
      fx_add(false, x, y);
      fx_add(false, x+8, y+4);
      fx_add(false, x-4, y-8);
      fx_add(false, x+2, y-10);

      ship.lives--;
      ship.respawn_timer = 90;
      ship.invincible_timer = 180;
    }
  }

  if(ship.shot_timer)
    ship.shot_timer--;
  else if(key[KEY_SPACE])
  {
    int x = ship.x + (SHIP_W / 2);
    if(shots_add(true, false, x, ship.y))
      ship.shot_timer = 5;
  }
}

void ship_draw()
{
  if(ship.lives < 0)
  {
    gtk_widget_hide(ship.picture);
    return;
  }
  if(ship.respawn_timer)
  {
    gtk_widget_hide(ship.picture);
    return;
  }
  if(((ship.invincible_timer / 2) % 3) == 1)
  {
    gtk_widget_hide(ship.picture);
    return;
  }

  gtk_fixed_move(GTK_FIXED(game_layer), ship.picture, ship.x, ship.y);
  gtk_widget_show(ship.picture);
}


// --- aliens ---

typedef enum ALIEN_TYPE
{
  ALIEN_TYPE_BUG = 0,
  ALIEN_TYPE_ARROW,
  ALIEN_TYPE_THICCBOI,
  ALIEN_TYPE_N
} ALIEN_TYPE;

typedef struct ALIEN
{
    int x, y;
    ALIEN_TYPE type;
    int shot_timer;
    int blink;
    int life;
    bool used;
    GtkWidget* picture;
} ALIEN;

#define ALIENS_N 16
ALIEN aliens[ALIENS_N];

void aliens_init()
{
  for(int i = 0; i < ALIENS_N; i++)
  {
    aliens[i].used = false;
    aliens[i].picture = gtk_picture_new();
    gtk_fixed_put(GTK_FIXED(game_layer), aliens[i].picture, 0, 0);
  }
}

void aliens_update()
{
  int new_quota =
    (frames % 120)
    ? 0
    : between(2, 4)
  ;
  int new_x = between(20, DISP_W-100);

  for(int i = 0; i < ALIENS_N; i++)
  {
    if(!aliens[i].used)
    {
      // if this alien is unused, should it spawn?
      if(new_quota > 0)
      {
        new_x += between(80, 160);
        if(new_x > (DISP_W - 120))
          new_x -= (DISP_W - 120);

        aliens[i].x = new_x;

        aliens[i].y = between(-80, -60);
        aliens[i].type = between(0, ALIEN_TYPE_N);
        aliens[i].shot_timer = between(1, 99);
        aliens[i].blink = 0;
        aliens[i].used = true;
        gtk_widget_show(aliens[i].picture);

        switch(aliens[i].type)
        {
          case ALIEN_TYPE_BUG:
            aliens[i].life = 4;
            break;
          case ALIEN_TYPE_ARROW:
            aliens[i].life = 2;
            break;
          case ALIEN_TYPE_THICCBOI:
            aliens[i].life = 12;
            break;
        }

        new_quota--;
      }
      continue;
    }

    switch(aliens[i].type)
    {
      case ALIEN_TYPE_BUG:
        if(frames % 2)
          aliens[i].y++;
        break;

      case ALIEN_TYPE_ARROW:
        aliens[i].y++;
        break;

      case ALIEN_TYPE_THICCBOI:
        if(!(frames % 4))
          aliens[i].y++;
        break;
    }

    if(aliens[i].y >= DISP_H)
    {
      aliens[i].used = false;
      gtk_widget_hide(aliens[i].picture);
      continue;
    }

    if(aliens[i].blink)
      aliens[i].blink--;

    if(shots_collide(false, aliens[i].x, aliens[i].y, ALIEN_W[aliens[i].type], ALIEN_H[aliens[i].type]))
    {
      aliens[i].life--;
      aliens[i].blink = 4;
    }

    int cx = aliens[i].x + (ALIEN_W[aliens[i].type] / 2);
    int cy = aliens[i].y + (ALIEN_H[aliens[i].type] / 2);

    if(aliens[i].life <= 0)
    {
      fx_add(false, cx, cy);

      switch(aliens[i].type)
      {
        case ALIEN_TYPE_BUG:
          score += 200;
          break;

        case ALIEN_TYPE_ARROW:
          score += 150;
          break;

        case ALIEN_TYPE_THICCBOI:
          score += 800;
          fx_add(false, cx-20, cy-8);
          fx_add(false, cx+8, cy+20);
          fx_add(false, cx+16, cy+16);
          break;
      }

      aliens[i].used = false;
      gtk_widget_hide(aliens[i].picture);
      continue;
    }

    aliens[i].shot_timer--;
    if(aliens[i].shot_timer == 0)
    {
      switch(aliens[i].type)
      {
        case ALIEN_TYPE_BUG:
          shots_add(false, false, cx, cy);
          aliens[i].shot_timer = 150;
          break;
        case ALIEN_TYPE_ARROW:
          shots_add(false, true, cx, aliens[i].y);
          aliens[i].shot_timer = 80;
          break;
        case ALIEN_TYPE_THICCBOI:
          shots_add(false, true, cx-10, cy);
          shots_add(false, true, cx+10, cy);
          shots_add(false, true, cx-10, cy + 16);
          shots_add(false, true, cx+10, cy + 16);
          aliens[i].shot_timer = 200;
          break;
      }
    }
  }
}

void aliens_draw()
{
  for(int i = 0; i < ALIENS_N; i++)
  {
    if(!aliens[i].used)
    {
      gtk_widget_hide(aliens[i].picture);
      continue;
    }
    if(aliens[i].blink > 2)
    {
      gtk_widget_hide(aliens[i].picture);
      continue;
    }
    
    gtk_picture_set_paintable(GTK_PICTURE(aliens[i].picture), 
      GDK_PAINTABLE(sprites.alien[aliens[i].type]));
    gtk_widget_set_size_request(aliens[i].picture, ALIEN_W[aliens[i].type],
      ALIEN_H[aliens[i].type]);
    gtk_fixed_move(GTK_FIXED(game_layer), aliens[i].picture, aliens[i].x, aliens[i].y);
    gtk_widget_show(aliens[i].picture);
  }
}


// --- stars ---

typedef struct STAR
{
  float y;
  float speed;
  GtkWidget* picture;
} STAR;

#define STARS_N ((BUFFER_W / 2) - 1)
STAR stars[STARS_N];

void stars_init()
{
  for(int i = 0; i < STARS_N; i++)
  {
    stars[i].y = between_f(0, DISP_H);
    stars[i].speed = between_f(0.2, 2);
    stars[i].picture = gtk_picture_new_for_paintable(GDK_PAINTABLE(sprites.star));
    gtk_widget_set_size_request(stars[i].picture, 2, 2);
    gtk_fixed_put(GTK_FIXED(game_layer), stars[i].picture, -4, 4);
  }
}

void stars_update()
{
  for(int i = 0; i < STARS_N; i++)
  {
    stars[i].y += stars[i].speed;
    if(stars[i].y >= DISP_H)
    {
      stars[i].y = 0;
      stars[i].speed = between_f(0.2, 2);
    }
  }
}

void stars_draw()
{
  float star_x = 1.5;
  for(int i = 0; i < STARS_N; i++)
  {
    gtk_fixed_move(GTK_FIXED(game_layer), stars[i].picture, star_x, stars[i].y);
    star_x += 4;
  }
}


// --- hud ---

long score_display = 0;
bool score_display_changed = FALSE;
GtkWidget* score_label;

GtkWidget* gameover_label;
GtkWidget* lives_pictures[3];

void hud_init()
{
  score_label = gtk_label_new("");
  gtk_label_set_use_markup(GTK_LABEL(score_label), TRUE);
  
  char label_str[50];
  sprintf(label_str, "<span foreground=\"white\">%06ld</span>", score_display);
  gtk_label_set_label(GTK_LABEL(score_label), label_str);
  gtk_fixed_put(GTK_FIXED(game_layer), score_label, 2, 2);
  
  int spacing = LIFE_W + 2;
  for(int i = 0; i < 3; i++)
  {
    lives_pictures[i] = gtk_picture_new_for_filename("life.png");
    gtk_widget_set_size_request(lives_pictures[i], LIFE_W, LIFE_H);
    gtk_fixed_put(GTK_FIXED(game_layer), lives_pictures[i], 2 + (i * spacing), 20);
  }
}

void hud_update()
{
  if(frames % 2)
    return;

  for(long i = 5; i > 0; i--)
  {
    long diff = 1 << i;
    if(score_display <= (score - diff))
    {
      score_display += diff;
      score_display_changed = TRUE;
    }
      
  }
}

void hud_draw()
{
  if(score_display_changed)
  {
    char label_str[50];
    sprintf(label_str, "<span foreground=\"white\">%06ld</span>", score_display);
    gtk_label_set_label(GTK_LABEL(score_label), label_str);
    score_display_changed = FALSE;
  }
  
  int start_i = ship.lives;
  if(start_i < 0) start_i = 0;
  for(int i = start_i; i < 3; ++i)
    gtk_widget_hide(lives_pictures[i]);

  if(ship.lives < 0)
    gtk_widget_show(gameover_label);
}


// --- game loop implemented with tick callback ---

GtkWidget *window;

static gboolean
game_loop(GtkWidget *widget,
          GdkFrameClock *frame_clock,
          gpointer user_data)
{
  fx_update();
  shots_update();
  stars_update();
  ship_update();
  aliens_update();
  hud_update();
  
  frames++;
  
  for(int i = 0; i < KEY_MAX; i++)
    key[i] &= KEY_SEEN;
  
  stars_draw();
  aliens_draw();
  shots_draw();
  fx_draw();
  ship_draw();

  hud_draw();
  
  return G_SOURCE_CONTINUE;
}


// --- keyboard ---

static int keyval_to_keytype(guint keyval)
{
  if(keyval == GDK_KEY_Left)
    return KEY_LEFT;
  if(keyval == GDK_KEY_Right)
    return KEY_RIGHT;
  if(keyval == GDK_KEY_Up)
    return KEY_UP;
  if(keyval == GDK_KEY_Down)
    return KEY_DOWN;
  if(keyval == GDK_KEY_space)
    return KEY_SPACE;
    
  return -1;
}

static void
key_pressed (GtkEventControllerKey *controller, 
             guint keyval, 
             guint keycode, 
             GdkModifierType state, 
             gpointer user_data)
{
  int keytype = keyval_to_keytype(keyval);
  if(keytype > -1)
    key[keytype] = KEY_SEEN | KEY_RELEASED;
  
  if(keyval == GDK_KEY_Escape)
    gtk_window_close(GTK_WINDOW(window));
}

static void
key_released (GtkEventControllerKey *controller, 
              guint keyval, 
              guint keycode, 
              GdkModifierType state, 
              gpointer user_data)
{
  int keytype = keyval_to_keytype(keyval);
  if(keytype > -1)
    key[keytype] &= KEY_RELEASED;
}


static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Gtk4 Shooting Game");
  gtk_window_set_default_size (GTK_WINDOW (window), DISP_W, DISP_H);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  
  GtkWidget *overlay = gtk_overlay_new();
  GtkWidget *bg_picture = gtk_picture_new_for_filename("bg_pixel.png");
  gtk_overlay_set_child(GTK_OVERLAY(overlay), bg_picture);
  
  game_layer = gtk_fixed_new();
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), game_layer);
  
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), vbox);
  gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);
  
  gameover_label = gtk_label_new("");
  gtk_label_set_use_markup(GTK_LABEL(gameover_label), TRUE);
  gtk_label_set_label(GTK_LABEL(gameover_label), 
    "<span foreground=\"white\">G A M E  O V E R</span>");
  gtk_box_append(GTK_BOX(vbox), gameover_label);
  gtk_widget_hide(gameover_label);
  
  gtk_window_set_child(GTK_WINDOW(window), overlay);
  
  // --- init game ---
  sprites_init();
  
  stars_init();
  aliens_init();
  shots_init();
  fx_init();
  ship_init();
  hud_init();
  
  // --- add keyboard callback ---
  GtkEventController *keyboard = gtk_event_controller_key_new();
  gtk_widget_add_controller(window, keyboard);
  g_signal_connect(keyboard, "key-pressed", G_CALLBACK(key_pressed), window);
  g_signal_connect(keyboard, "key-released", G_CALLBACK(key_released), window);
  
  // --- start game loop ---
  gtk_widget_add_tick_callback(window, game_loop, NULL, NULL);

  gtk_widget_show (window);
}

int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("lavalfils.gtk4.game", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  
  sprites_deinit();
  
  g_printf("%d", status);
  return status;
}
