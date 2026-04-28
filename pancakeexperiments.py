import numpy as np
command =  "/home/aifs2/dwr29/search/pancake/50pancake_solver {algorithm} -cost {cost} -wt {subopt} -width {width} < /home/aifs2/group/data/pancake/instance/50/{korfnum} > /home/aifs2/dwr29/search/pancake_results/{outfile}" 

widths = [4**i for i in range(2, 6)]  # [16, 64, 256, 1024]
weights = np.geomspace(1.01, 3, num=20)
# weights = [1.03, 1.05, 1.07, 1.1, 1.2, 1.3, 1.4, 1.5, 1.7,  2, 2.25, 2.8, 3, 5, 10]
# weights = [2.25, 2.8]
widths = [4**i for i in range(2, 6)]  # [16, 64, 256, 1024]
widths = [4**i for i in range(2, 8)]  # [16, 64, 256, 1024]
# weights = [1.03, 1.05, 1.07, 1.1, 1.2, 1.3, 1.4, 1.5, 1.7,  2, 2.25, 2.8, 3, 5, 10]
# weights = [1.03, 1.07, 1.2, 1.4, 1.7,  2, 3]
korf_nums = list(range(1, 999))

costs = ["unit", "heavy", "inverse"]

# aspects = [1, 100]

algorithms = {
    # "astar": {"width": False, "weight": False, "aspect": False},
    # "rectangle": {"width": False, "weight": False, "aspect": True},
    # "boundedrectangle": {"width": False, "weight": True, "aspect": True},
    # "wastar": {"width": False, "weight": True, "aspect": False},
     "wastar -dropdups": {"width": False, "weight": True, "aspect": False},
     "bead": {"width": True, "weight": False, "aspect": False},
    "bsbs": {"width": True, "weight": True, "aspect": False},
    # "bsbsflayer": {"width": True, "weight": True, "aspect": False},
    # "bsbsfill": {"width": True, "weight": True, "aspect": False},
    # "rrd": {"width": False, "weight": True, "aspect": False},
    # "rrdnofocal": {"width": False, "weight": True, "aspect": False},
    # "rrdnoopen": {"width": False, "weight": True, "aspect": False},
    # "ees": {"width": False, "weight": True, "aspect": False},
}

for cost in costs:
    for algorithm in algorithms:
        alg_widths = [-1]
        if algorithms[algorithm]["width"]:
            alg_widths = widths
        alg_weights = [-1]
        if algorithms[algorithm]["weight"]:
            alg_weights = weights
        alg_aspects = [-1]
        # if algorithms[algorithm]["aspect"]:
        #     alg_aspects = aspects

        for width in alg_widths:
            for weight in alg_weights:
                for aspect in alg_aspects:
                    for inst in korf_nums:
                        ofname = "_".join([algorithm.replace(" ", ""), cost, str(weight), str(width), str(inst)])  
                        print(command.format(algorithm = algorithm, cost = cost, subopt = weight, width = width, korfnum = inst, outfile =ofname))
