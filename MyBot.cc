#include <stdio.h>
#include <string.h>
#include <math.h>

struct Planet {
  double x;
  double y;
  int owner;
  int num_ships;
  int growth_rate;
  int fleets[4096]; // should be enough
  int num_fleets;

  double benefit;
  double strength;
  int    capacity;
  int    total_ship_count;
  int    total_fleet_count;
};

struct Fleet {
  int owner;
  int num_ships;
  int source;
  int destination;
  int total_trip_length;
  int turns_remaining;
};

struct Universe {
  Planet planets[4096];    // should be enough
  Fleet  fleets[4096];     // should be enough
  int    dist[4096][4096]; // should be enough

  int    sorted_planets[4096];

  bool calculated_distances;
  int num_planets;
  int num_fleets;
};

void shitty_sort(void *base_ptr, int elem_size, int num_elems, void *ctx, 
  int (*cmp)(void *lft, void *rht, void *cxt),
  int dirn
) {
  char *base = (char *)base_ptr;
  char tmp[elem_size];
  for(int i=0;i<num_elems;++i) {
    for(int j=0;j<num_elems;j++) {
      char *i_elem = base + (elem_size * i);
      char *j_elem = base + (elem_size * j);
      if ( (*cmp)(i_elem, j_elem, ctx)*dirn > 0 ) {
        memcpy(tmp, j_elem, elem_size);
        memcpy(j_elem, i_elem, elem_size);
        memcpy(i_elem, tmp, elem_size);
      }
    }
  }
}

void read_planet(char *buf, Universe *universe) {
  Planet *p = universe->planets + (universe->num_planets++);
  int res = sscanf(buf + 2, "%lf %lf %d %d %d %d %d", &p->x, &p->y, &p->owner, 
    &p->num_ships, &p->growth_rate);
  memset(p->fleets, 0, sizeof(p->fleets));
  p->num_fleets = 0;
}

void read_fleet(char *buf, Universe *universe) {
  Fleet *f = universe->fleets + (universe->num_fleets++);
  sscanf(buf + 2, "%d %d %d %d %d %d", &f->owner, &f->num_ships, &f->source, 
    &f->destination, &f->total_trip_length, &f->turns_remaining);
}

int calc_dist(Planet *p, Planet *q) {
  double dx = p->x - q->x;
  double dy = p->y - q->y;
  return (int) ceil(sqrt(dx*dx + dy*dy));
}

void calc_dist(Universe *universe) 
{
  if ( universe->calculated_distances == false ) {
    for(int p = 0; p<universe->num_planets; ++p) {
      for(int q = 0; q<universe->num_planets; ++q) {
        universe->dist[p][q] = calc_dist(universe->planets + p, universe->planets + q);
      }
    }
    universe->calculated_distances = true;
  }
}

double calc_benefit(int p, Universe *universe)
{
  Planet *planet = universe->planets + p;
  double growth_rate = (double) planet->growth_rate;
  if ( planet->owner != 0 ) {
    growth_rate *= 1.5;
  }
  planet->benefit = growth_rate;
  return growth_rate;
}

double calc_strength(int p, Universe *universe)
{
  Planet *planet = universe->planets + p;
  double strength = 0;
  for(int q=0; q<universe->num_planets; ++q) {
    Planet *qplanet = universe->planets + q;
    if ( qplanet->owner == 1 ) {
      strength += (qplanet->num_ships) / (1 + universe->dist[p][q]); 
    }
  }
  strength /= (1 + planet->num_ships);
  planet->strength = strength;
}

void calc_total_fleet_count(int p, Universe *universe)
{
  int sum = 0;
  Planet *pp = universe->planets + p;
  for(int f=0; f<pp->num_fleets; ++f) {
    Fleet *ff = universe->fleets + pp->fleets[f];
    if ( ff->owner == 1 ) {
      sum += ff->num_ships;
    }
    else {
      sum -= ff->num_ships;
    }
  }
  pp->total_fleet_count = sum;
}

int calc_total_ship_count(int p, Universe *universe)
{
  int sum = 0;
  Planet *pp = universe->planets + p;
  if ( pp->owner == 1 ) {
    sum += pp->num_ships;
  }
  else {
    sum -= pp->num_ships;
  }
  sum += pp->total_fleet_count;
  pp->total_ship_count = sum;
  return sum;
}

int calc_total_ship_count_at(int p, Universe *universe, int dist) {
  Planet *pp = universe->planets + p;
  if ( pp->owner != 0 ) {
    return pp->total_ship_count - ((dist + 1) * pp->growth_rate);
  }
  else {
    return pp->total_ship_count;
  }
}

int calc_capacity(int p, Universe *universe)
{
  Planet *pp = universe->planets + p;
  int sum = 0;
  if (pp->total_ship_count > pp->num_ships) {
    sum = pp->num_ships - 5;
  }
  else if (pp->total_ship_count > 0) {
    sum = pp->total_ship_count - 5;
  }
  else {
    sum = 0;
  }
  pp->capacity = sum;
  return sum;
}

int compare_planets(void *p1_ptr, void *p2_ptr, void *universe_ptr)
{
  Universe *universe = (Universe *)universe_ptr;
  int p1 = *(int *)p1_ptr;
  int p2 = *(int *)p2_ptr;
  Planet *pp1 = universe->planets + p1;
  Planet *pp2 = universe->planets + p2;
  double cmp = (pp2->benefit * pp2->strength) - (pp1->benefit * pp1->strength);
  if ( cmp > 0 ) {
    return 1;
  }
  else if ( cmp == 0 ) {
    return 0;
  }
  else {
    return -1;
  }
};

void send(int source, int dest, int num_ships) {
  fprintf(stdout, "%d %d %d\n", source, dest, num_ships);
}

void aquire_planet(int p, Universe *universe) {

  int dfs[] = {3, 4, 6, 8, 10, 12, 16, 20, 30, 50, 100, 0}; 
  int neighbours[4096]; // should be enough
  int num_neighbours = 0;

  Planet *pp = universe->planets + p;

  fprintf(stdout, "# aquire p=%d growth_rate=%d x=%lf y=%lf strength=%lf benefit=%lf tsc=%d\n", 
    p, pp->growth_rate, pp->x, pp->y, pp->strength, pp->benefit, pp->total_ship_count);
  if ( pp->total_ship_count < 0 ) {
    for(int dfs_i=0;dfs[dfs_i]!=0; ++dfs_i) {
      // calc strength in rings around the planet
      int df = dfs[dfs_i];
      double strength = 0;
      num_neighbours = 0;
      for(int n=0; n<universe->num_planets; ++n) {
        Planet *pn = universe->planets + n;
        if ( ( universe->dist[n][p] <= df ) && ( n != p ) && ( pn->owner == 1 ) ) {
          strength += calc_capacity(n, universe);
          neighbours[num_neighbours++] = n;
        }
      }

      double ts = calc_total_ship_count_at(p, universe, df) - 5;
      fprintf(stdout, "# - df=%d ts=%lf\n", df, ts);
      if ( strength > 0 && ( ts + strength > 0 ) && (ts < 0) ) {
        for(int ni=0; ni<num_neighbours; ++ni) {
          int n  = neighbours[ni];
          Planet *nplanet = universe->planets + n;
          int as = ((int) ceil( -ts * (nplanet->capacity / strength) )) + 1;
          if ( as >= nplanet->num_ships ) {
            as = nplanet[n].num_ships;
          }
          if ( as > 0 ) {
            nplanet->num_ships -= as;
            calc_total_ship_count(n, universe);
            send(n, p, as);
          }
        }
        break;
      }
    }
  }
}

int make_move(Universe *universe)
{
  calc_dist(universe);

  // apply the fleets
  for(int f=0; f<universe->num_fleets; ++f) {
    Fleet  *fleet = universe->fleets + f;
    Planet *planet = universe->planets + fleet->destination;
    planet->fleets[planet->num_fleets++] = f;
  }

  // rank each planet based on benefit
  for(int p=0; p<universe->num_planets; ++p) {
    calc_total_fleet_count(p, universe);
    calc_total_ship_count(p, universe);
    calc_benefit(p, universe);
    calc_strength(p, universe);
    universe->sorted_planets[p] = p;
  }
  shitty_sort(universe->sorted_planets, sizeof(*universe->sorted_planets), universe->num_planets,
    universe, &compare_planets, -1);
  
  for(int p=0; p <universe->num_planets; ++p) {
    aquire_planet(universe->sorted_planets[p], universe);
  }
}

char *readline(char *buf, int buf_len, FILE* in)
{
  int i = 0, c;
  while(i < buf_len-1 && (c = fgetc(in)) != EOF ) {
    buf[i++] = c;
    if ( c == '\n' ) break;
  }
  buf[i] = 0;
  if ( c == EOF && *buf == 0 ) {
    return 0;
  }
  else {
    return buf;
  }
};

int main(int argc, char **argv) {
  char buf[4096]; // should be enough
  int  index;
  char *s;

  Universe *universe = new Universe();
  universe->calculated_distances = false;

  FILE *trace = fopen("trace.out", "w");

  while( readline(buf, sizeof(buf), stdin) != 0 ) {
    fputs(buf, trace);
    fflush(trace);
    if ( (s = strchr(buf, '#')) != 0 ) { 
      *s = 0;
    };
    switch(*buf) {
        case 'P':
          read_planet(buf, universe);
          break;
        case 'F':
          read_fleet(buf, universe);
          break;
        case 'g':
          make_move(universe);
          fputs("go\n", stdout);
          fflush(stdout);
          universe->num_planets = 0;
          universe->num_fleets = 0;
          break;
    }
  }
  return 0;
};
