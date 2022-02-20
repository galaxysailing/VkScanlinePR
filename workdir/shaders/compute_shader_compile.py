import os
# d = os.system('glslc compute/gen_fragment.comp -o compute/spv/gen_fragment.comp.spv')
header = 'E:\VkScanlinePR\workdir\shaders\compute'
files = os.listdir(header)
files = [elem for elem in files if elem.endswith('.comp')]
# print(files)
for file in files:
    filename = header + '\\' + file
    out = header + '\spv\\' + file + '.spv'
    os.system('glslc ' + filename + ' -o ' + out)
    # print('glslc ' + filename + ' -o ' + out)
    