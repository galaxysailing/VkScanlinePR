import os
# d = os.system('glslc compute/gen_fragment.comp -o compute/spv/gen_fragment.comp.spv')
# print(__file__)
def compile_shader(path):
    files = os.listdir(path)
    files = [elem for elem in files if elem.endswith('.comp') or elem.endswith('.vert') or elem.endswith('.frag')]
    for file in files:
        filename = path + '\\' + file
        out = path + '\\spv\\' + file + '.spv'
        os.system('glslc ' + filename + ' -o ' + out)
        # print('glslc ' + filename + ' -o ' + out)


if __name__ == '__main__':
    abs_path = __file__
    abs_path = abs_path[0 : abs_path.rfind('\\')]
    compute_dir = abs_path + '\\scanline\\compute'
    surface_dir = abs_path + '\\scanline\\surface'
    common_dir = abs_path + '\\common'
    compile_shader(compute_dir)
    compile_shader(surface_dir)
    compile_shader(common_dir)