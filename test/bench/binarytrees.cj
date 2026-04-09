// LEVEL: 3
// (LLVM) EXEC: %compiler %cmp_opt %optimization_level_2 --fast-math %overflow_wrapping %f -o %output
// (JET) EXEC: %compiler %cmp_opt %overflow_wrapping %f -o %output
// (LLVM) RUN-EXEC-PIPE: %export cjHeapSize=16GB && %run time %run_opt %output 21 %run_args 1> log.txt
// (JET) RUN-EXEC-PIPE: %export JETVMPROP=-Xmx2G && time cj %run_opt %output 21 %run_args 1> log.txt

import std.sync.*
import std.convert.*
import std.collection.*
enum Tree{
	Empty | Node(Tree,Tree)
}

func itemCheck(tree:Tree):UInt32{
	match (tree){
		case Node(left,right) => return 1 + itemCheck(left) + itemCheck(right)
		case _ => return 1

	}
}


func bottomUpTree(depth:UInt32):Tree{
	if (depth > 0) {
		return Node(bottomUpTree(depth - 1),bottomUpTree(depth - 1))
	}
	return Empty
}

class AllResult{
    public var outBuff :Array<String>
    public var outSize : UInt32
    public init(maxDepth:UInt32,minDepth:UInt32){
        outSize = UInt32(3) + (maxDepth-minDepth) / UInt32(2)
        outBuff = Array<String>(Int64(outSize),repeat:"")
    }
}

func Run(maxDepth_:UInt32){
    var maxDepth__ = maxDepth_
    let minDepth = UInt32(4)
    if (maxDepth__ < minDepth + UInt32(2)) {
        maxDepth__ = minDepth + UInt32(2)
    }
    let maxDepth = maxDepth__
    let x = AllResult(maxDepth,minDepth)


    var futures = ArrayList<Future<Unit>>()
    var f1 = spawn {
        var tree = bottomUpTree(maxDepth + UInt32(1))
        var msg ="stretch tree of depth ${maxDepth+1}\t check: ${itemCheck(tree)}"
        x.outBuff[0] = msg
    }
    futures.add(f1)

    let longLivedTreeFuture = spawn {
      return bottomUpTree(maxDepth)
    }

    for (depth in minDepth..=maxDepth:2) {
        let iterations = UInt32(1) << (maxDepth - depth + minDepth)

        var f_temp = spawn{
            var acc = UInt32(0)
            for (i in 0..iterations){
                var a = bottomUpTree(depth)
                acc += itemCheck(a)
            }
            var msg = "${iterations}\t trees of depth ${depth}\t check: ${acc}"
            x.outBuff[Int64(depth)/2 - 1] = msg
        }
        futures.add(f_temp)
    }
    for (f in futures){
        f.get()
    }
    let longLivedTree = longLivedTreeFuture.get()

    let msg = "long lived tree of depth ${maxDepth}\t check: ${itemCheck(longLivedTree)}"
    x.outBuff[Int64(x.outSize)-1] = msg

    for (m in x.outBuff) {
        println(m)
    }

}

main(args: Array<String>) : Unit {

    var n = UInt32.tryParse(args[args.size-1])??UInt32(21)

    Run(n)
}
