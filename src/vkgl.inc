// ================= vkgl: camada OpenGL 1.x -> Vulkan (BL_VULKAN) =================
// O jogo continua chamando glBegin/glVertex/glTranslatef/glDrawArrays... (macros abaixo redirecionam para vkgl*).
// Esta camada guarda o estado GL (matrizes, blend, neblina, alpha test, textura...) e desenha com UM pipeline Vulkan (shaders em src/vk/).
// Sem sombras/reflexo/god rays nesta versao (FBO e shaders GL ficam indisponiveis; o jogo desliga esses efeitos sozinho).
// Vertice unificado = struct V do mundo (x,y,z,u,v,r,g,b,a, 36 bytes): os VBOs dos chunks sao usados SEM copia.
#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>
#include <functional>
#include <map>
#include <set>
static const uint32_t kVkVertSpv[] =
#include "vk_vert.inc"
;
static const uint32_t kVkFragSpv[] =
#include "vk_frag.inc"
;
static void logInfo(const char* fmt, ...); static void logWarn(const char* fmt, ...); static void logError(const char* fmt, ...);

struct VkV { float x, y, z, u, v, r, g, b, a; };
struct VkU { float mvp[16], mvz[4], cur[4], fogC[4], fogP[4], alphaP[4], sky[4], env[4], cam[4]; };   // = bloco uniform do shader (192 bytes)
struct VkBuf { VkBuffer buf = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE; };
struct VkTex { std::vector<std::vector<uint8_t>> lv; std::vector<std::pair<int, int>> wh; bool dirty = true, used = false;
    VkImage img = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE; VkImageView view = VK_NULL_HANDLE; VkDescriptorSet set = VK_NULL_HANDLE; uint64_t skey = ~0ull; int levels = 1;
    GLenum minF = 0x2702, magF = 0x2601, ws = 0x2901, wt = 0x2901; };
struct VkArr { bool en = false; int size = 4; GLenum type = GL_FLOAT; int stride = 0; const void* ptr = nullptr; };
struct VkFrame { VkCommandBuffer cb = nullptr; VkFence fence = VK_NULL_HANDLE; VkSemaphore acq = VK_NULL_HANDLE; VkBuffer vb = VK_NULL_HANDLE, ub = VK_NULL_HANDLE;
    VkDeviceMemory vm = VK_NULL_HANDLE, um = VK_NULL_HANDLE; uint8_t *vmap = nullptr, *umap = nullptr; size_t voff = 0, uoff = 0; VkDescriptorSet uset = VK_NULL_HANDLE; };

static SDL_Window* vkWin = nullptr;
static VkInstance vkInst = VK_NULL_HANDLE; static VkSurfaceKHR vkSurf = VK_NULL_HANDLE; static VkPhysicalDevice vkPD = VK_NULL_HANDLE; static VkDevice vkDev = VK_NULL_HANDLE; static VkQueue vkQ = VK_NULL_HANDLE; static uint32_t vkQF = 0;
static VkPhysicalDeviceProperties vkPDP; static VkPhysicalDeviceMemoryProperties vkMemP;
static VkSwapchainKHR vkSC = VK_NULL_HANDLE; static VkFormat vkSCFmt = VK_FORMAT_B8G8R8A8_UNORM; static VkExtent2D vkExt = {1, 1};
static std::vector<VkImage> vkImgs; static std::vector<VkImageView> vkViews; static std::vector<VkFramebuffer> vkFBs; static std::vector<VkSemaphore> vkDone;
static VkImage vkDImg = VK_NULL_HANDLE; static VkDeviceMemory vkDMem = VK_NULL_HANDLE; static VkImageView vkDView = VK_NULL_HANDLE; static VkFormat vkDFmt = VK_FORMAT_D32_SFLOAT;
static VkRenderPass vkRP = VK_NULL_HANDLE; static VkCommandPool vkPool = VK_NULL_HANDLE;
static VkDescriptorSetLayout vkDSLU = VK_NULL_HANDLE, vkDSLT = VK_NULL_HANDLE; static VkPipelineLayout vkPL = VK_NULL_HANDLE; static VkDescriptorPool vkDPU = VK_NULL_HANDLE, vkDPT = VK_NULL_HANDLE;
static VkShaderModule vkVS = VK_NULL_HANDLE, vkFS = VK_NULL_HANDLE; static VkBuf vkQuadIB;
static VkFrame vkF[2]; static int vkCur = 0; static uint32_t vkImgIdx = 0; static bool vkOpen = false, vkPassOn = false, vkVsync = false, vkReady = false, vkNeedSC = false; static uint64_t vkFrameNo = 0;
static const size_t VK_VRING = 24u << 20, VK_URING = 8u << 20; static size_t vkUAlign = 256; static const int VK_QMAX = 65532;
static std::vector<std::pair<uint64_t, std::function<void()>>> vkDefer;
static void vkLater(std::function<void()> f) { vkDefer.push_back({vkFrameNo + 3, std::move(f)}); }
#define VKC(x) do { const VkResult r_ = (x); if (r_ != VK_SUCCESS) logError("Vulkan: %s falhou (%d)", #x, (int)r_); } while (0)

// ---------- estado GL ----------
static struct VkState {
    bool blend = false, depth = false, dmask = true, cull = false, alphaT = false, fog = false, tex2d = false, scis = false, poff = false;
    GLenum bsrc = GL_ONE, bdst = GL_ZERO, dfunc = GL_LESS, afn = GL_ALWAYS; unsigned cmask = 15; float aref = 0.f, pof = 0.f, pou = 0.f;
    float fogS = 0.f, fogE = 1.f, fogD = 1.f; int fogM = 1; float fogC[4] = {0, 0, 0, 0}, cur[4] = {1, 1, 1, 1}, clr[4] = {0, 0, 0, 0}, bc[4] = {0, 0, 0, 0};
    GLuint tex = 0; int vp[4] = {0, 0, 1, 1}, sc[4] = {0, 0, 1, 1}; float sky[4] = {0, 0, 1, 1}, env[4] = {0, 1, 0, 0}, cam[4] = {0, 0, 0, 0}; std::set<unsigned> other;
} vkS;
static float vkMat[3][32][16]; static int vkTop[3] = {0, 0, 0}, vkMM = 0;
static VkArr vkAV, vkAT, vkAC; static GLuint vkAB = 0, vkEB = 0;
static std::vector<VkBuf> vkBufs(1); static std::vector<VkTex> vkTexs(1);
static std::map<uint64_t, VkSampler> vkSamps; static std::unordered_map<uint64_t, VkPipeline> vkPipes; static VkPipeline vkBound = VK_NULL_HANDLE; static VkDescriptorSet vkBoundTex = VK_NULL_HANDLE;
struct VkList { std::vector<std::pair<bool, std::vector<VkV>>> parts; };
static std::vector<VkList> vkLists(1); static int vkListCur = 0;
static GLenum vkImmMode = 0; static std::vector<VkV> vkImm; static float vkTC[2] = {0, 0};

// ---------- utilitarios ----------
static void m4mul(const float* A, const float* B, float* C) { float T[16]; for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) { float s = 0.f; for (int k = 0; k < 4; k++) s += A[k * 4 + r] * B[c * 4 + k]; T[c * 4 + r] = s; } memcpy(C, T, sizeof T); }
static void m4id(float* m) { memset(m, 0, 64); m[0] = m[5] = m[10] = m[15] = 1.f; }
static float* vkM() { return vkMat[vkMM][vkTop[vkMM]]; }
static uint32_t vkFindMem(uint32_t bits, VkMemoryPropertyFlags want) { for (uint32_t i = 0; i < vkMemP.memoryTypeCount; i++) if ((bits & (1u << i)) && (vkMemP.memoryTypes[i].propertyFlags & want) == want) return i; return 0xffffffffu; }
static bool vkMakeBuf(VkDeviceSize sz, VkBufferUsageFlags us, VkBuffer& b, VkDeviceMemory& m, void** map) {
    VkBufferCreateInfo bi = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO}; bi.size = sz; bi.usage = us; bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(vkDev, &bi, nullptr, &b) != VK_SUCCESS) return false;
    VkMemoryRequirements mr; vkGetBufferMemoryRequirements(vkDev, b, &mr);
    VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; ai.allocationSize = mr.size; ai.memoryTypeIndex = vkFindMem(mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (ai.memoryTypeIndex == 0xffffffffu || vkAllocateMemory(vkDev, &ai, nullptr, &m) != VK_SUCCESS) { vkDestroyBuffer(vkDev, b, nullptr); b = VK_NULL_HANDLE; return false; }
    vkBindBufferMemory(vkDev, b, m, 0); if (map) vkMapMemory(vkDev, m, 0, VK_WHOLE_SIZE, 0, map); return true;
}
static VkCommandBuffer vkOneBegin() { VkCommandBufferAllocateInfo a = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO}; a.commandPool = vkPool; a.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; a.commandBufferCount = 1; VkCommandBuffer cb; vkAllocateCommandBuffers(vkDev, &a, &cb);
    VkCommandBufferBeginInfo bi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO}; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; vkBeginCommandBuffer(cb, &bi); return cb; }
static void vkOneEnd(VkCommandBuffer cb) { vkEndCommandBuffer(cb); VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO}; si.commandBufferCount = 1; si.pCommandBuffers = &cb; vkQueueSubmit(vkQ, 1, &si, VK_NULL_HANDLE); vkQueueWaitIdle(vkQ); vkFreeCommandBuffers(vkDev, vkPool, 1, &cb); }

// ---------- swapchain ----------
static void vkDestroySC() {
    for (VkFramebuffer f : vkFBs) vkDestroyFramebuffer(vkDev, f, nullptr); vkFBs.clear();
    for (VkImageView v : vkViews) vkDestroyImageView(vkDev, v, nullptr); vkViews.clear();
    for (VkSemaphore s : vkDone) vkDestroySemaphore(vkDev, s, nullptr); vkDone.clear();
    if (vkDView) { vkDestroyImageView(vkDev, vkDView, nullptr); vkDView = VK_NULL_HANDLE; } if (vkDImg) { vkDestroyImage(vkDev, vkDImg, nullptr); vkDImg = VK_NULL_HANDLE; } if (vkDMem) { vkFreeMemory(vkDev, vkDMem, nullptr); vkDMem = VK_NULL_HANDLE; }
}
static bool vkMakeSC() {
    VkSurfaceCapabilitiesKHR caps; vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPD, vkSurf, &caps);
    int dw = 0, dh = 0; SDL_Vulkan_GetDrawableSize(vkWin, &dw, &dh);
    VkExtent2D e = caps.currentExtent; if (e.width == 0xffffffffu) { e.width = (uint32_t)std::max(dw, 1); e.height = (uint32_t)std::max(dh, 1); }
    if (e.width == 0 || e.height == 0) return false;   // janela minimizada
    uint32_t nf = 0; vkGetPhysicalDeviceSurfaceFormatsKHR(vkPD, vkSurf, &nf, nullptr); std::vector<VkSurfaceFormatKHR> fm(nf); vkGetPhysicalDeviceSurfaceFormatsKHR(vkPD, vkSurf, &nf, fm.data());
    VkSurfaceFormatKHR sf = fm.empty() ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR} : fm[0];
    for (const auto& f : fm) if (f.format == VK_FORMAT_R8G8B8A8_UNORM || f.format == VK_FORMAT_B8G8R8A8_UNORM) { sf = f; break; }   // UNORM (sem conversao sRGB: igual ao OpenGL)
    uint32_t np = 0; vkGetPhysicalDeviceSurfacePresentModesKHR(vkPD, vkSurf, &np, nullptr); std::vector<VkPresentModeKHR> pm(np); vkGetPhysicalDeviceSurfacePresentModesKHR(vkPD, vkSurf, &np, pm.data());
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    if (!vkVsync) { for (VkPresentModeKHR m : pm) if (m == VK_PRESENT_MODE_MAILBOX_KHR) mode = m; if (mode == VK_PRESENT_MODE_FIFO_KHR) for (VkPresentModeKHR m : pm) if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) mode = m; }
    VkSwapchainCreateInfoKHR ci = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR}; ci.surface = vkSurf;
    ci.minImageCount = std::max(caps.minImageCount + 1u, 3u); if (caps.maxImageCount && ci.minImageCount > caps.maxImageCount) ci.minImageCount = caps.maxImageCount;
    ci.imageFormat = sf.format; ci.imageColorSpace = sf.colorSpace; ci.imageExtent = e; ci.imageArrayLayers = 1; ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform = (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : caps.currentTransform;
    ci.compositeAlpha = (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    ci.presentMode = mode; ci.clipped = VK_TRUE; ci.oldSwapchain = vkSC;
    vkDeviceWaitIdle(vkDev); vkDestroySC();
    VkSwapchainKHR ns = VK_NULL_HANDLE; if (vkCreateSwapchainKHR(vkDev, &ci, nullptr, &ns) != VK_SUCCESS) return false;
    if (vkSC) vkDestroySwapchainKHR(vkDev, vkSC, nullptr); vkSC = ns; vkSCFmt = sf.format; vkExt = e;
    uint32_t n = 0; vkGetSwapchainImagesKHR(vkDev, vkSC, &n, nullptr); vkImgs.resize(n); vkGetSwapchainImagesKHR(vkDev, vkSC, &n, vkImgs.data());
    // profundidade
    VkImageCreateInfo di = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO}; di.imageType = VK_IMAGE_TYPE_2D; di.format = vkDFmt; di.extent = {e.width, e.height, 1}; di.mipLevels = 1; di.arrayLayers = 1; di.samples = VK_SAMPLE_COUNT_1_BIT;
    di.tiling = VK_IMAGE_TILING_OPTIMAL; di.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; di.sharingMode = VK_SHARING_MODE_EXCLUSIVE; di.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VKC(vkCreateImage(vkDev, &di, nullptr, &vkDImg)); VkMemoryRequirements mr; vkGetImageMemoryRequirements(vkDev, vkDImg, &mr);
    VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; ai.allocationSize = mr.size; ai.memoryTypeIndex = vkFindMem(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); if (ai.memoryTypeIndex == 0xffffffffu) ai.memoryTypeIndex = vkFindMem(mr.memoryTypeBits, 0);
    VKC(vkAllocateMemory(vkDev, &ai, nullptr, &vkDMem)); vkBindImageMemory(vkDev, vkDImg, vkDMem, 0);
    VkImageViewCreateInfo vi = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO}; vi.image = vkDImg; vi.viewType = VK_IMAGE_VIEW_TYPE_2D; vi.format = vkDFmt; vi.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    if (vkDFmt == VK_FORMAT_D24_UNORM_S8_UINT || vkDFmt == VK_FORMAT_D32_SFLOAT_S8_UINT) vi.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    VKC(vkCreateImageView(vkDev, &vi, nullptr, &vkDView));
    if (!vkRP) {   // render pass (1x): cor limpa/guardada, profundidade limpa/descartada
        VkAttachmentDescription at[2] = {}; at[0].format = vkSCFmt; at[0].samples = VK_SAMPLE_COUNT_1_BIT; at[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE; at[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; at[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        at[1] = at[0]; at[1].format = vkDFmt; at[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; at[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference cr = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, dr = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sp = {}; sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; sp.colorAttachmentCount = 1; sp.pColorAttachments = &cr; sp.pDepthStencilAttachment = &dr;
        VkSubpassDependency dep = {}; dep.srcSubpass = VK_SUBPASS_EXTERNAL; dep.dstSubpass = 0;
        dep.srcStageMask = dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; dep.dstAccessMask = dep.srcAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        VkRenderPassCreateInfo rp = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO}; rp.attachmentCount = 2; rp.pAttachments = at; rp.subpassCount = 1; rp.pSubpasses = &sp; rp.dependencyCount = 1; rp.pDependencies = &dep;
        VKC(vkCreateRenderPass(vkDev, &rp, nullptr, &vkRP));
    }
    for (VkImage im : vkImgs) {
        VkImageViewCreateInfo cv = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO}; cv.image = im; cv.viewType = VK_IMAGE_VIEW_TYPE_2D; cv.format = vkSCFmt; cv.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkImageView v; VKC(vkCreateImageView(vkDev, &cv, nullptr, &v)); vkViews.push_back(v);
        VkImageView at2[2] = {v, vkDView}; VkFramebufferCreateInfo fi = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO}; fi.renderPass = vkRP; fi.attachmentCount = 2; fi.pAttachments = at2; fi.width = e.width; fi.height = e.height; fi.layers = 1;
        VkFramebuffer fb; VKC(vkCreateFramebuffer(vkDev, &fi, nullptr, &fb)); vkFBs.push_back(fb);
        VkSemaphoreCreateInfo si = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO}; VkSemaphore sm; vkCreateSemaphore(vkDev, &si, nullptr, &sm); vkDone.push_back(sm);
    }
    vkS.vp[2] = std::min(vkS.vp[2], (int)e.width); vkS.vp[3] = std::min(vkS.vp[3], (int)e.height); vkNeedSC = false; return true;
}

// ---------- quadro: acquire / begin / end / present ----------
static bool vkBegin() {
    if (vkOpen) return true; if (!vkReady) return false;
    VkFrame& f = vkF[vkCur];
    vkWaitForFences(vkDev, 1, &f.fence, VK_TRUE, UINT64_MAX);
    for (size_t i = 0; i < vkDefer.size();) { if (vkDefer[i].first <= vkFrameNo) { vkDefer[i].second(); vkDefer.erase(vkDefer.begin() + i); } else i++; }
    int dw = 0, dh = 0; SDL_Vulkan_GetDrawableSize(vkWin, &dw, &dh); if (dw > 0 && dh > 0 && ((uint32_t)dw != vkExt.width || (uint32_t)dh != vkExt.height)) vkNeedSC = true;
    if (vkNeedSC && !vkMakeSC()) return false;
    for (int tries = 0; tries < 3; tries++) {
        const VkResult r = vkAcquireNextImageKHR(vkDev, vkSC, UINT64_MAX, f.acq, VK_NULL_HANDLE, &vkImgIdx);
        if (r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR) break;
        if (r == VK_ERROR_OUT_OF_DATE_KHR) { if (!vkMakeSC()) return false; continue; }
        logError("Vulkan: acquire falhou (%d)", (int)r); return false;
    }
    vkResetFences(vkDev, 1, &f.fence); vkResetCommandBuffer(f.cb, 0);
    VkCommandBufferBeginInfo bi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO}; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; vkBeginCommandBuffer(f.cb, &bi);
    f.voff = f.uoff = 0; vkOpen = true; vkPassOn = false; vkBound = VK_NULL_HANDLE; vkBoundTex = VK_NULL_HANDLE; return true;
}
static float vkCol0[4] = {0, 0, 0, 1};
static bool vkPass() {   // abre o render pass (limpando com a ultima cor de glClearColor pedida antes do 1o desenho)
    if (!vkBegin()) return false; if (vkPassOn) return true;
    VkClearValue cv[2]; memcpy(cv[0].color.float32, vkCol0, 16); cv[1].depthStencil = {1.f, 0};
    VkRenderPassBeginInfo bi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO}; bi.renderPass = vkRP; bi.framebuffer = vkFBs[vkImgIdx]; bi.renderArea = {{0, 0}, vkExt}; bi.clearValueCount = 2; bi.pClearValues = cv;
    vkCmdBeginRenderPass(vkF[vkCur].cb, &bi, VK_SUBPASS_CONTENTS_INLINE); vkPassOn = true; return true;
}
static void vkSwap() {
    if (!vkReady) return; if (!vkPass()) return;
    VkFrame& f = vkF[vkCur]; vkCmdEndRenderPass(f.cb); vkEndCommandBuffer(f.cb);
    const VkPipelineStageFlags ws = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO}; si.waitSemaphoreCount = 1; si.pWaitSemaphores = &f.acq; si.pWaitDstStageMask = &ws; si.commandBufferCount = 1; si.pCommandBuffers = &f.cb; si.signalSemaphoreCount = 1; si.pSignalSemaphores = &vkDone[vkImgIdx];
    VKC(vkQueueSubmit(vkQ, 1, &si, f.fence));
    VkPresentInfoKHR pi = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR}; pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = &vkDone[vkImgIdx]; pi.swapchainCount = 1; pi.pSwapchains = &vkSC; pi.pImageIndices = &vkImgIdx;
    const VkResult r = vkQueuePresentKHR(vkQ, &pi); if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) vkNeedSC = true;
    vkCur ^= 1; vkFrameNo++; vkOpen = vkPassOn = false;
}

// ---------- texturas ----------
static VkSampler vkSamp(const VkTex& t) {
    const bool mip = t.minF >= 0x2700 && t.minF <= 0x2703 && t.levels > 1; const bool minL = t.minF == 0x2601 || t.minF == 0x2701 || t.minF == 0x2703; const bool mipL = t.minF == 0x2702 || t.minF == 0x2703;
    auto wrap = [](GLenum w) { return w == 0x2901 ? VK_SAMPLER_ADDRESS_MODE_REPEAT : w == 0x8370 ? VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; };
    const uint64_t key = (uint64_t)minL | ((uint64_t)(t.magF == 0x2601) << 1) | ((uint64_t)mipL << 2) | ((uint64_t)mip << 3) | ((uint64_t)wrap(t.ws) << 4) | ((uint64_t)wrap(t.wt) << 8) | ((uint64_t)t.levels << 12);
    auto it = vkSamps.find(key); if (it != vkSamps.end()) return it->second;
    VkSamplerCreateInfo si = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO}; si.magFilter = t.magF == 0x2601 ? VK_FILTER_LINEAR : VK_FILTER_NEAREST; si.minFilter = minL ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    si.mipmapMode = mipL ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST; si.addressModeU = wrap(t.ws); si.addressModeV = wrap(t.wt); si.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; si.maxLod = mip ? (float)(t.levels - 1) : 0.f; si.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    VkSampler s = VK_NULL_HANDLE; VKC(vkCreateSampler(vkDev, &si, nullptr, &s)); vkSamps[key] = s; return s;
}
static void vkTexUpload(VkTex& t) {
    int n = 0; while (n < (int)t.lv.size() && !t.lv[n].empty()) n++;
    for (int l = 1; l < n; l++) if (t.wh[l].first != std::max(1, t.wh[0].first >> l) || t.wh[l].second != std::max(1, t.wh[0].second >> l)) { n = l; break; }
    static const uint8_t white[4] = {255, 255, 255, 255}; int w = 1, h = 1; if (n > 0) { w = t.wh[0].first; h = t.wh[0].second; } else n = 1;
    if (t.img) { const VkImage i = t.img; const VkImageView v = t.view; const VkDeviceMemory m = t.mem; vkLater([=] { vkDestroyImageView(vkDev, v, nullptr); vkDestroyImage(vkDev, i, nullptr); vkFreeMemory(vkDev, m, nullptr); }); t.img = VK_NULL_HANDLE; }
    VkImageCreateInfo ii = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO}; ii.imageType = VK_IMAGE_TYPE_2D; ii.format = VK_FORMAT_R8G8B8A8_UNORM; ii.extent = {(uint32_t)w, (uint32_t)h, 1}; ii.mipLevels = (uint32_t)n; ii.arrayLayers = 1; ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL; ii.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; ii.sharingMode = VK_SHARING_MODE_EXCLUSIVE; ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VKC(vkCreateImage(vkDev, &ii, nullptr, &t.img)); VkMemoryRequirements mr; vkGetImageMemoryRequirements(vkDev, t.img, &mr);
    VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; ai.allocationSize = mr.size; ai.memoryTypeIndex = vkFindMem(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); if (ai.memoryTypeIndex == 0xffffffffu) ai.memoryTypeIndex = vkFindMem(mr.memoryTypeBits, 0);
    VKC(vkAllocateMemory(vkDev, &ai, nullptr, &t.mem)); vkBindImageMemory(vkDev, t.img, t.mem, 0);
    VkImageViewCreateInfo vi = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO}; vi.image = t.img; vi.viewType = VK_IMAGE_VIEW_TYPE_2D; vi.format = ii.format; vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, (uint32_t)n, 0, 1}; VKC(vkCreateImageView(vkDev, &vi, nullptr, &t.view));
    size_t total = 0; std::vector<VkBufferImageCopy> rg(n); for (int l = 0; l < n; l++) { const int lw = t.lv.empty() || t.lv[0].empty() ? 1 : t.wh[l].first, lh = t.lv.empty() || t.lv[0].empty() ? 1 : t.wh[l].second;
        rg[l] = {}; rg[l].bufferOffset = total; rg[l].imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)l, 0, 1}; rg[l].imageExtent = {(uint32_t)lw, (uint32_t)lh, 1}; total += (size_t)lw * lh * 4; }
    VkBuffer sb; VkDeviceMemory sm; void* mp = nullptr;
    if (vkMakeBuf(total, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sb, sm, &mp)) {
        if (t.lv.empty() || t.lv[0].empty()) memcpy(mp, white, 4); else for (int l = 0; l < n; l++) memcpy((uint8_t*)mp + rg[l].bufferOffset, t.lv[l].data(), (size_t)rg[l].imageExtent.width * rg[l].imageExtent.height * 4);
        VkCommandBuffer cb = vkOneBegin(); VkImageMemoryBarrier b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER}; b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b.image = t.img; b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, (uint32_t)n, 0, 1};
        b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
        vkCmdCopyBufferToImage(cb, sb, t.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)n, rg.data());
        b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
        vkOneEnd(cb); vkDestroyBuffer(vkDev, sb, nullptr); vkFreeMemory(vkDev, sm, nullptr);
    }
    t.levels = n; t.dirty = false; t.skey = ~0ull; t.used = true;
    if (!t.lv.empty() && !t.lv[0].empty() && t.lv.size() == 1) {} // (mantem o nivel 0 em memoria: glTexParameteri depois do upload nao precisa reenviar)
    for (auto& v : t.lv) { std::vector<uint8_t>().swap(v); } t.lv.clear(); t.wh.clear();   // libera a copia na CPU
}
static VkDescriptorSet vkTexSet(GLuint id) {
    if (id >= vkTexs.size()) id = 0; VkTex& t = vkTexs[id];
    if (t.dirty || !t.img) vkTexUpload(t);
    VkSampler sp = vkSamp(t); const uint64_t key = (uint64_t)(uintptr_t)sp;
    if (!t.set || t.skey != key) {
        if (t.set) { const VkDescriptorSet o = t.set; vkLater([=] { vkFreeDescriptorSets(vkDev, vkDPT, 1, &o); }); }
        VkDescriptorSetAllocateInfo ai = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO}; ai.descriptorPool = vkDPT; ai.descriptorSetCount = 1; ai.pSetLayouts = &vkDSLT; if (vkAllocateDescriptorSets(vkDev, &ai, &t.set) != VK_SUCCESS) { logError("Vulkan: sem descritores de textura"); t.set = VK_NULL_HANDLE; return vkTexs[0].set; }
        VkDescriptorImageInfo ii = {sp, t.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}; VkWriteDescriptorSet w = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}; w.dstSet = t.set; w.descriptorCount = 1; w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w.pImageInfo = &ii; vkUpdateDescriptorSets(vkDev, 1, &w, 0, nullptr); t.skey = key;
    }
    return t.set;
}

// ---------- pipeline ----------
static int vkBF(GLenum g) { switch (g) { case GL_ZERO: return 0; case GL_ONE: return 1; case 0x300: return 2; case 0x301: return 3; case 0x302: return 4; case 0x303: return 5; case 0x304: return 6; case 0x305: return 7; case 0x306: return 8; case 0x307: return 9; case 0x8001: return 10; case 0x8002: return 11; case 0x8003: return 12; case 0x8004: return 13; case 0x308: return 14; } return 1; }
static VkBlendFactor vkBFv(int i) { static const VkBlendFactor T[15] = {VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR, VK_BLEND_FACTOR_CONSTANT_COLOR, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR, VK_BLEND_FACTOR_CONSTANT_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA, VK_BLEND_FACTOR_SRC_ALPHA_SATURATE}; return T[i < 0 || i > 14 ? 1 : i]; }
static VkPipeline vkPipe(bool lines) {
    const uint64_t key = (uint64_t)lines | ((uint64_t)vkS.blend << 1) | ((uint64_t)vkBF(vkS.bsrc) << 2) | ((uint64_t)vkBF(vkS.bdst) << 6) | ((uint64_t)vkS.depth << 10) | ((uint64_t)vkS.dmask << 11) | ((uint64_t)((vkS.dfunc - 0x200) & 7) << 12) | ((uint64_t)vkS.cmask << 15) | ((uint64_t)vkS.cull << 19) | ((uint64_t)vkS.poff << 20);
    auto it = vkPipes.find(key); if (it != vkPipes.end()) return it->second;
    VkPipelineShaderStageCreateInfo st[2] = {}; st[0].sType = st[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; st[0].stage = VK_SHADER_STAGE_VERTEX_BIT; st[0].module = vkVS; st[0].pName = "main"; st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; st[1].module = vkFS; st[1].pName = "main";
    VkVertexInputBindingDescription vb = {0, sizeof(VkV), VK_VERTEX_INPUT_RATE_VERTEX}; VkVertexInputAttributeDescription va[3] = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, {1, 0, VK_FORMAT_R32G32_SFLOAT, 12}, {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 20}};
    VkPipelineVertexInputStateCreateInfo vi = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO}; vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &vb; vi.vertexAttributeDescriptionCount = 3; vi.pVertexAttributeDescriptions = va;
    VkPipelineInputAssemblyStateCreateInfo ia = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO}; ia.topology = lines ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vs = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO}; vs.viewportCount = 1; vs.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO}; rs.polygonMode = VK_POLYGON_MODE_FILL; rs.cullMode = vkS.cull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE; rs.frontFace = VK_FRONT_FACE_CLOCKWISE; rs.depthBiasEnable = vkS.poff ? VK_TRUE : VK_FALSE; rs.lineWidth = 1.f;   // CLOCKWISE: o shader inverte Y
    VkPipelineMultisampleStateCreateInfo ms = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO}; ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO}; ds.depthTestEnable = vkS.depth; ds.depthWriteEnable = vkS.dmask && vkS.depth; ds.depthCompareOp = (VkCompareOp)((vkS.dfunc - 0x200) & 7);   // GL_NEVER..GL_ALWAYS = VK_COMPARE_OP_NEVER..ALWAYS (mesma ordem)
    VkPipelineColorBlendAttachmentState ba = {}; ba.blendEnable = vkS.blend; ba.srcColorBlendFactor = ba.srcAlphaBlendFactor = vkBFv(vkBF(vkS.bsrc)); ba.dstColorBlendFactor = ba.dstAlphaBlendFactor = vkBFv(vkBF(vkS.bdst)); ba.colorBlendOp = ba.alphaBlendOp = VK_BLEND_OP_ADD; ba.colorWriteMask = vkS.cmask & 15;
    VkPipelineColorBlendStateCreateInfo cb = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO}; cb.attachmentCount = 1; cb.pAttachments = &ba;
    const VkDynamicState dyn[4] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS, VK_DYNAMIC_STATE_BLEND_CONSTANTS};
    VkPipelineDynamicStateCreateInfo dy = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO}; dy.dynamicStateCount = 4; dy.pDynamicStates = dyn;
    VkGraphicsPipelineCreateInfo pi = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO}; pi.stageCount = 2; pi.pStages = st; pi.pVertexInputState = &vi; pi.pInputAssemblyState = &ia; pi.pViewportState = &vs; pi.pRasterizationState = &rs; pi.pMultisampleState = &ms; pi.pDepthStencilState = &ds; pi.pColorBlendState = &cb; pi.pDynamicState = &dy; pi.layout = vkPL; pi.renderPass = vkRP;
    VkPipeline p = VK_NULL_HANDLE; VKC(vkCreateGraphicsPipelines(vkDev, VK_NULL_HANDLE, 1, &pi, nullptr, &p)); vkPipes[key] = p; return p;
}
// prepara o desenho: pipeline, viewport/scissor, uniforms (matrizes, neblina, alpha...) e textura
static bool vkPrep(bool lines, bool vcol) {
    if (!vkPass()) return false; VkFrame& f = vkF[vkCur];
    if (f.uoff + vkUAlign > VK_URING) { static bool w = false; if (!w) { w = true; logWarn("Vulkan: anel de uniforms cheio neste quadro"); } return false; }
    const VkPipeline p = vkPipe(lines); if (p != vkBound) { vkCmdBindPipeline(f.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, p); vkBound = p; }
    VkViewport vp = {(float)vkS.vp[0], (float)vkExt.height - (float)(vkS.vp[1] + vkS.vp[3]), (float)vkS.vp[2], (float)vkS.vp[3], 0.f, 1.f}; vkCmdSetViewport(f.cb, 0, 1, &vp);
    VkRect2D sc = {{0, 0}, vkExt}; if (vkS.scis) { const int x = std::max(vkS.sc[0], 0), y = std::max((int)vkExt.height - (vkS.sc[1] + vkS.sc[3]), 0); sc = {{x, y}, {(uint32_t)std::max(vkS.sc[2], 0), (uint32_t)std::max(vkS.sc[3], 0)}}; } vkCmdSetScissor(f.cb, 0, 1, &sc);
    vkCmdSetDepthBias(f.cb, vkS.poff ? vkS.pou : 0.f, 0.f, vkS.poff ? vkS.pof : 0.f); vkCmdSetBlendConstants(f.cb, vkS.bc);
    VkU u; m4mul(vkMat[1][vkTop[1]], vkMat[0][vkTop[0]], u.mvp); const float* mv = vkMat[0][vkTop[0]]; u.mvz[0] = mv[2]; u.mvz[1] = mv[6]; u.mvz[2] = mv[10]; u.mvz[3] = mv[14];
    memcpy(u.cur, vkS.cur, 16); memcpy(u.fogC, vkS.fogC, 16); u.fogP[0] = vkS.fogS; u.fogP[1] = vkS.fogE; u.fogP[2] = vkS.fogD; u.fogP[3] = (float)vkS.fogM;
    const int fl = (vkS.fog ? 1 : 0) | (vkS.tex2d ? 2 : 0) | (vkS.alphaT ? 4 : 0) | (vcol ? 8 : 0); u.alphaP[0] = (float)((vkS.afn - 0x200) & 7); u.alphaP[1] = vkS.aref; u.alphaP[2] = (float)fl; u.alphaP[3] = 0.f; memcpy(u.sky, vkS.sky, 16); memcpy(u.env, vkS.env, 16); memcpy(u.cam, vkS.cam, 16);
    memcpy(f.umap + f.uoff, &u, sizeof u); const uint32_t off = (uint32_t)f.uoff; f.uoff += vkUAlign;
    vkCmdBindDescriptorSets(f.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPL, 0, 1, &f.uset, 1, &off);
    const VkDescriptorSet ts = vkTexSet(vkS.tex2d ? vkS.tex : 0); if (ts != vkBoundTex || true) { vkCmdBindDescriptorSets(f.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPL, 1, 1, &ts, 0, nullptr); vkBoundTex = ts; }
    return true;
}
static void vkDrawV(const std::vector<VkV>& v, bool lines) {   // vertices ja montados (lista de triangulos/linhas) -> anel
    if (v.empty()) return; if (!vkBegin()) return; VkFrame& f = vkF[vkCur]; const size_t bytes = v.size() * sizeof(VkV);
    if (f.voff + bytes + 16 > VK_VRING) { static bool w = false; if (!w) { w = true; logWarn("Vulkan: anel de vertices cheio neste quadro"); } return; }
    if (!vkPrep(lines, true)) return; memcpy(f.vmap + f.voff, v.data(), bytes); const VkDeviceSize o = f.voff; f.voff += (bytes + 15) & ~(size_t)15;
    vkCmdBindVertexBuffers(f.cb, 0, 1, &f.vb, &o); vkCmdDraw(f.cb, (uint32_t)v.size(), 1, 0, 0);
}
static void vkAsm(GLenum mode, const VkV* v, int n, std::vector<VkV>& o, bool& lines) {   // GL_QUADS/STRIP/FAN... -> lista de triangulos (ou linhas)
    lines = false; o.clear();
    switch (mode) {
    case GL_TRIANGLES: o.assign(v, v + n / 3 * 3); break;
    case GL_QUADS: for (int i = 0; i + 3 < n; i += 4) { o.push_back(v[i]); o.push_back(v[i + 1]); o.push_back(v[i + 2]); o.push_back(v[i]); o.push_back(v[i + 2]); o.push_back(v[i + 3]); } break;
    case GL_TRIANGLE_STRIP: for (int i = 0; i + 2 < n; i++) { if (i & 1) { o.push_back(v[i + 1]); o.push_back(v[i]); } else { o.push_back(v[i]); o.push_back(v[i + 1]); } o.push_back(v[i + 2]); } break;
    case GL_TRIANGLE_FAN: case GL_POLYGON: for (int i = 1; i + 1 < n; i++) { o.push_back(v[0]); o.push_back(v[i]); o.push_back(v[i + 1]); } break;
    case GL_QUAD_STRIP: for (int i = 0; i + 3 < n; i += 2) { o.push_back(v[i]); o.push_back(v[i + 1]); o.push_back(v[i + 3]); o.push_back(v[i]); o.push_back(v[i + 3]); o.push_back(v[i + 2]); } break;
    case GL_LINES: lines = true; o.assign(v, v + n / 2 * 2); break;
    case GL_LINE_STRIP: lines = true; for (int i = 0; i + 1 < n; i++) { o.push_back(v[i]); o.push_back(v[i + 1]); } break;
    case GL_LINE_LOOP: lines = true; for (int i = 0; i < n; i++) { o.push_back(v[i]); o.push_back(v[(i + 1) % n]); } break;
    default: break;
    }
}

// ---------- API GL (redirecionada por macros no fim do arquivo) ----------
static void vkCap(GLenum c, bool on) {
    switch (c) { case GL_TEXTURE_2D: vkS.tex2d = on; break; case GL_BLEND: vkS.blend = on; break; case GL_DEPTH_TEST: vkS.depth = on; break; case GL_ALPHA_TEST: vkS.alphaT = on; break; case GL_FOG: vkS.fog = on; break;
    case GL_CULL_FACE: vkS.cull = on; break; case GL_SCISSOR_TEST: vkS.scis = on; break; case GL_POLYGON_OFFSET_FILL: vkS.poff = on; break; default: if (on) vkS.other.insert(c); else vkS.other.erase(c); }
}
static void vkglEnable(GLenum c) { vkCap(c, true); } static void vkglDisable(GLenum c) { vkCap(c, false); }
static GLboolean vkglIsEnabled(GLenum c) { switch (c) { case GL_TEXTURE_2D: return vkS.tex2d; case GL_BLEND: return vkS.blend; case GL_DEPTH_TEST: return vkS.depth; case GL_ALPHA_TEST: return vkS.alphaT; case GL_FOG: return vkS.fog; case GL_CULL_FACE: return vkS.cull; case GL_SCISSOR_TEST: return vkS.scis; case GL_POLYGON_OFFSET_FILL: return vkS.poff; } return vkS.other.count(c) ? GL_TRUE : GL_FALSE; }
static void vkglBlendFunc(GLenum s, GLenum d) { vkS.bsrc = s; vkS.bdst = d; }
static void vkglBlendColor(float r, float g, float b, float a) { vkS.bc[0] = r; vkS.bc[1] = g; vkS.bc[2] = b; vkS.bc[3] = a; }
static void vkglDepthMask(GLboolean m) { vkS.dmask = m != 0; } static void vkglDepthFunc(GLenum f) { vkS.dfunc = f; }
static void vkglColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { vkS.cmask = (r ? 1u : 0u) | (g ? 2u : 0u) | (b ? 4u : 0u) | (a ? 8u : 0u); }
static void vkglAlphaFunc(GLenum f, GLclampf r) { vkS.afn = f; vkS.aref = r; } static void vkglPolygonOffset(GLfloat f, GLfloat u) { vkS.pof = f; vkS.pou = u; }
static void vkglFogf(GLenum p, GLfloat v) { if (p == GL_FOG_START) vkS.fogS = v; else if (p == GL_FOG_END) vkS.fogE = v; else if (p == GL_FOG_DENSITY) vkS.fogD = v; else if (p == GL_FOG_MODE) vkS.fogM = v == (float)GL_LINEAR ? 0 : v == (float)GL_EXP ? 1 : 2; }
static void vkglFogi(GLenum p, GLint v) { vkglFogf(p, (float)v); } static void vkglFogfv(GLenum p, const GLfloat* v) { if (p == GL_FOG_COLOR) memcpy(vkS.fogC, v, 16); else vkglFogf(p, v[0]); }
static void vkglClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) { vkS.clr[0] = r; vkS.clr[1] = g; vkS.clr[2] = b; vkS.clr[3] = a; if (!vkPassOn) { vkCol0[0] = r; vkCol0[1] = g; vkCol0[2] = b; vkCol0[3] = a; } }
static void vkglClear(GLbitfield m) {
    if (!vkBegin()) return;
    if (!vkPassOn && !vkS.scis) { if (m & GL_COLOR_BUFFER_BIT) memcpy(vkCol0, vkS.clr, 16); return; }   // 1o clear do quadro: vira o loadOp do render pass (barato em GPU mobile)
    if (!vkPass()) return; VkClearAttachment a[2]; int n = 0;
    if (m & GL_COLOR_BUFFER_BIT) { a[n] = {}; a[n].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; memcpy(a[n].clearValue.color.float32, vkS.clr, 16); n++; }
    if (m & GL_DEPTH_BUFFER_BIT) { a[n] = {}; a[n].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; a[n].clearValue.depthStencil = {1.f, 0}; n++; }
    if (!n) return; VkClearRect r = {{{0, 0}, vkExt}, 0, 1}; if (vkS.scis) r.rect = {{std::max(vkS.sc[0], 0), std::max((int)vkExt.height - (vkS.sc[1] + vkS.sc[3]), 0)}, {(uint32_t)std::max(vkS.sc[2], 0), (uint32_t)std::max(vkS.sc[3], 0)}};
    vkCmdClearAttachments(vkF[vkCur].cb, (uint32_t)n, a, 1, &r);
}
static void vkglViewport(GLint x, GLint y, GLsizei w, GLsizei h) { vkS.vp[0] = x; vkS.vp[1] = y; vkS.vp[2] = w; vkS.vp[3] = h; } static void vkglScissor(GLint x, GLint y, GLsizei w, GLsizei h) { vkS.sc[0] = x; vkS.sc[1] = y; vkS.sc[2] = w; vkS.sc[3] = h; }
static void vkglMatrixMode(GLenum m) { vkMM = m == GL_PROJECTION ? 1 : m == GL_TEXTURE ? 2 : 0; } static void vkglLoadIdentity() { m4id(vkM()); } static void vkglLoadMatrixf(const GLfloat* m) { memcpy(vkM(), m, 64); }
static void vkglPushMatrix() { if (vkTop[vkMM] < 31) { memcpy(vkMat[vkMM][vkTop[vkMM] + 1], vkM(), 64); vkTop[vkMM]++; } } static void vkglPopMatrix() { if (vkTop[vkMM] > 0) vkTop[vkMM]--; }
static void vkglTranslatef(GLfloat x, GLfloat y, GLfloat z) { float* m = vkM(); for (int i = 0; i < 4; i++) m[12 + i] += m[i] * x + m[4 + i] * y + m[8 + i] * z; }
static void vkglScalef(GLfloat x, GLfloat y, GLfloat z) { float* m = vkM(); for (int i = 0; i < 4; i++) { m[i] *= x; m[4 + i] *= y; m[8 + i] *= z; } }
static void vkglRotatef(GLfloat a, GLfloat x, GLfloat y, GLfloat z) {
    const float l = sqrtf(x * x + y * y + z * z); if (l < 1e-8f) return; x /= l; y /= l; z /= l; const float r = a * 0.017453292519943295f, c = cosf(r), s = sinf(r), t = 1.f - c; float R[16]; m4id(R);
    R[0] = t * x * x + c; R[1] = t * x * y + s * z; R[2] = t * x * z - s * y; R[4] = t * x * y - s * z; R[5] = t * y * y + c; R[6] = t * y * z + s * x; R[8] = t * x * z + s * y; R[9] = t * y * z - s * x; R[10] = t * z * z + c; m4mul(vkM(), R, vkM());
}
static void vkglOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) { float O[16]; m4id(O); O[0] = (float)(2 / (r - l)); O[5] = (float)(2 / (t - b)); O[10] = (float)(-2 / (f - n)); O[12] = (float)(-(r + l) / (r - l)); O[13] = (float)(-(t + b) / (t - b)); O[14] = (float)(-(f + n) / (f - n)); m4mul(vkM(), O, vkM()); }
static void vkglFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) { float F[16]; memset(F, 0, 64); F[0] = (float)(2 * n / (r - l)); F[5] = (float)(2 * n / (t - b)); F[8] = (float)((r + l) / (r - l)); F[9] = (float)((t + b) / (t - b)); F[10] = (float)(-(f + n) / (f - n)); F[11] = -1.f; F[14] = (float)(-2 * f * n / (f - n)); m4mul(vkM(), F, vkM()); }
static void vkglGetFloatv(GLenum p, GLfloat* v) { if (p == GL_MODELVIEW_MATRIX) memcpy(v, vkMat[0][vkTop[0]], 64); else if (p == GL_PROJECTION_MATRIX) memcpy(v, vkMat[1][vkTop[1]], 64); else if (p == GL_FOG_START) v[0] = vkS.fogS; else if (p == GL_FOG_END) v[0] = vkS.fogE; else v[0] = 0.f; }
static void vkglGetIntegerv(GLenum p, GLint* v) { if (p == GL_VIEWPORT) memcpy(v, vkS.vp, 16); else if (p == GL_TEXTURE_BINDING_2D) v[0] = (GLint)vkS.tex; else v[0] = 0; }
static const GLubyte* vkglGetString(GLenum n) { static std::string v = "Vulkan", r, d; if (r.empty()) { r = vkPDP.deviceName; d = "Vulkan " + std::to_string(VK_VERSION_MAJOR(vkPDP.apiVersion)) + "." + std::to_string(VK_VERSION_MINOR(vkPDP.apiVersion)) + " (blockland vkgl)"; } return (const GLubyte*)(n == GL_RENDERER ? r.c_str() : n == GL_VERSION ? d.c_str() : n == GL_VENDOR ? "Vulkan" : ""); }
static GLenum vkglGetError() { return 0; }
// cor/textura correntes e modo imediato
static void vkglColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { vkS.cur[0] = r; vkS.cur[1] = g; vkS.cur[2] = b; vkS.cur[3] = a; } static void vkglColor3f(GLfloat r, GLfloat g, GLfloat b) { vkglColor4f(r, g, b, 1.f); }
static void vkglTexCoord2f(GLfloat u, GLfloat v) { vkTC[0] = u; vkTC[1] = v; }
static void vkglBegin(GLenum m) { vkImmMode = m; vkImm.clear(); }
static void vkglVertex3f(GLfloat x, GLfloat y, GLfloat z) { vkImm.push_back({x, y, z, vkTC[0], vkTC[1], vkS.cur[0], vkS.cur[1], vkS.cur[2], vkS.cur[3]}); }
static void vkglVertex2f(GLfloat x, GLfloat y) { vkglVertex3f(x, y, 0.f); } static void vkglVertex3d(GLdouble x, GLdouble y, GLdouble z) { vkglVertex3f((float)x, (float)y, (float)z); }
static void vkglEnd() { std::vector<VkV> o; bool ln = false; vkAsm(vkImmMode, vkImm.data(), (int)vkImm.size(), o, ln); if (vkListCur) vkLists[vkListCur].parts.push_back({ln, std::move(o)}); else vkDrawV(o, ln); vkImm.clear(); }
static GLuint vkglGenLists(GLsizei n) { const GLuint f = (GLuint)vkLists.size(); for (int i = 0; i < n; i++) vkLists.emplace_back(); return f; }
static void vkglNewList(GLuint id, GLenum) { if (id < vkLists.size()) { vkListCur = (int)id; vkLists[id].parts.clear(); } } static void vkglEndList() { vkListCur = 0; }
static void vkglCallList(GLuint id) { if (id < vkLists.size()) for (auto& p : vkLists[id].parts) vkDrawV(p.second, p.first); }
// arrays de vertices
static void vkglEnableClientState(GLenum a) { if (a == GL_VERTEX_ARRAY) vkAV.en = true; else if (a == GL_TEXTURE_COORD_ARRAY) vkAT.en = true; else if (a == GL_COLOR_ARRAY) vkAC.en = true; }
static void vkglDisableClientState(GLenum a) { if (a == GL_VERTEX_ARRAY) vkAV.en = false; else if (a == GL_TEXTURE_COORD_ARRAY) vkAT.en = false; else if (a == GL_COLOR_ARRAY) vkAC.en = false; }
static void vkglVertexPointer(GLint s, GLenum t, GLsizei st, const void* p) { vkAV.size = s; vkAV.type = t; vkAV.stride = st; vkAV.ptr = p; } static void vkglTexCoordPointer(GLint s, GLenum t, GLsizei st, const void* p) { vkAT.size = s; vkAT.type = t; vkAT.stride = st; vkAT.ptr = p; }
static void vkglColorPointer(GLint s, GLenum t, GLsizei st, const void* p) { vkAC.size = s; vkAC.type = t; vkAC.stride = st; vkAC.ptr = p; }
static void vkGatherOne(int i, VkV& o) {   // le o vertice i dos arrays de cliente (ponteiros reais)
    o = {0, 0, 0, 0, 0, vkS.cur[0], vkS.cur[1], vkS.cur[2], vkS.cur[3]};
    auto rd = [](const VkArr& a, int i, float* d, int n, bool col) { const uint8_t* p = (const uint8_t*)a.ptr + (size_t)i * (a.stride ? a.stride : a.size * (a.type == GL_FLOAT ? 4 : a.type == GL_UNSIGNED_BYTE ? 1 : 2)); for (int k = 0; k < n && k < a.size; k++) { if (a.type == GL_FLOAT) d[k] = ((const float*)p)[k]; else if (a.type == GL_UNSIGNED_BYTE) d[k] = p[k] / 255.f; else if (a.type == GL_SHORT) d[k] = ((const int16_t*)p)[k] * (col ? 1.f / 32767.f : 1.f); } };
    float t[4];
    if (vkAV.en && vkAV.ptr) { t[2] = 0; rd(vkAV, i, t, 3, false); o.x = t[0]; o.y = t[1]; o.z = t[2]; }
    if (vkAT.en && vkAT.ptr) { t[0] = t[1] = 0; rd(vkAT, i, t, 2, false); o.u = t[0]; o.v = t[1]; }
    if (vkAC.en && vkAC.ptr) { t[0] = t[1] = t[2] = 0; t[3] = 1; rd(vkAC, i, t, 4, true); o.r = t[0]; o.g = t[1]; o.b = t[2]; o.a = t[3]; }
}
static void vkBindVB(size_t off) { VkFrame& f = vkF[vkCur]; const VkDeviceSize o = off; vkCmdBindVertexBuffers(f.cb, 0, 1, &vkBufs[vkAB].buf, &o); }
static void vkglDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (count <= 0 || !vkAV.en) return;
    if (vkAB && vkAB < vkBufs.size() && vkBufs[vkAB].buf) {   // VBO: layout igual ao VkV -> usa direto
        if (mode != GL_QUADS && mode != GL_TRIANGLES) return; if (!vkPrep(false, vkAC.en)) return; const size_t base = (size_t)(uintptr_t)vkAV.ptr; VkFrame& f = vkF[vkCur];
        if (mode == GL_TRIANGLES) { vkBindVB(base + (size_t)first * sizeof(VkV)); vkCmdDraw(f.cb, (uint32_t)count, 1, 0, 0); return; }
        vkCmdBindIndexBuffer(f.cb, vkQuadIB.buf, 0, VK_INDEX_TYPE_UINT16);
        for (int d = 0; d < count; d += VK_QMAX) { const int n = std::min(count - d, VK_QMAX); vkBindVB(base + (size_t)(first + d) * sizeof(VkV)); vkCmdDrawIndexed(f.cb, (uint32_t)(n / 4 * 6), 1, 0, 0, 0); }
        return;
    }
    std::vector<VkV> v((size_t)count); for (int i = 0; i < count; i++) vkGatherOne(first + i, v[i]); std::vector<VkV> o; bool ln; vkAsm(mode, v.data(), count, o, ln); vkDrawV(o, ln);
}
static void vkglDrawElements(GLenum mode, GLsizei count, GLenum type, const void* idx) {
    if (count <= 0 || !vkAV.en) return;
    if (vkAB && vkAB < vkBufs.size() && vkBufs[vkAB].buf && vkEB && vkEB < vkBufs.size() && vkBufs[vkEB].buf) {
        if (mode != GL_TRIANGLES || !vkPrep(false, vkAC.en)) return; VkFrame& f = vkF[vkCur];
        vkCmdBindIndexBuffer(f.cb, vkBufs[vkEB].buf, (VkDeviceSize)(uintptr_t)idx, type == GL_UNSIGNED_INT ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16); vkBindVB((size_t)(uintptr_t)vkAV.ptr); vkCmdDrawIndexed(f.cb, (uint32_t)count, 1, 0, 0, 0); return;
    }
    std::vector<VkV> v((size_t)count); for (int i = 0; i < count; i++) { const int k = type == GL_UNSIGNED_INT ? (int)((const uint32_t*)idx)[i] : type == GL_UNSIGNED_BYTE ? (int)((const uint8_t*)idx)[i] : (int)((const uint16_t*)idx)[i]; vkGatherOne(k, v[i]); }
    std::vector<VkV> o; bool ln; vkAsm(mode, v.data(), count, o, ln); vkDrawV(o, ln);
}
// buffers (VBO/IBO)
static void vkglGenBuffers(GLsizei n, GLuint* ids) { for (int i = 0; i < n; i++) { ids[i] = (GLuint)vkBufs.size(); vkBufs.emplace_back(); } }
static void vkglBindBuffer(GLenum t, GLuint id) { if (t == 0x8892) vkAB = id; else if (t == 0x8893) vkEB = id; }
static void vkFreeBuf(VkBuf& b) { if (b.buf) { const VkBuffer x = b.buf; const VkDeviceMemory m = b.mem; vkLater([=] { vkDestroyBuffer(vkDev, x, nullptr); vkFreeMemory(vkDev, m, nullptr); }); b.buf = VK_NULL_HANDLE; b.mem = VK_NULL_HANDLE; } }
static void vkglBufferData(GLenum t, GLsizeiptr size, const void* data, GLenum) {
    const GLuint id = t == 0x8892 ? vkAB : t == 0x8893 ? vkEB : 0; if (!id || id >= vkBufs.size() || size <= 0) return; VkBuf& b = vkBufs[id]; vkFreeBuf(b);   // sempre um buffer novo: o antigo pode estar em uso por um quadro anterior
    void* mp = nullptr; if (!vkMakeBuf((VkDeviceSize)size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, b.buf, b.mem, &mp)) { logError("Vulkan: sem memoria para buffer de %lld bytes", (long long)size); return; }
    if (data) memcpy(mp, data, (size_t)size); vkUnmapMemory(vkDev, b.mem);
}
static void vkglDeleteBuffers(GLsizei n, const GLuint* ids) { for (int i = 0; i < n; i++) if (ids[i] && ids[i] < vkBufs.size()) vkFreeBuf(vkBufs[ids[i]]); }
// texturas
static void vkglGenTextures(GLsizei n, GLuint* ids) { for (int i = 0; i < n; i++) { ids[i] = (GLuint)vkTexs.size(); vkTexs.emplace_back(); } }
static void vkglBindTexture(GLenum t, GLuint id) { if (t == GL_TEXTURE_2D) vkS.tex = id < vkTexs.size() ? id : 0; }
static void vkglDeleteTextures(GLsizei n, const GLuint* ids) { for (int i = 0; i < n; i++) { if (ids[i] == 0 || ids[i] >= vkTexs.size()) continue; VkTex& t = vkTexs[ids[i]]; const VkImage im = t.img; const VkImageView v = t.view; const VkDeviceMemory m = t.mem; const VkDescriptorSet s = t.set;
    vkLater([=] { if (s) vkFreeDescriptorSets(vkDev, vkDPT, 1, &s); if (v) vkDestroyImageView(vkDev, v, nullptr); if (im) vkDestroyImage(vkDev, im, nullptr); if (m) vkFreeMemory(vkDev, m, nullptr); }); t = VkTex(); } }
static void vkglTexImage2D(GLenum t, GLint level, GLint, GLsizei w, GLsizei h, GLint, GLenum fmt, GLenum type, const void* px) {
    if (t != GL_TEXTURE_2D || vkS.tex == 0 || vkS.tex >= vkTexs.size() || fmt != GL_RGBA || type != GL_UNSIGNED_BYTE || level < 0 || level > 15) return;   // so RGBA8 (profundidade/FBO: nao suportado nesta camada)
    VkTex& T = vkTexs[vkS.tex]; if (T.used && level == 0) { T.lv.clear(); T.wh.clear(); T.used = false; }
    if ((int)T.lv.size() <= level) { T.lv.resize(level + 1); T.wh.resize(level + 1); } T.lv[level].assign((size_t)w * h * 4, 0); if (px) memcpy(T.lv[level].data(), px, (size_t)w * h * 4); T.wh[level] = {w, h}; T.dirty = true;
}
static void vkglTexSubImage2D(GLenum t, GLint level, GLint xo, GLint yo, GLsizei w, GLsizei h, GLenum fmt, GLenum type, const void* px) {   // atualiza um retangulo do nivel (CPU) e remarca a textura para reenvio
    if (t != GL_TEXTURE_2D || !px || vkS.tex == 0 || vkS.tex >= vkTexs.size() || fmt != GL_RGBA || type != GL_UNSIGNED_BYTE || level < 0) return;
    VkTex& T = vkTexs[vkS.tex]; if (level >= (int)T.lv.size() || T.lv[level].empty()) return;
    const int tw = T.wh[level].first, th = T.wh[level].second; if (xo < 0 || yo < 0 || xo + w > tw || yo + h > th) return;
    for (int y = 0; y < h; y++) memcpy(&T.lv[level][((size_t)(yo + y) * tw + xo) * 4], (const uint8_t*)px + (size_t)y * w * 4, (size_t)w * 4);
    T.dirty = true;
}
static void vkglTexParameteri(GLenum t, GLenum p, GLint v) { if (t != GL_TEXTURE_2D || vkS.tex == 0 || vkS.tex >= vkTexs.size()) return; VkTex& T = vkTexs[vkS.tex]; if (p == 0x2801) T.minF = (GLenum)v; else if (p == 0x2800) T.magF = (GLenum)v; else if (p == 0x2802) T.ws = (GLenum)v; else if (p == 0x2803) T.wt = (GLenum)v; }
static void vkglActiveTexture(GLenum) {}
// sem efeito nesta camada (iluminacao fixa, stencil, FBOs, atributos...)
static void vkglNop() {}
static void vkglLightModelfv(GLenum, const GLfloat*) {} static void vkglColorMaterial(GLenum, GLenum) {} static void vkglStencilOp(GLenum, GLenum, GLenum) {} static void vkglStencilFunc(GLenum, GLint, GLuint) {} static void vkglClearStencil(GLint) {}
static void vkglReadBuffer(GLenum) {} static void vkglDrawBuffer(GLenum) {} static void vkglCopyTexSubImage2D(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei) {} static void vkglPixelStorei(GLenum, GLint) {} static void vkglHint(GLenum, GLenum) {}
static void vkglLineWidth(GLfloat) {} static void vkglClipPlane(GLenum, const GLdouble*) {} static void vkglFlush() {} static void vkglPushAttrib(GLbitfield) {} static void vkglPopAttrib() {} static void vkglPushClientAttrib(GLbitfield) {} static void vkglPopClientAttrib() {}

// ---------- criacao / destruicao ----------
static std::vector<char> vkDummy;
static bool vkInit(SDL_Window* win) {
    vkWin = win; uint32_t ne = 0; if (!SDL_Vulkan_GetInstanceExtensions(win, &ne, nullptr)) { SDL_SetError("Vulkan: SDL nao achou extensoes de instancia (%s)", SDL_GetError()); return false; }
    std::vector<const char*> ext(ne); SDL_Vulkan_GetInstanceExtensions(win, &ne, ext.data());
    VkApplicationInfo ap = {VK_STRUCTURE_TYPE_APPLICATION_INFO}; ap.pApplicationName = "Blockland"; ap.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo ic = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO}; ic.pApplicationInfo = &ap; ic.enabledExtensionCount = ne; ic.ppEnabledExtensionNames = ext.data();
    if (vkCreateInstance(&ic, nullptr, &vkInst) != VK_SUCCESS) { SDL_SetError("Vulkan: vkCreateInstance falhou"); return false; }
    if (!SDL_Vulkan_CreateSurface(win, vkInst, &vkSurf)) { SDL_SetError("Vulkan: sem surface (%s)", SDL_GetError()); return false; }
    uint32_t nd = 0; vkEnumeratePhysicalDevices(vkInst, &nd, nullptr); std::vector<VkPhysicalDevice> pds(nd); vkEnumeratePhysicalDevices(vkInst, &nd, pds.data()); int bestScore = -1;
    for (VkPhysicalDevice pd : pds) {
        uint32_t nq = 0; vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, nullptr); std::vector<VkQueueFamilyProperties> qp(nq); vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, qp.data());
        for (uint32_t q = 0; q < nq; q++) { VkBool32 pr = VK_FALSE; vkGetPhysicalDeviceSurfaceSupportKHR(pd, q, vkSurf, &pr); if (!(qp[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) || !pr) continue;
            VkPhysicalDeviceProperties pp; vkGetPhysicalDeviceProperties(pd, &pp); const int sc = pp.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 3 : pp.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 2 : 1; if (sc > bestScore) { bestScore = sc; vkPD = pd; vkQF = q; } break; }
    }
    if (!vkPD) { SDL_SetError("Vulkan: nenhuma GPU com fila grafica/apresentacao"); return false; }
    vkGetPhysicalDeviceProperties(vkPD, &vkPDP); vkGetPhysicalDeviceMemoryProperties(vkPD, &vkMemP); vkUAlign = std::max<size_t>(256, (size_t)vkPDP.limits.minUniformBufferOffsetAlignment);
    if (vkUAlign > 256) vkUAlign = (vkUAlign + 255) & ~(size_t)255;
    { const VkFormat c[4] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D16_UNORM}; vkDFmt = c[3];
      for (int i = 3; i >= 0; i--) { VkFormatProperties fp; vkGetPhysicalDeviceFormatProperties(vkPD, c[i], &fp); if (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) vkDFmt = c[i]; } }
    const float pr = 1.f; VkDeviceQueueCreateInfo qi = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO}; qi.queueFamilyIndex = vkQF; qi.queueCount = 1; qi.pQueuePriorities = &pr; const char* de[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo di = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO}; di.queueCreateInfoCount = 1; di.pQueueCreateInfos = &qi; di.enabledExtensionCount = 1; di.ppEnabledExtensionNames = de;
    if (vkCreateDevice(vkPD, &di, nullptr, &vkDev) != VK_SUCCESS) { SDL_SetError("Vulkan: vkCreateDevice falhou"); return false; }
    vkGetDeviceQueue(vkDev, vkQF, 0, &vkQ);
    VkCommandPoolCreateInfo cp = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO}; cp.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; cp.queueFamilyIndex = vkQF; VKC(vkCreateCommandPool(vkDev, &cp, nullptr, &vkPool));
    // layouts: set0 = uniforms dinamicos (VS+FS), set1 = textura (FS)
    { VkDescriptorSetLayoutBinding b = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}; VkDescriptorSetLayoutCreateInfo li = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; li.bindingCount = 1; li.pBindings = &b; VKC(vkCreateDescriptorSetLayout(vkDev, &li, nullptr, &vkDSLU));
      b = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}; VKC(vkCreateDescriptorSetLayout(vkDev, &li, nullptr, &vkDSLT)); }
    { VkDescriptorSetLayout ls[2] = {vkDSLU, vkDSLT}; VkPipelineLayoutCreateInfo pl = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO}; pl.setLayoutCount = 2; pl.pSetLayouts = ls; VKC(vkCreatePipelineLayout(vkDev, &pl, nullptr, &vkPL)); }
    { VkDescriptorPoolSize ps = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2}; VkDescriptorPoolCreateInfo pc = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO}; pc.maxSets = 2; pc.poolSizeCount = 1; pc.pPoolSizes = &ps; VKC(vkCreateDescriptorPool(vkDev, &pc, nullptr, &vkDPU));
      ps = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096}; pc.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; pc.maxSets = 4096; VKC(vkCreateDescriptorPool(vkDev, &pc, nullptr, &vkDPT)); }
    VkShaderModuleCreateInfo sm = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO}; sm.codeSize = sizeof kVkVertSpv; sm.pCode = kVkVertSpv; VKC(vkCreateShaderModule(vkDev, &sm, nullptr, &vkVS)); sm.codeSize = sizeof kVkFragSpv; sm.pCode = kVkFragSpv; VKC(vkCreateShaderModule(vkDev, &sm, nullptr, &vkFS));
    int dw = 0, dh = 0; SDL_Vulkan_GetDrawableSize(win, &dw, &dh); vkS.vp[2] = std::max(dw, 1); vkS.vp[3] = std::max(dh, 1); vkS.sc[2] = vkS.vp[2]; vkS.sc[3] = vkS.vp[3];
    if (!vkMakeSC()) { SDL_SetError("Vulkan: swapchain falhou"); return false; }
    for (int i = 0; i < 2; i++) {
        VkFrame& f = vkF[i]; VkCommandBufferAllocateInfo ca = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO}; ca.commandPool = vkPool; ca.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; ca.commandBufferCount = 1; VKC(vkAllocateCommandBuffers(vkDev, &ca, &f.cb));
        VkFenceCreateInfo fi = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO}; fi.flags = VK_FENCE_CREATE_SIGNALED_BIT; vkCreateFence(vkDev, &fi, nullptr, &f.fence); VkSemaphoreCreateInfo si = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO}; vkCreateSemaphore(vkDev, &si, nullptr, &f.acq);
        void* mp = nullptr; if (!vkMakeBuf(VK_VRING, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, f.vb, f.vm, &mp)) { SDL_SetError("Vulkan: sem memoria para o anel de vertices"); return false; } f.vmap = (uint8_t*)mp;
        if (!vkMakeBuf(VK_URING, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, f.ub, f.um, &mp)) { SDL_SetError("Vulkan: sem memoria para o anel de uniforms"); return false; } f.umap = (uint8_t*)mp;
        VkDescriptorSetAllocateInfo da = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO}; da.descriptorPool = vkDPU; da.descriptorSetCount = 1; da.pSetLayouts = &vkDSLU; VKC(vkAllocateDescriptorSets(vkDev, &da, &f.uset));
        VkDescriptorBufferInfo bi = {f.ub, 0, sizeof(VkU)}; VkWriteDescriptorSet w = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}; w.dstSet = f.uset; w.descriptorCount = 1; w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; w.pBufferInfo = &bi; vkUpdateDescriptorSets(vkDev, 1, &w, 0, nullptr);
    }
    { void* mp = nullptr; const int nq = VK_QMAX / 4; if (!vkMakeBuf((VkDeviceSize)nq * 6 * 2, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, vkQuadIB.buf, vkQuadIB.mem, &mp)) return false; uint16_t* p = (uint16_t*)mp;
      for (int q = 0; q < nq; q++) { const uint16_t b = (uint16_t)(q * 4); p[q * 6] = b; p[q * 6 + 1] = b + 1; p[q * 6 + 2] = b + 2; p[q * 6 + 3] = b; p[q * 6 + 4] = b + 2; p[q * 6 + 5] = b + 3; } }
    for (int m = 0; m < 3; m++) m4id(vkMat[m][0]);
    vkTexs[0].lv.clear(); vkReady = true;   // textura 0 = branco 1x1 (criada no 1o uso)
    logInfo("Vulkan: %s | API %u.%u | swapchain %ux%u | profundidade fmt %d", vkPDP.deviceName, VK_VERSION_MAJOR(vkPDP.apiVersion), VK_VERSION_MINOR(vkPDP.apiVersion), vkExt.width, vkExt.height, (int)vkDFmt); return true;
}
static SDL_GLContext vkglCreate(SDL_Window* w) {
    if (!vkInit(w)) {
#ifdef __ANDROID__
        if (FILE* f = fopen("no_vulkan", "wb")) { fputs("1", f); fclose(f); }   // o proximo inicio usa a versao OpenGL (BlocklandActivity)
#endif
        return nullptr; }
    return (SDL_GLContext)1;
}
static void vkglDestroy() { if (!vkDev) return; vkDeviceWaitIdle(vkDev); for (auto& d : vkDefer) d.second(); vkDefer.clear(); }   // (o SO libera o resto ao encerrar o processo)
static int vkglSwapInterval(int i) { const bool v = i != 0; if (v != vkVsync) { vkVsync = v; vkNeedSC = true; } return 0; }
static void* vkglProc(const char* n) {
    static const std::pair<const char*, void*> T[] = {{"glGenBuffers", (void*)vkglGenBuffers}, {"glBindBuffer", (void*)vkglBindBuffer}, {"glBufferData", (void*)vkglBufferData}, {"glDeleteBuffers", (void*)vkglDeleteBuffers}, {"glActiveTexture", (void*)vkglActiveTexture}, {"glBlendColor", (void*)vkglBlendColor}};
    for (const auto& e : T) if (!strcmp(e.first, n)) return e.second; return nullptr;   // shaders/FBO GL: indisponiveis (o jogo desliga sozinho)
}

// ---------- redirecionamento: o resto do jogo continua chamando gl* / SDL_GL_* ----------
#undef SDL_GL_GetProcAddress
#undef SDL_GL_SwapWindow
#define SDL_GL_GetProcAddress vkglProc
#define SDL_GL_SwapWindow(w) vkSwap()
#define SDL_GL_CreateContext vkglCreate
#define SDL_GL_DeleteContext(c) vkglDestroy()
#define SDL_GL_SetSwapInterval vkglSwapInterval
#define SDL_GL_SetAttribute(a, b) 0
#define SDL_WINDOW_OPENGL SDL_WINDOW_VULKAN
#define glEnable vkglEnable
#define glDisable vkglDisable
#define glIsEnabled vkglIsEnabled
#define glBlendFunc vkglBlendFunc
#define glDepthMask vkglDepthMask
#define glDepthFunc vkglDepthFunc
#define glColorMask vkglColorMask
#define glAlphaFunc vkglAlphaFunc
#define glPolygonOffset vkglPolygonOffset
#define glFogf vkglFogf
#define glFogi vkglFogi
#define glFogfv vkglFogfv
#define glClearColor vkglClearColor
#define glClear vkglClear
#define glViewport vkglViewport
#define glScissor vkglScissor
#define glMatrixMode vkglMatrixMode
#define glLoadIdentity vkglLoadIdentity
#define glLoadMatrixf vkglLoadMatrixf
#define glPushMatrix vkglPushMatrix
#define glPopMatrix vkglPopMatrix
#define glTranslatef vkglTranslatef
#define glScalef vkglScalef
#define glRotatef vkglRotatef
#define glOrtho vkglOrtho
#define glFrustum vkglFrustum
#define glGetFloatv vkglGetFloatv
#define glGetIntegerv vkglGetIntegerv
#define glGetString vkglGetString
#define glGetError vkglGetError
#define glColor4f vkglColor4f
#define glColor3f vkglColor3f
#define glTexCoord2f vkglTexCoord2f
#define glBegin vkglBegin
#define glEnd vkglEnd
#define glVertex3f vkglVertex3f
#define glVertex2f vkglVertex2f
#define glVertex3d vkglVertex3d
#define glGenLists vkglGenLists
#define glNewList vkglNewList
#define glEndList vkglEndList
#define glCallList vkglCallList
#define glEnableClientState vkglEnableClientState
#define glDisableClientState vkglDisableClientState
#define glVertexPointer vkglVertexPointer
#define glTexCoordPointer vkglTexCoordPointer
#define glColorPointer vkglColorPointer
#define glDrawArrays vkglDrawArrays
#define glDrawElements vkglDrawElements
#define glGenBuffers vkglGenBuffers
#define glBindBuffer vkglBindBuffer
#define glBufferData vkglBufferData
#define glDeleteBuffers vkglDeleteBuffers
#define glGenTextures vkglGenTextures
#define glBindTexture vkglBindTexture
#define glDeleteTextures vkglDeleteTextures
#define glTexImage2D vkglTexImage2D
#define glTexSubImage2D vkglTexSubImage2D
#define glTexParameteri vkglTexParameteri
#define glActiveTexture vkglActiveTexture
#define glBlendColor vkglBlendColor
#define glLightModelfv vkglLightModelfv
#define glColorMaterial vkglColorMaterial
#define glStencilOp vkglStencilOp
#define glStencilFunc vkglStencilFunc
#define glClearStencil vkglClearStencil
#define glReadBuffer vkglReadBuffer
#define glDrawBuffer vkglDrawBuffer
#define glCopyTexSubImage2D vkglCopyTexSubImage2D
#define glPixelStorei vkglPixelStorei
#define glHint vkglHint
#define glLineWidth vkglLineWidth
#define glClipPlane vkglClipPlane
#define glFlush vkglFlush
#define glPushAttrib vkglPushAttrib
#define glPopAttrib vkglPopAttrib
#define glPushClientAttrib vkglPushClientAttrib
#define glPopClientAttrib vkglPopClientAttrib
