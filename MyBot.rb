#!/usr/bin/ruby

class Log
  def puts(s)
    STDOUT.puts("# " + s)
  end
end

LOG = Log.new

class Planet
  attr_accessor :x, :y, :owner, :num_ships, :growth_rate, :fleets, :benefit, :strength

  def initialize(x, y, owner, num_ships, growth_rate)
    @x = x
    @y = y
    @owner = owner
    @num_ships = num_ships
    @growth_rate = growth_rate
    @fleets = []
  end

  def total_ship_count
    sum = 0
    if owner == 1 then
      sum += num_ships
    else
      sum -= num_ships
    end
    @fleets.each do |f|
      if f.owner == 1 then
        sum += f.num_ships
      else
        sum -= f.num_ships
      end
    end
    return sum
  end

  def capacity
    t = total_ship_count
    if total_ship_count > num_ships then
      return num_ships - 5
    elsif total_ship_count > 0 then
      return total_ship_count - 5
    else
      return 0
    end
  end

  def total_ship_count_at(dist)
    sum = total_ship_count
    sum -= (dist + 1) * growth_rate
  end
end

class Fleet
  attr_accessor :owner, :num_ships, :source, :destination, 
    :total_trip_length, :turns_remaining

  def initialize(owner, num_ships, source, destination, total_trip_length, turns_remaining)
    @owner = owner
    @num_ships = num_ships
    @source = source
    @destination = destination
    @total_trip_length = total_trip_length
    @turns_remaining = turns_remaining
  end
end

class Driver

  attr_accessor :planets, :fleets, :bot

  def initialize(bot)
    @bot = bot
    @planets = []
    @fleets = []
  end

  def run
    File.open("trace.out", "w") do |trace|
      while ! STDIN.eof && (line = STDIN.readline) != nil do
        trace.write line
        trace.flush
        if (index = line.index("#")) != nil then
          line = line[0...index]
        end
        fields = line.split(/\s+/)
        case fields[0]
        when ""
        when "go"
          @bot.move(@planets, @fleets)
          STDOUT.puts "go"
          STDOUT.flush
          @planets = []
          @fleets = []
          ObjectSpace.garbage_collect
        when "P"
          @planets << Planet.new(fields[1].to_f, fields[2].to_f, fields[3].to_i, fields[4].to_i, fields[5].to_i)
        when "F"
          @fleets << Fleet.new(*fields[1..6].map { |x| x.to_i })
        end
      end
    end
  end
end

class Bot
  def calc_dist_for_pt(p, q)
    dx = p.x - q.x
    dy = p.y - q.y
    Math.sqrt(dx*dx + dy*dy).ceil.to_i
  end

  def calc_dist()
    dist = Hash.new { |h,k| h[k] = Hash.new }
    for p in 0 ... @planets.size do
      for q in 0 ... @planets.size do
        d = dist[p][q] = calc_dist_for_pt(@planets[p], @planets[q])
      end
    end    
    return dist
  end

  def move(planets, fleets)
    @planets = planets
    @fleets = fleets
    @dist ||= calc_dist
    candidates = Hash.new { |h,k| h[k] = [] }
    owned_planets = []

    # apply the fleets to each planet
    for f in 0 ... fleets.size do
      ff = @fleets[f]
      pp = @planets[ff.destination]
      pp.fleets << ff
    end

    # rank each planet in terms of benefit
    for p in 0 ... @planets.size do
      pp = @planets[p]
      pp.benefit = benefit(p)
      pp.strength = strength(p)
      LOG.puts [p, pp.benefit, pp.strength].inspect
    end

    ranked_planets = (0 ... planets.size).to_a.sort { |x, y| 
      px = @planets[x]
      py = @planets[y]
      py.benefit * py.strength <=> px.benefit * px.strength
    }

    for p in ranked_planets do
      aquire(p)
    end
  end

  def benefit(p)
    pp = @planets[p]
    gr = pp.growth_rate
    if pp.owner != 0 then
      gr *= 1.5
    end
    return gr
  end

  def strength(p)
    pp = @planets[p]
    strength = 0
    for q in 0 ... @planets.size do
      pq = @planets[q]
      if pq.owner == 1 then
        strength += (pq.num_ships) / (1 + @dist[p][q])
      end
    end
    strength /= (1 + pp.num_ships)
    return strength
  end

  def aquire(p)
    pp = @planets[p]
    strength = Hash.new { |h,k| h[k] = Hash.new }
    LOG.puts "aquire #{p} total_ship_count=#{pp.total_ship_count} #{pp.inspect}"
    if pp.total_ship_count < 0 then
      for df in [3, 4, 6, 8, 12, 20, 50, 100] do
        for n in 0 ... @planets.size do
          pn = @planets[n]
          if n != p && pn.owner == 1 then
            strength[@dist[n][p] / df][n] = @planets[n].capacity
          end
        end

        attack_dist = strength.keys.sort.first
        if attack_dist != nil then
          attackers = strength[attack_dist]
          total_s = 0
          for n, s in attackers do
            total_s += s
          end
          
          LOG.puts "  total_s=#{total_s}"
          ts = pp.total_ship_count_at(attack_dist) - 5
          if ((ts + total_s) > 0) && (ts < 0) then
            for n, s in attackers do
              as = ( - ts * (s.to_f / total_s) ).ceil.to_i + 1
              if as >= @planets[n].num_ships then
                as = @planets[n].num_ships
              end
              @planets[n].num_ships -= as
              send(n, p, as)
            end
            break
          end
        end
      end
    end
  end

  def send(source, destination, num_ships)
    if num_ships > 0 then
      STDOUT.puts [(source).to_s, (destination).to_s, num_ships.to_s].join(" ")
    end
  end
end

Driver.new(Bot.new).run
