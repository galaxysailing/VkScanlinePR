#include "scanline_rasterizer.h"

#include <stdexcept>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <array>
#include <iostream>
#include <assert.h>
#include <algorithm>

#include <fstream>
#include <sstream>

#include <GLFW/glfw3.h>

#define GPU_VULKAN_BUFFER(T) NEW_VULKAN_BUFFER(T, _vulkanDevice, usage_flags, memory_property_flags, queue)

#define COMPUTE_KERNAL(desc_types, shader, pcr) std::make_shared<ComputeKernal>(_device, _pipelineCache \
, desc_types                                                                                       \
, _compute.cmd_pool                                                                         \
, true                                                                                      \
, loadShader(shader, VK_SHADER_STAGE_COMPUTE_BIT)                                           \
, _vkCmdPushDescriptorSetKHR                                                        \
, pcr)                   

#define DESC_TYPE_SB VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
#define DESC_TYPE_UB VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
#define PUSH_SB_WRITE_DESC_SET(binding, buf_info) vk::initializer::writeDescriptorSet(0, DESC_TYPE_SB, binding, buf_info)
#define PUSH_UB_WRITE_DESC_SET(binding, buf_info) vk::initializer::writeDescriptorSet(0, DESC_TYPE_UB, binding, buf_info)

inline int divup(int a, int b) { return (a + (b - 1)) / b; }

inline uint32_t float_as_uint(float a) {
    union X {
        float a;
        uint32_t b;
    };
    X x;
    x.a = a;
    return x.b;
}

inline float int_as_float(int a) {
    union X {
        int a;
        float b;
    };
    X x;
    x.a = a;
}


inline float uint_as_float(uint32_t a) {
    union X {
        uint32_t a;
        float b;
    };
    X x;
    x.a = a;
}

namespace Galaxysailing {

using _Base = Galaxysailing::VulkanVGRasterizerBase;
 
// -------------------------------- public interface --------------------------------
void ScanlineVGRasterizer::initialize(void *window, uint32_t w, uint32_t h)
{
    // about window
    _window = (GLFWwindow*)window;
    _width = w, _height = h;

    _enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    _enabledDeviceExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);

    _Base::initVulkan();

    // Create a compute capable device queue
    vkGetDeviceQueue(_device, _vulkanDevice->queueFamilyIndices.compute, 0, &_compute.queue);
}

void ScanlineVGRasterizer::render()
{
    if (!_prepared) {
        prepare();
    }

    drawFrame();

}

void ScanlineVGRasterizer::loadVG(std::shared_ptr<VGContainer> vg_input)
{
    auto& vg = *vg_input;

    auto& point = vg.pointData;
    auto& curve = vg.curveData;
    auto& path = vg.pathData;
    
    int n_paths = path.pathIndex + 1;
    int n_curves = curve.curveIndex + 1;
    int n_points = point.pos.size();

    using std::vector;
    using namespace glm;

    // point 
    vector<vec2> position;
    vector<uint32_t> pos_path_idx;
    position.reserve(n_points);
    pos_path_idx.reserve(n_points);

    // curve
    vector<uint32_t> curve_pos_map;
    vector<uint8_t> curve_type;
    vector<uint32_t> curve_path_idx;
    //curve_pos_map.reserve(n_curves + 1);
    curve_type.reserve(n_curves);
    curve_path_idx.reserve(n_curves);

    // path
    vector<uint8_t> path_fill_rule;
    vector<uint32_t> path_fill_info;
    path_fill_rule.reserve(n_paths);
    path_fill_info.reserve(n_paths);

    for (uint32_t pi = 0; pi < n_paths; ++pi) {
        uint32_t path_idx = pi;
        uint32_t curve_begin = path.curveIndices[pi];
        uint32_t curve_end = (pi != n_paths - 1 ? path.curveIndices[pi + 1] : n_curves);
        
        // process fill color
        glm::vec4& color = path.fillColor[pi];
        color.a *= path.fillOpacity[pi];
        color *= 255.0f;
        uint8_t col[4];
#pragma unroll
        for (int i = 0; i < 4; ++i) {
            col[i] = static_cast<uint8_t>(color[i]);
        }
        uint32_t c = *((uint32_t*)col);
        c = c & 0xFF000000 ? c : 0;
        path_fill_info.push_back(c);

        // process fill rule
        path_fill_rule.push_back(static_cast<uint8_t>(path.fillRule[pi]));

        for (uint32_t ci = curve_begin; ci < curve_end; ++ci) {
            uint32_t curve_idx = ci;
            uint32_t point_begin = curve.posIndices[ci];
            uint32_t point_end = (ci != n_curves - 1 ? curve.posIndices[ci + 1] : n_points);
            curve_path_idx.push_back(path_idx);
            curve_pos_map.push_back(point_begin);
            curve_type.push_back(static_cast<uint8_t>(curve.curveType[ci]));
            for (uint32_t poi = point_begin; poi < point_end; ++poi) {
                position.push_back(point.pos[poi]);
                pos_path_idx.push_back(path_idx);
            }
        }
    }

    // record final curve-pos map
    //curve_pos_map.push_back(n_points - 1);

    auto& _in_curve = _compute.curve_input;
    auto& _in_path = _compute.path_input;

    VkQueue queue = _compute.queue;
    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VkMemoryPropertyFlags memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    _in_curve.position = GPU_VULKAN_BUFFER(vec2);
    _in_curve.position_path_idx = GPU_VULKAN_BUFFER(uint32_t);

    _in_curve.curve_position_map = GPU_VULKAN_BUFFER(uint32_t);
    _in_curve.curve_type = GPU_VULKAN_BUFFER(uint8_t);
    _in_curve.curve_path_idx = GPU_VULKAN_BUFFER(uint32_t);

    _in_curve.n_curves = n_curves;
    _in_curve.n_points = n_points;

    _in_path.fill_info = GPU_VULKAN_BUFFER(uint32_t);
    _in_path.fill_rule = GPU_VULKAN_BUFFER(uint8_t);

    _in_curve.position->set(position);
    _in_curve.position_path_idx->set(pos_path_idx);

    _in_curve.curve_position_map->set(curve_pos_map);
    _in_curve.curve_type->set(curve_type);
    _in_curve.curve_path_idx->set(curve_path_idx);

    _in_path.n_paths = n_paths;
    _in_path.fill_info->set(path_fill_info);
    _in_path.fill_rule->set(path_fill_rule);


    // debug
    //uint32* ptr = (uint32*)_in_curve.curve_type->cptr();
    //printf("-------------------- begin --------------------\n");
    //for (int i = 0; i < _compute.curve_input.n_curves; ++i) {
    //    printf("0x%08x\n", *ptr);
    //    uint offset = 0;
    //    printf("%u\n", ((ptr[0] >> offset) & 0x000000FF));
    //    offset += 8;
    //    printf("%u\n", ((ptr[0] >> offset) & 0x000000FF));
    //    offset += 8;
    //    printf("%u\n", ((ptr[0] >> offset) & 0x000000FF));
    //    offset += 8;
    //    printf("%u\n", ((ptr[0] >> offset) & 0x000000FF));
    //}
    //printf("-------------------- end --------------------\n");

}

void ScanlineVGRasterizer::setMVP(const glm::mat4& m)
{
    _compute.trans_pos_in.m0 = m[0];
    _compute.trans_pos_in.m1 = m[1];
    _compute.trans_pos_in.m2 = m[2];
    _compute.trans_pos_in.m3 = m[3];
}

//void ScanlineVGRasterizer::viewport(int x, int y, int w, int h)
//{
//    if (!_initialized) {
//        throw std::runtime_error("ScanlineVGRasterizer is not initialized");
//        return;
//    }
//
//}

// ---------------------------------------------------------------------------------

// ------------------------------ Vulkan Base override -----------------------------

void ScanlineVGRasterizer::prepare()
{
    _Base::prepare();

    // Get device push descriptor properties (to display them)
    PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(_instance, "vkGetPhysicalDeviceProperties2KHR"));
    if (!vkGetPhysicalDeviceProperties2KHR) {
        throw std::runtime_error("Could not get a valid function pointer for vkGetPhysicalDeviceProperties2KHR");
    }

    _vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(_device, "vkCmdPushDescriptorSetKHR");
    if (!_vkCmdPushDescriptorSetKHR) {
        throw std::runtime_error("vkCmdPushDescriptorSetKHR not found");
    }

    VkPhysicalDeviceProperties2KHR deviceProps2{};
    pushDescriptorProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
    deviceProps2.pNext = &pushDescriptorProps;
    vkGetPhysicalDeviceProperties2KHR(_physicalDevice, &deviceProps2);

    prepareGraphics();

    prepareComputeBuffers();
    prepareCompute();
    _prepared = true;
}

void ScanlineVGRasterizer::getEnabledFeatures()
{
    _enabledFeatures.wideLines = VK_TRUE;
}


// ------------------------------ Scaline PR rasterization -------------------------

void ScanlineVGRasterizer::prepareGraphics()
{
    /// test
    mockDataload();

    prepareTexelBuffers();
    setupDescriptorPool();
    setupLayoutsAndDescriptors();
    preparePipelines();
    buildCommandBuffers();
}
void ScanlineVGRasterizer::mockDataload() {
    std::ifstream fin;
    fin.open("test_data.csv");
    if (!fin.is_open()) {
        throw std::runtime_error("mock data file open failed");
    }
    auto replace_comma = [](std::string& str) {
        for (auto& c : str) { if (c == ',') { c = ' '; } }
    };
    std::string tstr;
    while (fin>>tstr) {
        replace_comma(tstr);
        glm::ivec4 v;
        std::istringstream iss(tstr);
        iss >> v.x >> v.y >> v.z >> v.w;
        if (v.w != 0) {
            outputIndex.push_back(v);
        }
    }
    std::cout << "load success\n";
}
void ScanlineVGRasterizer::drawFrame()
{   
    //vkWaitForFences(_device, 1, &_compute.fence, VK_TRUE, UINT64_MAX);
    VkSemaphore wait_compute;
    auto& k_scan = *(_kernal.scan);

    auto& k_transform_pos = *(_kernal.transform_pos);
    auto& k_make_inte_0 = *(_kernal.make_intersection_0);
    auto& k_make_inte_1 = *(_kernal.make_intersection_1);
    auto& k_gen_fragment = *(_kernal.gen_fragment);
    auto& k_seg_sort = *(_kernal.seg_sort);
    auto& k_shuffle_fragment = *(_kernal.shuffle_fragment);
    auto& k_mark_merged_fragment_and_span = *(_kernal.mark_merged_fragment_and_span);
    auto& k_gen_merged_fragment_and_span = *(_kernal.gen_merged_fragment_and_span);

    static bool is_first_draw = true;
    std::vector<VkPipelineStageFlags> wait_dst_stage_masks = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

    _compute.uniform_buffers.k_trans_pos_ubo->set(_compute.trans_pos_in, 1);

    // transform position
    std::vector<VkSemaphore> wait_sema = {};
    std::vector<VkSemaphore> signal_sema = {};
    VkSubmitInfo transpos_submit = k_transform_pos.submitInfo(is_first_draw, wait_sema, signal_sema, wait_dst_stage_masks.data());
    VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &transpos_submit, VK_NULL_HANDLE));
    wait_compute = k_transform_pos.semaphore;

    is_first_draw = false;

    // make intersection 0
    VkSubmitInfo make_inte_0_submit = k_make_inte_0.submitInfo(is_first_draw
        , wait_sema = { k_transform_pos.semaphore }
        , signal_sema = {}
        , wait_dst_stage_masks.data()
    );
    VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &make_inte_0_submit, VK_NULL_HANDLE));
    wait_compute = k_make_inte_0.semaphore;

    //drawDebug();

    uint32_t n_curves = _compute.curve_input.n_curves;
    int32_t n_fragments = 0;

    auto& _c = _compute;
    auto& _cub = _compute.uniform_buffers;
    auto& _csb = _compute.storage_buffers;
    auto& _in_curve = _c.curve_input;
    auto& _in_path = _c.path_input;

    std::vector<VkWriteDescriptorSet> write_desc_sets;
    // exclusive scan
    {
        auto& csb_curve_pixel_count = *_csb.curve_pixel_count;
        write_desc_sets = {
            PUSH_SB_WRITE_DESC_SET(0, &csb_curve_pixel_count.desc.buf_info),
            PUSH_SB_WRITE_DESC_SET(1, &csb_curve_pixel_count.desc.buf_info)
        };
        k_scan.beginCmdBuffer(true)
            ->cmdPushDescSet(write_desc_sets)
            ->cmdPushConst(0, 4, &_compute.curve_input.n_curves)
            ->cmdDispatch(1)
            ->endCmdBuffer();
        VkSubmitInfo scan_submit = k_scan.submitInfo(is_first_draw
            , wait_sema = { wait_compute }
            , signal_sema = {}
            , wait_dst_stage_masks.data()
        );
        VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &scan_submit, VK_NULL_HANDLE));
        wait_compute = k_scan.semaphore;

        VK_CHECK_RESULT(vkQueueWaitIdle(_compute.queue));
        n_fragments = csb_curve_pixel_count[n_curves];
    }
    _c.n_fragments = n_fragments;
    

    // make intersection 1
    int32_t stride_fragments = (n_fragments + 256) & -256;

    _c.stride_fragments = stride_fragments;
    _csb.intersection->resizeWithoutCopy(n_fragments * 2 + 2);
    _csb.fragment_data->resizeWithoutCopy(8 * stride_fragments + 1);
    
	_c.make_inte_in.n_fragments = n_fragments;
	_cub.k_make_inte_ubo->set(_c.make_inte_in, 1);

	write_desc_sets = {
		PUSH_UB_WRITE_DESC_SET(0, &_cub.k_make_inte_ubo->desc.buf_info),
		PUSH_SB_WRITE_DESC_SET(1, &_csb.intersection->desc.buf_info),
		PUSH_SB_WRITE_DESC_SET(2, &_csb.monotonic_cutpoint_cache->desc.buf_info),
		PUSH_SB_WRITE_DESC_SET(3, &_csb.curve_pixel_count->desc.buf_info),
		PUSH_SB_WRITE_DESC_SET(4, &_in_curve.curve_type->desc.buf_info),
		PUSH_SB_WRITE_DESC_SET(5, &_in_curve.curve_position_map->desc.buf_info),
		PUSH_SB_WRITE_DESC_SET(6, &_csb.transformed_pos->desc.buf_info),
		PUSH_SB_WRITE_DESC_SET(7, &_in_curve.curve_path_idx->desc.buf_info),
		PUSH_SB_WRITE_DESC_SET(8, &_csb.path_visible->desc.buf_info),
	};
	k_make_inte_1.beginCmdBuffer(true)
		->cmdPushDescSet(write_desc_sets)
		->cmdDispatch(divup(_compute.curve_input.n_curves, BLOCK_SIZE))
		->endCmdBuffer();
	VkSubmitInfo make_inte_1_submit = k_make_inte_1.submitInfo(is_first_draw
		, wait_sema = { wait_compute }
		, signal_sema = {}
		, wait_dst_stage_masks.data()
	);
	VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &make_inte_1_submit, VK_NULL_HANDLE));
    wait_compute = k_make_inte_1.semaphore;
    
    // gen_fragment_and_stencil_mask
    write_desc_sets = {
        PUSH_SB_WRITE_DESC_SET(0, &_csb.intersection->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(1, &_in_curve.curve_path_idx->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(2, &_in_curve.curve_position_map->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(3, &_in_curve.curve_type->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(4, &_csb.transformed_pos->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(5, &_csb.fragment_data->desc.buf_info),
    };
    k_gen_fragment.beginCmdBuffer(true)
        ->cmdPushDescSet(write_desc_sets)
        ->cmdPushConst(0, sizeof(uint32_t), &_compute.path_input.n_paths)
        ->cmdPushConst(sizeof(uint32_t), sizeof(uint32_t), &_compute.curve_input.n_curves)
        ->cmdPushConst(sizeof(uint32_t) * 2, sizeof(int32_t), &n_fragments)
        ->cmdPushConst(sizeof(uint32_t) * 3, sizeof(int32_t), &stride_fragments)
        ->cmdPushConst(sizeof(uint32_t) * 4, sizeof(int32_t), &_width)
        ->cmdPushConst(sizeof(uint32_t) * 5, sizeof(int32_t), &_height)
        ->cmdDispatch(divup(n_fragments, BLOCK_SIZE))
        ->endCmdBuffer();
    VkSubmitInfo gen_frag_submit = k_gen_fragment.submitInfo(is_first_draw
        , wait_sema = { wait_compute }
        , signal_sema = {}
        , wait_dst_stage_masks.data()
    );
    VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &gen_frag_submit, VK_NULL_HANDLE));
    wait_compute = k_gen_fragment.semaphore;
    
    // seg sort
    VkDescriptorBufferInfo key_desc, value_desc, seg_desc;
    key_desc.offset = 0;
    key_desc.buffer = _csb.fragment_data->buffer();
    key_desc.range = n_fragments * sizeof(int32_t);

    value_desc.offset = stride_fragments * sizeof(int32_t);
    value_desc.buffer = _csb.fragment_data->buffer();
    value_desc.range = n_fragments * sizeof(int32_t);

    seg_desc.offset = 3 * stride_fragments * sizeof(int32_t);
    seg_desc.buffer = _csb.fragment_data->buffer();
    seg_desc.range = (_in_path.n_paths + 1) * sizeof(int32_t);

    write_desc_sets = {
        PUSH_SB_WRITE_DESC_SET(0, &key_desc),
        PUSH_SB_WRITE_DESC_SET(1, &value_desc),
        PUSH_SB_WRITE_DESC_SET(2, &seg_desc)
    };
    k_seg_sort.beginCmdBuffer(true)
        ->cmdPushDescSet(write_desc_sets)
        ->cmdPushConst(0, sizeof(int32_t), &n_fragments)
        ->cmdPushConst(4, sizeof(int32_t), &_in_path.n_paths)
        ->cmdDispatch(BLOCK_SIZE, divup(_in_path.n_paths, BLOCK_SIZE))
        ->endCmdBuffer();
    VkSubmitInfo seg_sort_submit = k_seg_sort.submitInfo(is_first_draw
        , wait_sema = { wait_compute }
        , signal_sema = {}
        , wait_dst_stage_masks.data()
    );
    VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &seg_sort_submit, VK_NULL_HANDLE));
    wait_compute = k_seg_sort.semaphore;

    //drawDebug();
    

    // shuffle fragment
    write_desc_sets = {
        PUSH_SB_WRITE_DESC_SET(0, &_csb.fragment_data->desc.buf_info)
    };
    k_shuffle_fragment.beginCmdBuffer(true)
        ->cmdPushDescSet(write_desc_sets)
        ->cmdPushConst(0, sizeof(int32_t), &n_fragments)
        ->cmdPushConst(4, sizeof(int32_t), &stride_fragments)
        ->cmdDispatch(divup(n_fragments, BLOCK_SIZE))
        ->endCmdBuffer();
    VkSubmitInfo shuffle_frag_submit = k_shuffle_fragment.submitInfo(is_first_draw
        , wait_sema = { wait_compute }
        , signal_sema = {}
        , wait_dst_stage_masks.data()
    );
    VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &shuffle_frag_submit, VK_NULL_HANDLE));
    wait_compute = k_shuffle_fragment.semaphore;

    // exclusive scan
    {
        VkDescriptorBufferInfo desc;
        desc.offset = 3 * stride_fragments * sizeof(int32_t);
        desc.buffer = _csb.fragment_data->buffer();
        desc.range = n_fragments * sizeof(int32_t);
        write_desc_sets = {
            PUSH_SB_WRITE_DESC_SET(0, &desc),
            PUSH_SB_WRITE_DESC_SET(1, &desc)
        };
        k_scan.beginCmdBuffer(true)
            ->cmdPushDescSet(write_desc_sets)
            ->cmdPushConst(0, 4, &n_fragments)
            ->cmdDispatch(1)
            ->endCmdBuffer();
        VkSubmitInfo scan_submit = k_scan.submitInfo(is_first_draw
            , wait_sema = { wait_compute }
            , signal_sema = {}
            , wait_dst_stage_masks.data()
        );
        VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &scan_submit, VK_NULL_HANDLE));
        wait_compute = k_scan.semaphore;
        VK_CHECK_RESULT(vkQueueWaitIdle(_compute.queue));
    }

    drawDebug();

    // mark_merged_fragment_and_span
    write_desc_sets = {
        PUSH_SB_WRITE_DESC_SET(0, &_in_path.fill_rule->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(1, &_csb.fragment_data->desc.buf_info)
    };
    k_mark_merged_fragment_and_span.beginCmdBuffer(true)
        ->cmdPushDescSet(write_desc_sets)
        ->cmdPushConst(0, sizeof(int32_t), &n_fragments)
        ->cmdPushConst(4, sizeof(int32_t), &stride_fragments)
        ->cmdPushConst(8, sizeof(int32_t), &_width)
        ->cmdPushConst(12, sizeof(int32_t), &_height)
        ->cmdDispatch(divup(n_fragments, BLOCK_SIZE))
        ->endCmdBuffer();
    VkSubmitInfo mark_merge_submit = k_mark_merged_fragment_and_span.submitInfo(is_first_draw
        , wait_sema = { wait_compute }
        , signal_sema = {}
        , wait_dst_stage_masks.data()
    );
    VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &mark_merge_submit, VK_NULL_HANDLE));
    wait_compute = k_mark_merged_fragment_and_span.semaphore;

    /*
    ----------------------------------------------------------------
    | offset  | size  |
    | sf * 0  | nf    | position (sorted)
    | sf * 1  | nf    | index (sorted)
    | sf * 2  | nf    | pathid & winding_number_change
    | sf * 3  | nf    | winding number (sorted & scaned)
    | sf * 4  | 2*nf  | merged fragment flag, span flag
    | sf * 5  | *     |
    | sf * 6  | -     | -
    | sf * 7  | -     | -
    */

    // exclusive scan
    {
        VkDescriptorBufferInfo input_desc, output_desc;
        input_desc.offset = 4 * stride_fragments * sizeof(int32_t);
        input_desc.buffer = _csb.fragment_data->buffer();
        input_desc.range = 2 * n_fragments * sizeof(int32_t);

        output_desc.offset = 6 * stride_fragments * sizeof(int32_t);
        output_desc.buffer = input_desc.buffer;
        output_desc.range = (2 * n_fragments + 1) * sizeof(int32_t);

        write_desc_sets = {
            PUSH_SB_WRITE_DESC_SET(0, &input_desc),
            PUSH_SB_WRITE_DESC_SET(1, &output_desc)
        };
        int n = n_fragments * 2;
        k_scan.beginCmdBuffer(true)
            ->cmdPushDescSet(write_desc_sets)
            ->cmdPushConst(0, 4, &n)
            ->cmdDispatch(1)
            ->endCmdBuffer();
        VkSubmitInfo scan_submit = k_scan.submitInfo(is_first_draw
            , wait_sema = { wait_compute }
            , signal_sema = {}
            , wait_dst_stage_masks.data()
        );
        VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &scan_submit, VK_NULL_HANDLE));
        wait_compute = k_scan.semaphore;
        VK_CHECK_RESULT(vkQueueWaitIdle(_compute.queue));
    }

    //drawDebug();

    // gen_merged_fragment_and_span
    int n_output_fragments = (*_csb.fragment_data)[stride_fragments * 6 + n_fragments];
    int n_spans = (*_csb.fragment_data)[stride_fragments * 6 + n_fragments * 2];
    n_spans -= n_output_fragments;

    _compute.merged_fragment = n_output_fragments;
    _compute.span = n_spans;
    graphics.output_buf->resizeWithoutCopy(n_output_fragments + n_spans);
    graphics.output_buf->setupBufferView(VK_FORMAT_R32G32B32A32_SINT, VK_WHOLE_SIZE);
    VK_CHECK_RESULT(vkCreateBufferView(_device, &graphics.output_buf->desc.buf_view, nullptr, &graphics.output_buf_view));
    write_desc_sets = {
            PUSH_SB_WRITE_DESC_SET(0, &_csb.fragment_data->desc.buf_info),
            PUSH_SB_WRITE_DESC_SET(1, &_in_path.fill_info->desc.buf_info),
            PUSH_SB_WRITE_DESC_SET(2, &graphics.output_buf->desc.buf_info)
    };
    k_gen_merged_fragment_and_span.beginCmdBuffer(true)
        ->cmdPushDescSet(write_desc_sets)
        ->cmdPushConst(0, 4, &n_fragments)
        ->cmdPushConst(4, 4, &stride_fragments)
        ->cmdPushConst(8, 4, &_width)
        ->cmdPushConst(12, 4, &_height)
        ->cmdPushConst(16, 4, &n_output_fragments)
        ->cmdPushConst(20, 4, &n_spans)
        ->cmdDispatch(divup(n_fragments, BLOCK_SIZE))
        ->endCmdBuffer();
    VkSubmitInfo gen_fs_submit = k_gen_merged_fragment_and_span.submitInfo(is_first_draw
        , wait_sema = { wait_compute }
        , signal_sema = {}
        , wait_dst_stage_masks.data()
    );
    VK_CHECK_RESULT(vkQueueSubmit(_compute.queue, 1, &gen_fs_submit, VK_NULL_HANDLE));
    wait_compute = k_gen_merged_fragment_and_span.semaphore;

    // Submit graphics commands
    _Base::prepareFrame();
    
    VkCommandBufferBeginInfo cmdBufInfo = vk::initializer::commandBufferBeginInfo();
    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK_RESULT(vkBeginCommandBuffer(_drawCmdBuffers[_currentBuffer], &cmdBufInfo));

    // Acquire storage buffers from compute queue
    //addComputeToGraphicsBarriers(drawCmdBuffers[i]);

    // Draw the particle system using the update vertex buffer
    VkRenderPassBeginInfo renderPassBeginInfo = vk::initializer::renderPassBeginInfo();    
    VkClearValue clearValue = { {{1.0f, 1.0f, 1.0f, 1.0f}} };
    renderPassBeginInfo = vk::initializer::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = _renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = _width;
    renderPassBeginInfo.renderArea.extent.height = _height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

        // Set target frame buffer
    renderPassBeginInfo.framebuffer = _frameBuffers[_currentBuffer];
    vkCmdBeginRenderPass(_drawCmdBuffers[_currentBuffer], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = vk::initializer::viewport((float)_width, (float)_height, 0.0f, 1.0f);
    vkCmdSetViewport(_drawCmdBuffers[_currentBuffer], 0, 1, &viewport);

    VkRect2D scissor = vk::initializer::rect2D(_width, _height, 0, 0);
    vkCmdSetScissor(_drawCmdBuffers[_currentBuffer], 0, 1, &scissor);

    // Render path
    vkCmdBindPipeline(_drawCmdBuffers[_currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelines.scanline);

    write_desc_sets = {
        vk::initializer::writeDescriptorSet(0, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0, &graphics.output_buf_view)
        //vk::initializer::writeDescriptorSet(0, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0, &graphics.outputIndexBuffer.bufferView)
    };
    //vkCmdBindDescriptorSets(_drawCmdBuffers[_currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineLayout, 0, 1, &graphics.descriptorSet, 0, NULL);
    _vkCmdPushDescriptorSetKHR(_drawCmdBuffers[_currentBuffer]
        , VK_PIPELINE_BIND_POINT_GRAPHICS
        , graphics.pipelineLayout
        , 0
        , static_cast<uint32_t>(write_desc_sets.size())
        , write_desc_sets.data());
    vkCmdDraw(_drawCmdBuffers[_currentBuffer], graphics.output_buf->size() * 2, 1, 0, 0);
    //vkCmdDraw(_drawCmdBuffers[_currentBuffer], outputIndex.size() * 2, 1, 0, 0);

    //drawUI(drawCmdBuffers[i]);

    vkCmdEndRenderPass(_drawCmdBuffers[_currentBuffer]);

    // release the storage buffers to the compute queue
    //addGraphicsToComputeBarriers(drawCmdBuffers[i]);

    VK_CHECK_RESULT(vkEndCommandBuffer(_drawCmdBuffers[_currentBuffer]));

    std::array<VkPipelineStageFlags,2> waitDstStageMask = {
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    std::array<VkSemaphore, 2> waitSemaphores = {
        _semaphores.presentComplete
        , wait_compute
    };
    std::array<VkSemaphore, 1> signalSemaphores = {
        _semaphores.renderComplete
    };
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitDstStageMask.data();

    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_drawCmdBuffers[_currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(_presentQueue, 1, &submitInfo, VK_NULL_HANDLE));
    _Base::submitFrame();
    
}

void ScanlineVGRasterizer::drawDebug()
{
    auto save_data = [&](const std::string& kv_file, int ind) {
        std::ofstream fin;
        fin.open(kv_file);
        if (!fin.is_open()) {
            std::runtime_error(kv_file + " open failed");
        }
        int* ptr = _compute.storage_buffers.fragment_data->cptr();
        for (int i = 0; i <= _compute.n_fragments; ++i) {
            switch (ind) {
            case 1:
                fin << ptr[i] << "," << ptr[_compute.stride_fragments + i] << "," << ptr[_compute.stride_fragments * 3 + i] << "\n";
                break;
            case 2:
                fin << ptr[_compute.stride_fragments * 3 + i] << "\n";
                break;
            }
            //printf("%d,%d\n", ptr[0 * _compute.stride_fragments + i], ptr[_compute.stride_fragments + i]);
        }
        fin.close();
    };
    //save_data("kv.out", 1);
    //save_data("segments.out", 2);
    // for point debug
    //vec2* ptr = _compute.storage_buffers.transformed_pos->cptr();
    //printf("-------------------- begin --------------------\n");
    //for (int i = 0; i < _compute.curve_input.n_points; ++i) {
    //    printf("%d: %f, %f\n", i, ptr[i].x, ptr[i].y);
    //}
    //printf("-------------------- end --------------------\n");

    // for path debug
    //int32_t* ptr = _compute.storage_buffers.path_visible->cptr();
    //printf("-------------------- begin --------------------\n");
    //for (int i = 0; i < _compute.path_input.n_paths; ++i, ++ptr) {
    //    printf("%d\n", *ptr);
    //    //printf("%f, %f\n", ptr->x, ptr->y);
    //}
    //printf("-------------------- end --------------------\n");

    //printf("-------------------- begin --------------------\n");
    //int* ptr = _compute.storage_buffers.fragment_data->cptr();
    //for (int i = 0; i <= _compute.n_fragments; ++i) {
    //    //printf("%d,%d\n", ptr[0 * _compute.stride_fragments + i], ptr[_compute.stride_fragments + i]);
    //    //std::cout << ptr[i] << "," << ptr[_compute.stride_fragments + i] << "\n";
    //    std::cout << i << ": " << ptr[_compute.stride_fragments * 4 + i] << "\n";
    //}
    //printf("-------------------- end --------------------\n");

    //save_data("kv_out.txt");


    // for curve debug
    //int32_t* ptr = _compute.storage_buffers.curve_pixel_count->cptr();
    //printf("-------------------- begin --------------------\n");
    //for (int i = 0; i <= _compute.curve_input.n_curves; ++i) {
    //    printf("%d\n", ptr[i]);
    //    //printf("%f, %f\n", ptr->x, ptr->y);
    //}
    //printf("-------------------- end --------------------\n");
    //_compute.storage_buffers.curve_pixel_count->cptr_clear();

    // for fragments
    //int* ptr = _compute.storage_buffers.fragment_data->cptr();
    //printf("-------------------- begin --------------------\n");
    //for (int i = 0; i <= _compute.stride_fragments; ++i) {
    //    //int yx = ptr[i];
    //    //int16_t y = (int16_t)((yx >> 16) - 0x7FFF);
    //    //int16_t x = (int16_t)((yx & 0xFFFF) - 0x7FFF);
    //    //printf("%d: %hd, %hd\n", i, x, y);
    //    //printf("\n");
    //}
    //printf("-------------------- end --------------------\n");

    //float* ptr = _compute.storage_buffers.monotonic_cutpoint_cache->cptr();
    //ptr += 4;
    //union X{
    //    uint32_t a;
    //    float b;
    //};
    //printf("-------------------- begin --------------------\n");
    //for (int i = 0; i < _compute.curve_input.n_curves; ++i, ptr += 5) {
    //    X x;
    //    x.b = *ptr;

    //    printf("%u %f\n", x.a, x.b);
    //    //printf("%f, %f\n", ptr->x, ptr->y);
    //}
    //printf("-------------------- end --------------------\n");

    // for merge fragments
    //vec4* ptr = graphics.output_buf->cptr();
    //printf("-------------------- begin --------------------\n");
    //for (int i = 0; i <= _compute.n_fragments; ++i) {
    //    int yx = ptr[i].x;
    //    int16_t y = (int16_t)(yx >> 16);
    //    int16_t x = (int16_t)(yx & 0xFFFF);
    //    printf("%d: \n pos: %hd, %hd\n", i, x, y);
    //    printf(" width: %d\n", ptr[i].y);
    //    printf(" fill_info: %d\n", ptr[i].z);
    //    printf(" frag_index: %d\n\n", ptr[i].w);
    //}
    //printf("-------------------- end --------------------\n");
}

void ScanlineVGRasterizer::prepareTexelBuffers()
{
	// Vertex shader uniform buffer block
    _vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &graphics.outputIndexBuffer,
        sizeof(outputIndex[0]) * outputIndex.size());
	VK_CHECK_RESULT(graphics.outputIndexBuffer.map());
    VK_CHECK_RESULT(graphics.outputIndexBuffer.createBufferView(VK_FORMAT_R32G32B32A32_SINT, VK_WHOLE_SIZE));

    graphics.outputIndexBuffer.copyTo(outputIndex.data(), sizeof(outputIndex[0]) * outputIndex.size());

    // output merged fragment
    VkQueue queue = _presentQueue;
    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    graphics.output_buf = GPU_VULKAN_BUFFER(ivec4);
}

void ScanlineVGRasterizer::setupDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes = {
        // test
        vk::initializer::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1),
        ///
        vk::initializer::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5),
        vk::initializer::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2)
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vk::initializer::descriptorPoolCreateInfo(poolSizes, 3);

    VK_CHECK_RESULT(vkCreateDescriptorPool(_device, &descriptorPoolInfo, nullptr, &_descriptorPool));
}

void ScanlineVGRasterizer::setupLayoutsAndDescriptors()
{
    // graphics
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vk::initializer::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vk::initializer::descriptorSetLayoutCreateInfo(setLayoutBindings);
    // use push cmd
    descriptorLayout.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_device, &descriptorLayout, nullptr, &graphics.descriptorSetLayout));

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
        vk::initializer::pipelineLayoutCreateInfo(&graphics.descriptorSetLayout, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &pipelineLayoutCreateInfo, nullptr, &graphics.pipelineLayout));

    // Set
    //VkDescriptorSetAllocateInfo allocInfo =
    //    vk::initializer::descriptorSetAllocateInfo(_descriptorPool, &graphics.descriptorSetLayout, 1);
    //VK_CHECK_RESULT(vkAllocateDescriptorSets(_device, &allocInfo, &graphics.descriptorSet));

    //std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
    //    //vk::initializer::writeDescriptorSet(graphics.descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0, &graphics.outputIndexBuffer.bufferView)
    //    vk::initializer::writeDescriptorSet(graphics.descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0, &graphics.output_buf_view)
    //};
    //vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);


    // compute
    
}

void ScanlineVGRasterizer::preparePipelines()
{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vk::initializer::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, 0, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizationState =
        vk::initializer::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL
            , VK_CULL_MODE_NONE
            , VK_FRONT_FACE_COUNTER_CLOCKWISE
            , 2.0f
            , 0);

    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vk::initializer::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
            , VK_FALSE);

    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vk::initializer::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        vk::initializer::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

    VkPipelineViewportStateCreateInfo viewportState =
        vk::initializer::pipelineViewportStateCreateInfo(1, 1, 0);

    VkPipelineMultisampleStateCreateInfo multisampleState =
        vk::initializer::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState =
        vk::initializer::pipelineDynamicStateCreateInfo(dynamicStateEnables, 0);

    // Rendering pipeline
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    shaderStages[0] = loadShader(SURFACE_SPV_DIR + "scanlinepr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(SURFACE_SPV_DIR + "scanlinepr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = vk::initializer::pipelineCreateInfo(graphics.pipelineLayout, _renderPass);


    //// Assign to vertex buffer
    VkPipelineVertexInputStateCreateInfo inputState = vk::initializer::pipelineVertexInputStateCreateInfo();
    //inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(inputBindings.size());
    //inputState.pVertexBindingDescriptions = inputBindings.data();
    //inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(inputAttributes.size());
    //inputState.pVertexAttributeDescriptions = inputAttributes.data();

    pipelineCreateInfo.pVertexInputState = &inputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.renderPass = _renderPass;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(_device, _pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphics.pipelines.scanline));
}

void ScanlineVGRasterizer::prepareCompute()
{
    // Separate command pool as queue family for compute may be different than graphics
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = _vulkanDevice->queueFamilyIndices.compute;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(_device, &cmdPoolInfo, nullptr, &_compute.cmd_pool));


    // CPU-GPU synchronization
    //VkFenceCreateInfo fenceInfo = vk::initializer::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    //fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    //VK_CHECK_RESULT(vkCreateFence(_device, &fenceInfo, VK_NULL_HANDLE, &_compute.fence));

    prepareCommonComputeKernal();
    auto& _c = _compute;
    auto& _cin_curve = _compute.curve_input;
    auto& _csb = _compute.storage_buffers;
    auto& _cub = _compute.uniform_buffers;

    auto& _k = _kernal;

    std::vector<VkWriteDescriptorSet> write_desc_sets;
    auto wds2dt = [](std::vector<VkWriteDescriptorSet> &write_desc_sets) -> std::vector<VkDescriptorType> {
        std::vector<VkDescriptorType> desc_type;
        desc_type.reserve(write_desc_sets.size());
        for (auto& wds : write_desc_sets) {
            desc_type.push_back(wds.descriptorType);
        }
        return desc_type;
    };

    // transform position
    std::vector<VkWriteDescriptorSet> wds_transform = {
        PUSH_UB_WRITE_DESC_SET(0, &_cub.k_trans_pos_ubo->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(1, &_cin_curve.position->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(2, &_cin_curve.position_path_idx->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(3, &_csb.transformed_pos->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(4, &_csb.path_visible->desc.buf_info)
    };
    _k.transform_pos = COMPUTE_KERNAL(wds2dt(wds_transform), COMPUTE_SPV_DIR + "transform_pos.comp.spv", nullptr);
    _k.transform_pos->buildCmdBuffer(divup(_compute.curve_input.n_points, BLOCK_SIZE), wds_transform);

    // make intersection 0
    std::vector<VkWriteDescriptorSet> wds_make_int_0 {
        PUSH_UB_WRITE_DESC_SET(0, &_cub.k_make_inte_ubo->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(1, &_cin_curve.curve_type->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(2, &_cin_curve.curve_position_map->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(3, &_csb.transformed_pos->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(4, &_cin_curve.curve_path_idx->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(5, &_csb.path_visible->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(6, &_csb.monotonic_cutpoint_cache->desc.buf_info),
        PUSH_SB_WRITE_DESC_SET(7, &_csb.curve_pixel_count->desc.buf_info)
    };
    _k.make_intersection_0 = COMPUTE_KERNAL(wds2dt(wds_make_int_0), COMPUTE_SPV_DIR + "make_intersection_0.comp.spv", nullptr);
    _k.make_intersection_0->buildCmdBuffer(divup(_compute.curve_input.n_curves, BLOCK_SIZE), wds_make_int_0);

    // make intersection 1
    std::vector<VkDescriptorType> dt_make_int_1{
        DESC_TYPE_UB,
        DESC_TYPE_SB,DESC_TYPE_SB,
        DESC_TYPE_SB,DESC_TYPE_SB,
        DESC_TYPE_SB,DESC_TYPE_SB,
        DESC_TYPE_SB,DESC_TYPE_SB
    };

    _k.make_intersection_1 = COMPUTE_KERNAL(dt_make_int_1, COMPUTE_SPV_DIR + "make_intersection_1.comp.spv", nullptr);
    
    // generate fragments
    std::vector<VkDescriptorType> dt_gen_frag{
        DESC_TYPE_SB,DESC_TYPE_SB,
        DESC_TYPE_SB,DESC_TYPE_SB,
        DESC_TYPE_SB,DESC_TYPE_SB
    };
    std::vector<VkPushConstantRange> gen_frag_pcr{
        vk::initializer::pushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t) * 6, 0)
    };
    _k.gen_fragment = COMPUTE_KERNAL(dt_gen_frag, COMPUTE_SPV_DIR + "gen_fragment.comp.spv", &gen_frag_pcr);

    // shuffle fragment
    std::vector<VkDescriptorType> dt_shuffle_frag{
        DESC_TYPE_SB,DESC_TYPE_SB,
    };
    std::vector<VkPushConstantRange> shuffle_frag_pcr{
        vk::initializer::pushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t) * 2, 0)
    };
    _k.shuffle_fragment = COMPUTE_KERNAL(dt_shuffle_frag, COMPUTE_SPV_DIR + "shuffle_fragment.comp.spv", &shuffle_frag_pcr);

    // mark merged fragment and span
    std::vector<VkDescriptorType> dt_mark_merge{
        DESC_TYPE_SB,DESC_TYPE_SB,
        DESC_TYPE_SB,DESC_TYPE_SB
    };
    std::vector<VkPushConstantRange> mark_merge_pcr{
        vk::initializer::pushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t) * 4, 0)
    };
    _k.mark_merged_fragment_and_span = COMPUTE_KERNAL(dt_mark_merge, COMPUTE_SPV_DIR + "mark_merged_fragment_and_span.comp.spv", &mark_merge_pcr);

    //gen_merged_fragment_and_span
    std::vector<VkDescriptorType> dt_gen_fs{
        DESC_TYPE_SB,DESC_TYPE_SB,
        DESC_TYPE_SB
    };
    std::vector<VkPushConstantRange> gen_fs_pcr{
        vk::initializer::pushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t) * 6, 0)
    };
    _k.gen_merged_fragment_and_span = COMPUTE_KERNAL(dt_gen_fs, COMPUTE_SPV_DIR + "gen_merged_fragment_and_span.comp.spv", &gen_fs_pcr);

}

void ScanlineVGRasterizer::buildCommandBuffers()
{
    //VkCommandBufferBeginInfo cmdBufInfo = vk::initializer::commandBufferBeginInfo();
    VkClearValue clearValue = { {{1.0f, 1.0f, 1.0f, 1.0f}} };

    //VkRenderPassBeginInfo renderPassBeginInfo = vk::initializer::renderPassBeginInfo();
    //VkRenderPassBeginInfo& renderPassBeginInfo = graphics.renderPassBeginInfo;
    graphics.renderPassBeginInfo = vk::initializer::renderPassBeginInfo();
    graphics.renderPassBeginInfo.renderPass = _renderPass;
    graphics.renderPassBeginInfo.renderArea.offset.x = 0;
    graphics.renderPassBeginInfo.renderArea.offset.y = 0;
    graphics.renderPassBeginInfo.renderArea.extent.width = _width;
    graphics.renderPassBeginInfo.renderArea.extent.height = _height;
    graphics.renderPassBeginInfo.clearValueCount = 1;
    graphics.renderPassBeginInfo.pClearValues = &clearValue;

    for (int32_t i = 0; i < _drawCmdBuffers.size(); ++i)
    {
        // Set target frame buffer
        graphics.renderPassBeginInfo.framebuffer = _frameBuffers[i];

        //VK_CHECK_RESULT(vkBeginCommandBuffer(_drawCmdBuffers[i], &cmdBufInfo));

        //// Acquire storage buffers from compute queue
        ////addComputeToGraphicsBarriers(drawCmdBuffers[i]);

        //// Draw the particle system using the update vertex buffer

        //vkCmdBeginRenderPass(_drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        //VkViewport viewport = vk::initializer::viewport((float)_width, (float)_height, 0.0f, 1.0f);
        //vkCmdSetViewport(_drawCmdBuffers[i], 0, 1, &viewport);
        //
        //VkRect2D scissor = vk::initializer::rect2D(_width, _height, 0, 0);
        //vkCmdSetScissor(_drawCmdBuffers[i], 0, 1, &scissor);

        //// Render path
        //vkCmdBindPipeline(_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelines.scanline);
        //vkCmdBindDescriptorSets(_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineLayout, 0, 1, &graphics.descriptorSet, 0, NULL);
        //vkCmdDraw(_drawCmdBuffers[i], outputIndex.size() * 2, 1, 0, 0);

        ////drawUI(drawCmdBuffers[i]);

        //vkCmdEndRenderPass(_drawCmdBuffers[i]);

        //// release the storage buffers to the compute queue
        ////addGraphicsToComputeBarriers(drawCmdBuffers[i]);

        //VK_CHECK_RESULT(vkEndCommandBuffer(_drawCmdBuffers[i]));
    }
}

void ScanlineVGRasterizer::prepareComputeBuffers()
{   
    auto& _c = _compute;
    auto& _csb = _c.storage_buffers;
    auto& _cub = _c.uniform_buffers;
    auto& _in_curve = _c.curve_input;
    auto& _in_path = _c.path_input;
    
    VkQueue queue = _c.queue;
    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    // storage buffer
    _csb.transformed_pos = GPU_VULKAN_BUFFER(vec2);
    _csb.path_visible = GPU_VULKAN_BUFFER(int32_t);
    _csb.curve_pixel_count = GPU_VULKAN_BUFFER(int32_t);
    _csb.monotonic_cutpoint_cache = GPU_VULKAN_BUFFER(float);
    //_csb.monotonic_n_cuts_cache = GPU_VULKAN_BUFFER(uint32_t);
    _csb.intersection = GPU_VULKAN_BUFFER(float);
    _csb.fragment_data = GPU_VULKAN_BUFFER(int32_t);

    _csb.transformed_pos->resizeWithoutCopy(_in_curve.n_points);
    _csb.path_visible->resizeWithoutCopy(_in_path.n_paths);

    // mono
    _csb.curve_pixel_count->resizeWithoutCopy(_in_curve.n_curves + 1);
    _csb.monotonic_cutpoint_cache->resizeWithoutCopy(_in_curve.n_curves * 5);
    //_csb.monotonic_n_cuts_cache->resizeWithoutCopy(_in_curve.n_curves);

    // debug
    _csb.debug = GPU_VULKAN_BUFFER(int32_t);

    // uniform buffer
    usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    _cub.k_trans_pos_ubo = GPU_VULKAN_BUFFER(TransPosIn);
    _cub.k_make_inte_ubo = GPU_VULKAN_BUFFER(MakeInteIn);
    
    _c.trans_pos_in.n_points = _in_curve.n_points;
    _c.trans_pos_in.w = _width;
    _c.trans_pos_in.h = _height;

    _c.trans_pos_in.m0 = vec4(1, 0, 0, 1);
    _c.trans_pos_in.m1 = vec4(0, 1, 0, 0);
    _c.trans_pos_in.m2 = vec4(0, 0, 1, 0);
    _c.trans_pos_in.m3 = vec4(0, 0, 0, 1);

    //_c.trans_pos_in.m0 = vec4(17.789654, 0, 0, -815.924683);
    //_c.trans_pos_in.m1 = vec4(0, 17.789654, 0, -16161.220703);
    //_c.trans_pos_in.m2 = vec4(0, 0, 1, 0);
    //_c.trans_pos_in.m3 = vec4(0, 0, 0, 1);

    //_c.trans_pos_in.m0 = vec4(1.03434348, 0, 0, 204.363617);
    //_c.trans_pos_in.m1 = vec4(0, 1.03434348, 0, 0);
    //_c.trans_pos_in.m2 = vec4(0, 0, 1, 0);
    //_c.trans_pos_in.m3 = vec4(0, 0, 0, 1);

    //_c.trans_pos_in.m0 = vec4(38.851223, 0, 0, -2544.642822);
    //_c.trans_pos_in.m1 = vec4(0, 38.851223, 0, -34850.464844);
    //_c.trans_pos_in.m2 = vec4(0, 0, 1, 0);
    //_c.trans_pos_in.m3 = vec4(0, 0, 0, 1);

    _c.make_inte_in.w = _width;
    _c.make_inte_in.h = _height;
    _c.make_inte_in.n_curves = _in_curve.n_curves;

    _cub.k_trans_pos_ubo->set(_c.trans_pos_in, 1);
    _cub.k_make_inte_ubo->set(_c.make_inte_in, 1);
    
    
}

void ScanlineVGRasterizer::prepareCommonComputeKernal()
{
    auto& _k = _kernal;

    // scan
    std::vector<VkDescriptorType> scan_dt{
        DESC_TYPE_SB,
        DESC_TYPE_SB
    };
    std::vector<VkPushConstantRange> scan_pcr{
        vk::initializer::pushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t), 0)
    };
    _k.scan = COMPUTE_KERNAL(scan_dt, COMMON_COMPUTE_SPV_DIR + "naive_scan.comp.spv", &scan_pcr);

    // seg_sort
    std::vector<VkDescriptorType> seg_sort_dt{
        DESC_TYPE_SB,
        DESC_TYPE_SB,
        DESC_TYPE_SB
    };
    std::vector<VkPushConstantRange> seg_sort_pcr{
        vk::initializer::pushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t) * 2, 0)
    };
    _k.seg_sort = COMPUTE_KERNAL(seg_sort_dt, COMMON_COMPUTE_SPV_DIR + "naive_seg_sort_pairs.comp.spv", &seg_sort_pcr);

}

}