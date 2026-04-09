// LEVEL: 3
// (LLVM) EXEC: %compiler %cmp_opt %optimization_level_2 --fast-math %overflow_wrapping %f -o %output
// (JET) EXEC: %compiler %cmp_opt %overflow_wrapping %f -o %output
// (LLVM) RUN-EXEC-PIPE: %export cjHeapSize=16GB && %run time %run_opt %output 50000000 %run_args 1> log.txt
// (JET) RUN-EXEC-PIPE: %export JETVMPROP=-Xmx2G && time cj %run_opt %output 50000000 %run_args 1> log.txt

import std.io.*
import std.fs.*
import std.convert.*
import std.sync.*
import std.collection.*
import std.math.*

var last_random : UInt32 = 42
let A : UInt32 = 3877
let C : UInt32 = 29573
let M : UInt32 = 139968

func rand() : Float64 {
    last_random = (last_random * A + C) % M
    return Float64(last_random) / Float64(M)
}

func generateRandom(buf : Array<Float64>, size : Int64)
{
   var i = 0
   while (i < size) {
      last_random = (last_random * A + C) % M
      buf [i] = Float64(last_random) / Float64(M)
      i++
   }
}

func makeCumulative(values : Array<Float64>): Array<Float64>{
    var temp: Array<Float64> = values.clone()
    var last : Float64 = 0.0
    let keySize = temp.size
    for (c in 0..keySize) {
        var cur = temp[c]
        temp[c] = cur+last
        last = cur+last
    }
    return temp
}

let ALU : Array<Byte> = "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA".toArray()

let IUB_keys : Array<Byte> = "acgtBDHKMNRSVWY".toArray()

let IUB_values : Array<Float64> = [
    0.27,0.12,0.12,0.27,
    0.02,0.02,0.02,0.02,
    0.02,0.02,0.02,0.02,
    0.02,0.02,0.02 ]

let IUB_cumulative : Array<Float64> = makeCumulative (IUB_values)

let HomoSap_keys : Array<Byte> = "acgt".toArray()

let HomoSap_values : Array<Float64> = [
    0.3029549426680,
    0.1979883004921,
    0.1975473066391,
    0.3015094502008 ]

let HomoSap_cumulative : Array<Float64> = makeCumulative (HomoSap_values)

let WIDTH = 60 // Fold lines after WIDTH bytes
let out = File("/dev/stdout", Write)
let bos = BufferedOutputStream(out)
let RANDOM_BUF_SIZE = WIDTH * 1000
let OUT_BUF_SIZE    = (WIDTH + 1) * 1000

let SLOT_COUNT = 4

let rbufs = Array<Array<Float64>> (SLOT_COUNT, repeat: Array<Float64>(RANDOM_BUF_SIZE, repeat: 0.0))
let wbufs = Array<Array<Byte>> (SLOT_COUNT, repeat: Array<Byte>(OUT_BUF_SIZE, repeat: 0))

class Slots
{
    let available_slots : Array<Int64>
    var available_slots_count : Int64;
    var monitor = Monitor()

    public init(slot_count : Int64)
    {
        available_slots_count = slot_count
        available_slots = Array<Int64> (slot_count, {i => i})
    }

    public func allocate() : Int64
    {
        synchronized(monitor) {
            while (available_slots_count == 0) {
                monitor.wait ();
            }
            available_slots_count--;
            return available_slots [available_slots_count];
        }
    }

    public func release(slot : Int64)
    {
        synchronized(monitor) {
            available_slots [available_slots_count] = slot;
            available_slots_count++;
            if (available_slots_count == 1) {
                monitor.notify ();
            }
        }
    }
}

class Queue<T>
{
    var monitor = Monitor()
    var closed = false
    var jobs = ArrayList<Future<T>>()

    public func add(job : Future<T>)
    {
        synchronized(monitor) {
            jobs.add (job)
            monitor.notify ()
        }
    }

    public func get() : Option<Future<T>>
    {
        synchronized(monitor) {
            while (! closed && jobs.isEmpty()) {
                monitor.wait ()
            }
            if (! jobs.isEmpty()) {
                return Some<Future<T>> (jobs.remove(at: 0))
            }
            return None<Future<T>>
        }
    }

    public func close()
    {
        synchronized(monitor) {
            closed = true
            monitor.notify()
        }
    }
}

func fastaRepeat(n_: Int64, seq: Array<Byte>){
    var pos = 0
    let size = seq.size
    var seqOut = Array<Byte>(size + WIDTH, repeat:0)
    seq.copyTo(seqOut, 0, 0, size)
    seq.copyTo(seqOut, 0, size, WIDTH)
    var n = n_
    while (n > 0) {
        let line = min(WIDTH, n)
        bos.write(seqOut[pos..pos+line])
        bos.write("\n".toArray())
        pos += line
        if (pos >= size) {
            pos -= size
        }
        n -= line
    }
}

func generateDna(keys : Array<Byte>, values_cumulative : Array<Float64>, rb : Array<Float64>, wb : Array<Byte>, size : Int64)
{
    let keysSize = keys.size
    var count = size

    var i = 0
    var o = 0
    while (count > 0) {
        var line = min(WIDTH, count)
        count -= line
        var j = 0
        while (j < line) {
            let r = rb[i]
            var y = 0
            while (y < keysSize) {
                if (r < values_cumulative[y]) {
                    wb[o] = keys[y]
                    break
                }
                y++
            }
            i++
            o++
            j++
        }
        wb[o] = UInt8 (10) // UInt32 (r'\n'))
        o++
    }
    return o
}

var slots : Slots = Slots (SLOT_COUNT)

func writer(queue : Queue<(Int64, Int64)>)
{
    while (true) {
        var res = queue.get()
        match (res) {
        case None =>
            return
        case Some(x) =>
            let (slot, size) = x.get ()
            let buf = wbufs[slot]
            bos.write(buf[0..size])
            slots.release (slot)
        }
    }
}


func fastaRandom(size : Int64, keys : Array<Byte>, values_cumulative : Array<Float64>){

    let queue : Queue<(Int64, Int64)> = Queue<(Int64, Int64)> ()

    let done = spawn {
        writer (queue)
    }
    var count = size
    while (count > 0) {
       let chunk = min(count, RANDOM_BUF_SIZE)
       count = count - chunk
       let slot = slots.allocate ()
       generateRandom (rbufs[slot], chunk)
       queue.add (spawn {
           let size = generateDna (keys, values_cumulative, rbufs[slot], wbufs[slot], chunk)
           (slot,  size)
       })
   }
   queue.close ()
   done.get ()
}


main(args: Array<String>){

    for (i in 0..SLOT_COUNT) {
        rbufs [i] = Array<Float64>(RANDOM_BUF_SIZE, repeat: 0.0)
        wbufs [i] = Array<Byte>(OUT_BUF_SIZE, repeat: 0)
    }

    bos.write(">ONE Homo sapiens alu\n".toArray())
    var n = Int64.tryParse(args[args.size-1])??Int64(50000000)

    fastaRepeat(2*n, ALU)

    bos.write(">TWO IUB ambiguity codes\n".toArray())
    fastaRandom(3*n, IUB_keys, IUB_cumulative)

    bos.write(">THREE Homo sapiens frequency\n".toArray())
    fastaRandom(5*n, HomoSap_keys, HomoSap_cumulative)

    bos.flush()
    out.close()
}
