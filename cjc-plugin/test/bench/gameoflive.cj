// EXEC: %compiler %cmp_opt %optimization_level_2 %overflow_wrapping %f -o %output
class Point {
  public Point(var x: Int64, var y: Int64) {
  }
}

class Location {

    public static let NUM_OF_NEIGHBORS = 8

    public var current: Point
    public var neighbors: Array<Point>

    public Location(curr: Point) {
      current = curr;
      neighbors = [Point(curr.x - 1, curr.y - 1),
                   Point(curr.x,     curr.y - 1),
                   Point(curr.x + 1, curr.y - 1),
                   Point(curr.x - 1, curr.y),
                   Point(curr.x + 1, curr.y),
                   Point(curr.x - 1, curr.y + 1),
                   Point(curr.x,     curr.y + 1),
                   Point(curr.x + 1, curr.y + 1)]
    }
}

struct Human {
  public let alive: Bool
  public var place : Location

  public Human(point: Point) {
    place = Location(point)
    alive = true
  }

  public init() {
    place = Location(Point(0, 0))
    alive = false
  }

  public func countNeighbors(s: Planet): Int64 {
    var count = 0
    for (i in 0..Location.NUM_OF_NEIGHBORS) {
      let friendPoint = place.neighbors[i]
      if (s.onPlanet(friendPoint) && s.isPopulationAlive(friendPoint)) {
        count++;
      }
    }
    count
  }

  public func reproduction(s: Planet, ns: Planet) {
    let n = countNeighbors(s)

    if (n > 1 && n < 4 && !ns.isPopulationAlive(this.place.current)) {
        ns.occupyPlace(this.place.current, this);
    }

    for (i in 0..Location.NUM_OF_NEIGHBORS) {
      let friend = Human(this.place.neighbors[i]);

      if (ns.onPlanet(friend.place.current) && !s.isPopulationAlive(friend.place.current) && !ns.isPopulationAlive(friend.place.current) && friend.countNeighbors(s) == 3) {
        ns.occupyPlace(friend.place.current, Human(friend.place.current))
      }
    }
  }

  public operator func ==(that : Human) : Bool {
    (this.alive == that.alive) &&
    (this.place.current.x == that.place.current.x) &&
    (this.place.current.y == that.place.current.y)
  }

  public operator func !=(that : Human) : Bool {
    !(this == that)
  }
}

class Planet {

    public var day  : Int64
    public var towns: Array<Array<Human>>

    public Planet(var sizex: Int64, var sizey: Int64) {
      day = 0
      towns = Array<Array<Human>>(sizex, repeat:Array<Human>(sizey, repeat:Human()))
      for (i in 0..sizex) {
        towns[i] = Array<Human>(sizey, repeat:Human())
      }
    }

    public func onPlanet(t: Point): Bool {
        return t.x >= 0 && t.y >= 0 && t.x < sizex && t.y < sizey
    }

    public func isPopulationAlive(t: Point): Bool {
      return towns[t.x][t.y].alive
    }

    public func occupyPlace(t: Point, h: Human) {
      towns[t.x][t.y] = h
    }

    public func newDay() {
      let ns = Planet(this.sizex, this.sizey)
      for (y in 0..sizey) {
        for (x in 0..sizex) {
          if (towns[x][y].alive) {
            let h = towns[x][y]
            h.reproduction(this, ns)
          }
        }
      }
      towns = ns.towns
      day++
    }

    public func runLife(days: Int64) {
      for (i in 0..days) {
        newDay()
      }
    }

    public func survivors() : Int64 {
      var result = 0
      for (x in 0..sizex) {
        for (y in 0..sizey) {
          if (towns[x][y].alive) {
            result++
          }
        }
      }
      result
    }

    public func print() {
      print("Day: " + day.toString() + "\n")
      printGrid()
    }

    public func printGrid() {
      print("+")
      for (i in 0..sizex) {
        print("-")
      }
      print("+")
      print("\n")

      for (y in 0..sizey) {
        print("|")
        for (x in 0..sizex) {
          if (towns[x][y].alive) {
            print("*")
          } else {
            print(" ")
          }
        }
        print("|")
        print("\n")
      }

      print("+")
      for (i in 0..sizex) {
        print("-")
      }
      print("+")
      print("\n")
    }

  public operator func ==(that : Planet) : Bool {
    if (this.sizex != that.sizex || this.sizey != that.sizey || this.day != that.day) {
      return false
    }
    for (i in 0..this.sizex) {
      for (j in 0..this.sizey) {
        if (this.towns[i][j] != that.towns[i][j]) {
          return false
        }
      }
    }
    true
  }

  public operator func !=(that : Planet) : Bool {
    !(this == that)
  }
}

let RAND_MAX : Int32 = 0x7FFFFFFF

func createSquarePlanet(size: Int64, seed: Int32): Planet {
  unsafe { srand(seed) }

  let planet = Planet(size, size)

  for (x in 0..size) {
    for (y in 0..size) {
      if (unsafe { rand() } < ((RAND_MAX / 10) * 4)) {
        let p = Point(x, y)
        planet.occupyPlace(p, Human(p))
      }
    }
  }
  planet
}

/*func readPlanet(day: Int64, data: Array<String>): Planet {
  let sizey = data.size
  // assert sizey != 0
  let sizex = data[0].size
  let planet = Planet(sizex, sizey)
  planet.day = day
  for (x in 0..sizex) {
    // assert data[x].size == sizey
    for (y in 0..sizey) {
      if (data[y][x] == '*') {
        let p = Point(x, y)
        planet.occupyPlace(p, Human(p))
      }
    }
  }
  planet
}*/

foreign func srand(seed: Int32) :Unit
foreign func rand(): Int32

main() {

  // Benchmark params
  let seed: Int32 = 1
  let size = 500
  let days = 20
  let VALIDATION_ENABLED = false

/*  if (VALIDATION_ENABLED) {
    // validation on small field
    let planetForValidation = createSquarePlanet(10, 1)
    planetForValidation.runLife(5)
    let planetReference = readPlanet(5, @{"      **  ",
                                          "     * *  ",
                                          "      *   ",
                                          "          ",
                                          "          ",
                                          "          ",
                                          "  *  * ** ",
                                          " **     * ",
                                          " **    ** ",
                                          "     **   "})

    if (planetForValidation != planetReference) {
      print("Basic validation failed\n")
      print("Planet for validation:\n")
      planetForValidation.print()
      print("Reference planet:\n")
      planetReference.print()
      return 1
    } else {
      print("Basic validation succeed\n")
    }
  } */
  let earth = createSquarePlanet(size, 1)
  earth.runLife(days)
  print("Survivors after simulation: " +  earth.survivors().toString() + "\n")
  0
}
