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

header = 'E:\VkScanlinePR\workdir\shaders\common'
files = os.listdir(header)
files = [elem for elem in files if elem.endswith('.comp')]
# print(files)
for file in files:
    filename = header + '\\' + file
    out = header + '\spv\\' + file + '.spv'
    os.system('glslc ' + filename + ' -o ' + out)

scanlinepr_vert_in = 'E:\VkScanlinePR\workdir\shaders\scanlinepr.vert'
scanlinepr_vert_out = 'E:\VkScanlinePR\workdir\shaders\scanlinepr.vert.spv'
scanlinepr_frag_in = 'E:\VkScanlinePR\workdir\shaders\scanlinepr.frag'
scanlinepr_frag_out = 'E:\VkScanlinePR\workdir\shaders\scanlinepr.frag.spv'
os.system('glslc ' + scanlinepr_vert_in + ' -o ' + scanlinepr_vert_out)
os.system('glslc ' + scanlinepr_frag_in + ' -o ' + scanlinepr_frag_out)