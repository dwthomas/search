command =  "/home/aifs2/dwr29/search/tiles/15md_solver {algorithm} -cost {cost} -wt {subopt} -width {width} < /home/aifs2/group/data/tiles_instances/korf/4/4/{korfnum} > /home/aifs2/dwr29/search/results/{outfile}" 
widths = [4**i for i in range(2, 6)]  # [16, 64, 256, 1024]
weights = [1.03, 1.05, 1.1, 1.3, 1.5, 2, 3, 5, 10]

korf_nums = list(range(1, 101))

costs = ["unit", "heavy", "inverse"]

algorithms = {
    "astar": {"width": False, "weight": False},
    "wastar": {"width": False, "weight": True},
    "bead": {"width": True, "weight": False},
    "bsbs": {"width": True, "weight": True},
    "rrd": {"width": False, "weight": True},
    "rrdnofocal": {"width": False, "weight": True},
    "rrdnoopen": {"width": False, "weight": True},
    "ees": {"width": False, "weight": True},
}

for cost in costs:
    for algorithm in algorithms:
        alg_widths = [-1]
        if algorithms[algorithm]["width"]:
            alg_widths = widths
        alg_weights = [-1]
        if algorithms[algorithm]["weight"]:
            alg_weights = weights
        for width in alg_widths:
            for weight in alg_weights:
                for inst in korf_nums:
                    ofname = "_".join([algorithm, str(weight), str(width), str(inst)])  
                    print(command.format(algorithm = algorithm, cost = cost, subopt = weight, width = width, korfnum = inst, outfile =ofname))
