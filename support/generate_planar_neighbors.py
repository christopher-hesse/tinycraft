for d in range(6):
    first_axis = 0
    second_axis = 0
    first_axis_multiplier = 1
    second_axis_multiplier = 1

    if d == 0:
        first_axis = 0
        second_axis = 2
        second_axis_multiplier = -1
    elif d == 1:
        first_axis = 2
        second_axis = 1
        first_axis_multiplier = -1
    elif d == 2:
        first_axis = 0
        second_axis = 1
        first_axis_multiplier = -1
    elif d == 3:
        first_axis = 2
        second_axis = 1
    elif d == 4:
        first_axis = 0
        second_axis = 1
    elif d == 5:
        first_axis = 0
        second_axis = 2

    print("  {")
    for corner in range(4):
        if corner == 0:
            offsets = [[-1, -1], [-1, 0], [0, -1]]
        elif corner == 1:
            offsets = [[0, -1], [1, 0], [1, -1]]
        elif corner == 2:
            offsets = [[0, 1], [1, 1], [1, 0]]
        elif corner == 3:
            offsets = [[-1, 0], [-1, 1], [0, 1]]
        voxeloffsets = []
        for offset_idx in range(3):
            voxeloffset = [0, 0, 0]
            voxeloffset[first_axis] = offsets[offset_idx][0] * first_axis_multiplier
            voxeloffset[second_axis] = offsets[offset_idx][1] * second_axis_multiplier
            voxeloffsets.append(
                f"{{{voxeloffset[0]}, {voxeloffset[1]}, {voxeloffset[2]}}}"
            )
        print("    {" + ", ".join(voxeloffsets) + "}" + ",")
    print("  },")
